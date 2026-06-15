#include <Windows.h>

#include <algorithm>
#include <clocale>
#include <cwchar>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#define LOG_ERROR(api) PrintLastError((api), __FILE__, __FUNCTION__, __LINE__)

constexpr const wchar_t* META_DIR = L".minirepo";
constexpr const wchar_t* SNAPSHOT_FILE = L"snapshot.dat";
constexpr const wchar_t* CHANGE_LOG_FILE = L"change.log";
constexpr DWORD SNAPSHOT_MAGIC = 0x4D525031; // MRP1
constexpr DWORD SNAPSHOT_VERSION = 1;

struct FileRecord
{
    std::wstring relativePath;
    ULONGLONG size = 0;
    FILETIME lastWriteTime = { 0, 0 };
};

struct SnapshotHeader
{
    DWORD magic = SNAPSHOT_MAGIC;
    DWORD version = SNAPSHOT_VERSION;
    DWORD count = 0;
};

struct SnapshotDiskRecord
{
    WCHAR relativePath[MAX_PATH] = { 0, };
    ULONGLONG size = 0;
    FILETIME lastWriteTime = { 0, 0 };
};

struct WatchContext
{
    std::wstring repoPath;
    HANDLE stopEvent = NULL;
};

HANDLE g_watchThread = NULL;
HANDLE g_stopEvent = NULL;
std::wstring g_repoPath;

std::wstring JoinPath(const std::wstring& left, const std::wstring& right)
{
    if (left.empty())
        return right;
    if (right.empty())
        return left;
    if (left.back() == L'\\' || left.back() == L'/')
        return left + right;
    return left + L"\\" + right;
}

std::wstring Trim(const std::wstring& text)
{
    const wchar_t* whitespace = L" \t\r\n";
    size_t first = text.find_first_not_of(whitespace);
    if (first == std::wstring::npos)
        return L"";

    size_t last = text.find_last_not_of(whitespace);
    return text.substr(first, last - first + 1);
}

void PrintLastError(const char* api, const char* file, const char* func, int line)
{
    DWORD err = GetLastError();
    std::cout << "[ERROR] " << api << " failed. code=" << err
        << " (" << file << ":" << line << ", " << func << ")\n";
}

bool IsDotName(const WCHAR* name)
{
    return wcscmp(name, L".") == 0 || wcscmp(name, L"..") == 0;
}

bool IsInsideMetaDir(const std::wstring& relativePath)
{
    return relativePath == META_DIR ||
        relativePath.rfind(std::wstring(META_DIR) + L"\\", 0) == 0;
}

bool EnsureDirectory(const std::wstring& path)
{
    if (CreateDirectoryW(path.c_str(), NULL))
        return true;

    DWORD err = GetLastError();
    if (err == ERROR_ALREADY_EXISTS)
        return true;

    LOG_ERROR("CreateDirectoryW");
    return false;
}

bool DirectoryExists(const std::wstring& path)
{
    DWORD attr = GetFileAttributesW(path.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

bool AppendLog(const std::wstring& repoPath, const std::wstring& message)
{
    std::wstring logPath = JoinPath(JoinPath(repoPath, META_DIR), CHANGE_LOG_FILE);
    HANDLE hFile = CreateFileW(
        logPath.c_str(),
        FILE_APPEND_DATA,
        FILE_SHARE_READ,
        NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        LOG_ERROR("CreateFileW(log)");
        return false;
    }

    SYSTEMTIME st = { 0, };
    GetLocalTime(&st);

    wchar_t timeBuf[64] = { 0, };
    swprintf_s(timeBuf, L"[%04d-%02d-%02d %02d:%02d:%02d] ",
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

    std::wstring line = std::wstring(timeBuf) + message + L"\r\n";
    DWORD bytesToWrite = static_cast<DWORD>(line.size() * sizeof(wchar_t));
    DWORD written = 0;
    BOOL ok = WriteFile(hFile, line.c_str(), bytesToWrite, &written, NULL);

    CloseHandle(hFile);

    if (!ok || written != bytesToWrite)
    {
        LOG_ERROR("WriteFile(log)");
        return false;
    }

    return true;
}

bool InitRepository(const std::wstring& repoPath)
{
    if (!EnsureDirectory(repoPath))
        return false;

    std::wstring metaPath = JoinPath(repoPath, META_DIR);
    if (!EnsureDirectory(metaPath))
        return false;

    AppendLog(repoPath, L"repository initialized");
    std::wcout << L"Repository ready: " << repoPath << L"\n";
    return true;
}

bool ReadFileInfo(const std::wstring& fullPath, FileRecord& record)
{
    HANDLE hFile = CreateFileW(
        fullPath.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        LOG_ERROR("CreateFileW(file info)");
        return false;
    }

    LARGE_INTEGER size = { 0, };
    if (!GetFileSizeEx(hFile, &size))
    {
        LOG_ERROR("GetFileSizeEx");
        CloseHandle(hFile);
        return false;
    }

    FILETIME createTime = { 0, 0 };
    FILETIME accessTime = { 0, 0 };
    FILETIME writeTime = { 0, 0 };
    if (!GetFileTime(hFile, &createTime, &accessTime, &writeTime))
    {
        LOG_ERROR("GetFileTime");
        CloseHandle(hFile);
        return false;
    }

    record.size = static_cast<ULONGLONG>(size.QuadPart);
    record.lastWriteTime = writeTime;
    CloseHandle(hFile);
    return true;
}

void ScanDirectoryRecursive(
    const std::wstring& repoPath,
    const std::wstring& currentRelative,
    std::vector<FileRecord>& records
)
{
    std::wstring searchRoot = currentRelative.empty()
        ? repoPath
        : JoinPath(repoPath, currentRelative);
    std::wstring pattern = JoinPath(searchRoot, L"*");

    WIN32_FIND_DATAW ffd = { 0, };
    HANDLE hFind = FindFirstFileW(pattern.c_str(), &ffd);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        LOG_ERROR("FindFirstFileW");
        return;
    }

    do
    {
        if (IsDotName(ffd.cFileName))
            continue;

        std::wstring relative = currentRelative.empty()
            ? ffd.cFileName
            : JoinPath(currentRelative, ffd.cFileName);

        if (IsInsideMetaDir(relative))
            continue;

        std::wstring fullPath = JoinPath(repoPath, relative);

        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            ScanDirectoryRecursive(repoPath, relative, records);
            continue;
        }

        FileRecord record;
        record.relativePath = relative;
        if (ReadFileInfo(fullPath, record))
            records.push_back(record);

    } while (FindNextFileW(hFind, &ffd));

    DWORD err = GetLastError();
    if (err != ERROR_NO_MORE_FILES)
        LOG_ERROR("FindNextFileW");

    FindClose(hFind);
}

std::vector<FileRecord> ScanRepository(const std::wstring& repoPath)
{
    std::vector<FileRecord> records;
    if (!DirectoryExists(repoPath))
    {
        std::wcout << L"Repository path does not exist.\n";
        return records;
    }

    ScanDirectoryRecursive(repoPath, L"", records);
    std::sort(records.begin(), records.end(),
        [](const FileRecord& a, const FileRecord& b)
        {
            return a.relativePath < b.relativePath;
        });
    return records;
}

bool SaveSnapshot(const std::wstring& repoPath)
{
    if (!InitRepository(repoPath))
        return false;

    std::vector<FileRecord> records = ScanRepository(repoPath);
    std::wstring snapshotPath = JoinPath(JoinPath(repoPath, META_DIR), SNAPSHOT_FILE);

    HANDLE hFile = CreateFileW(
        snapshotPath.c_str(),
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        LOG_ERROR("CreateFileW(snapshot write)");
        return false;
    }

    SnapshotHeader header;
    header.count = static_cast<DWORD>(records.size());

    DWORD written = 0;
    BOOL ok = WriteFile(hFile, &header, sizeof(header), &written, NULL);
    if (!ok || written != sizeof(header))
    {
        LOG_ERROR("WriteFile(snapshot header)");
        CloseHandle(hFile);
        return false;
    }

    for (const FileRecord& record : records)
    {
        SnapshotDiskRecord diskRecord;
        wcsncpy_s(diskRecord.relativePath, record.relativePath.c_str(), _TRUNCATE);
        diskRecord.size = record.size;
        diskRecord.lastWriteTime = record.lastWriteTime;

        written = 0;
        ok = WriteFile(hFile, &diskRecord, sizeof(diskRecord), &written, NULL);
        if (!ok || written != sizeof(diskRecord))
        {
            LOG_ERROR("WriteFile(snapshot record)");
            CloseHandle(hFile);
            return false;
        }
    }

    CloseHandle(hFile);
    AppendLog(repoPath, L"snapshot saved");
    std::wcout << L"Snapshot saved. files=" << records.size() << L"\n";
    return true;
}

bool LoadSnapshot(const std::wstring& repoPath, std::vector<FileRecord>& records)
{
    std::wstring snapshotPath = JoinPath(JoinPath(repoPath, META_DIR), SNAPSHOT_FILE);
    HANDLE hFile = CreateFileW(
        snapshotPath.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        std::wcout << L"No snapshot found. Save current status first.\n";
        return false;
    }

    SnapshotHeader header;
    DWORD read = 0;
    BOOL ok = ReadFile(hFile, &header, sizeof(header), &read, NULL);
    if (!ok || read != sizeof(header) ||
        header.magic != SNAPSHOT_MAGIC || header.version != SNAPSHOT_VERSION)
    {
        std::wcout << L"Snapshot file is invalid.\n";
        CloseHandle(hFile);
        return false;
    }

    records.clear();
    for (DWORD i = 0; i < header.count; ++i)
    {
        SnapshotDiskRecord diskRecord;
        read = 0;
        ok = ReadFile(hFile, &diskRecord, sizeof(diskRecord), &read, NULL);
        if (!ok || read != sizeof(diskRecord))
        {
            LOG_ERROR("ReadFile(snapshot record)");
            CloseHandle(hFile);
            return false;
        }

        FileRecord record;
        record.relativePath = diskRecord.relativePath;
        record.size = diskRecord.size;
        record.lastWriteTime = diskRecord.lastWriteTime;
        records.push_back(record);
    }

    CloseHandle(hFile);
    return true;
}

bool SameWriteTime(const FILETIME& a, const FILETIME& b)
{
    return a.dwLowDateTime == b.dwLowDateTime &&
        a.dwHighDateTime == b.dwHighDateTime;
}

void PrintStatus(const std::wstring& repoPath)
{
    std::vector<FileRecord> oldRecords;
    if (!LoadSnapshot(repoPath, oldRecords))
        return;

    std::vector<FileRecord> currentRecords = ScanRepository(repoPath);
    std::map<std::wstring, FileRecord> oldMap;
    std::map<std::wstring, FileRecord> currentMap;

    for (const FileRecord& record : oldRecords)
        oldMap[record.relativePath] = record;
    for (const FileRecord& record : currentRecords)
        currentMap[record.relativePath] = record;

    int changes = 0;
    for (const auto& item : currentMap)
    {
        auto found = oldMap.find(item.first);
        if (found == oldMap.end())
        {
            std::wcout << L"[NEW]      " << item.first << L"\n";
            ++changes;
            continue;
        }

        const FileRecord& oldRecord = found->second;
        const FileRecord& nowRecord = item.second;
        if (oldRecord.size != nowRecord.size ||
            !SameWriteTime(oldRecord.lastWriteTime, nowRecord.lastWriteTime))
        {
            std::wcout << L"[MODIFIED] " << item.first << L"\n";
            ++changes;
        }
    }

    for (const auto& item : oldMap)
    {
        if (currentMap.find(item.first) == currentMap.end())
        {
            std::wcout << L"[DELETED]  " << item.first << L"\n";
            ++changes;
        }
    }

    if (changes == 0)
        std::wcout << L"No changes.\n";

    AppendLog(repoPath, L"status checked");
}

void ListFiles(const std::wstring& repoPath)
{
    std::vector<FileRecord> records = ScanRepository(repoPath);
    if (records.empty())
    {
        std::wcout << L"No files found.\n";
        return;
    }

    for (const FileRecord& record : records)
        std::wcout << L"- " << record.relativePath << L" (" << record.size << L" bytes)\n";
}

std::wstring ActionName(DWORD action)
{
    switch (action)
    {
    case FILE_ACTION_ADDED:
        return L"ADDED";
    case FILE_ACTION_REMOVED:
        return L"REMOVED";
    case FILE_ACTION_MODIFIED:
        return L"MODIFIED";
    case FILE_ACTION_RENAMED_OLD_NAME:
        return L"RENAMED_OLD";
    case FILE_ACTION_RENAMED_NEW_NAME:
        return L"RENAMED_NEW";
    default:
        return L"UNKNOWN";
    }
}

DWORD WINAPI WatchThreadProc(LPVOID param)
{
    WatchContext* ctx = static_cast<WatchContext*>(param);
    HANDLE hDir = CreateFileW(
        ctx->repoPath.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL
    );

    if (hDir == INVALID_HANDLE_VALUE)
    {
        LOG_ERROR("CreateFileW(watch directory)");
        delete ctx;
        return 1;
    }

    BYTE buffer[4096] = { 0, };
    DWORD flags = FILE_NOTIFY_CHANGE_FILE_NAME |
        FILE_NOTIFY_CHANGE_DIR_NAME |
        FILE_NOTIFY_CHANGE_SIZE |
        FILE_NOTIFY_CHANGE_LAST_WRITE |
        FILE_NOTIFY_CHANGE_CREATION;

    OVERLAPPED ov = { 0, };
    ov.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (ov.hEvent == NULL)
    {
        LOG_ERROR("CreateEventW(watch overlapped)");
        CloseHandle(hDir);
        delete ctx;
        return 1;
    }

    HANDLE waitHandles[2] = { ctx->stopEvent, ov.hEvent };

    while (true)
    {
        ResetEvent(ov.hEvent);
        DWORD bytesReturned = 0;
        BOOL ok = ReadDirectoryChangesW(
            hDir,
            buffer,
            sizeof(buffer),
            TRUE,
            flags,
            &bytesReturned,
            &ov,
            NULL
        );

        if (!ok && GetLastError() != ERROR_IO_PENDING)
        {
            LOG_ERROR("ReadDirectoryChangesW");
            Sleep(500);
            continue;
        }

        DWORD waitResult = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);
        if (waitResult == WAIT_OBJECT_0)
        {
            CancelIoEx(hDir, &ov);
            break;
        }

        if (waitResult != WAIT_OBJECT_0 + 1)
        {
            LOG_ERROR("WaitForMultipleObjects");
            break;
        }

        if (!GetOverlappedResult(hDir, &ov, &bytesReturned, FALSE))
        {
            if (GetLastError() != ERROR_OPERATION_ABORTED)
                LOG_ERROR("GetOverlappedResult");
            continue;
        }

        if (bytesReturned == 0)
            continue;

        FILE_NOTIFY_INFORMATION* eventInfo =
            reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer);

        while (true)
        {
            std::wstring fileName(
                eventInfo->FileName,
                eventInfo->FileNameLength / sizeof(WCHAR)
            );

            if (!IsInsideMetaDir(fileName))
            {
                std::wstring message = L"[WATCH] " + ActionName(eventInfo->Action) +
                    L" " + fileName;
                std::wcout << message << L"\n";
                AppendLog(ctx->repoPath, message);
            }

            if (eventInfo->NextEntryOffset == 0)
                break;

            eventInfo = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
                reinterpret_cast<BYTE*>(eventInfo) + eventInfo->NextEntryOffset
            );
        }
    }

    CloseHandle(ov.hEvent);
    CloseHandle(hDir);
    delete ctx;
    return 0;
}

bool StartWatch(const std::wstring& repoPath)
{
    if (g_watchThread != NULL)
    {
        std::wcout << L"Watch is already running.\n";
        return false;
    }

    g_stopEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (g_stopEvent == NULL)
    {
        LOG_ERROR("CreateEventW");
        return false;
    }

    WatchContext* ctx = new WatchContext;
    ctx->repoPath = repoPath;
    ctx->stopEvent = g_stopEvent;

    g_watchThread = CreateThread(NULL, 0, WatchThreadProc, ctx, 0, NULL);
    if (g_watchThread == NULL)
    {
        LOG_ERROR("CreateThread");
        CloseHandle(g_stopEvent);
        g_stopEvent = NULL;
        delete ctx;
        return false;
    }

    AppendLog(repoPath, L"watch started");
    std::wcout << L"Watch started.\n";
    return true;
}

void StopWatch()
{
    if (g_watchThread == NULL)
    {
        std::wcout << L"Watch is not running.\n";
        return;
    }

    SetEvent(g_stopEvent);
    WaitForSingleObject(g_watchThread, INFINITE);
    CloseHandle(g_watchThread);
    CloseHandle(g_stopEvent);
    g_watchThread = NULL;
    g_stopEvent = NULL;
    AppendLog(g_repoPath, L"watch stopped");
    std::wcout << L"Watch stopped.\n";
}

void ShowLog(const std::wstring& repoPath)
{
    std::wstring logPath = JoinPath(JoinPath(repoPath, META_DIR), CHANGE_LOG_FILE);
    HANDLE hFile = CreateFileW(
        logPath.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        std::wcout << L"No log file found.\n";
        return;
    }

    LARGE_INTEGER size = { 0, };
    if (!GetFileSizeEx(hFile, &size) || size.QuadPart == 0)
    {
        CloseHandle(hFile);
        std::wcout << L"Log is empty.\n";
        return;
    }

    if (size.QuadPart > 1024 * 1024)
    {
        CloseHandle(hFile);
        std::wcout << L"Log is too large to display.\n";
        return;
    }

    std::vector<wchar_t> buffer(static_cast<size_t>(size.QuadPart / sizeof(wchar_t)) + 1);
    DWORD read = 0;
    BOOL ok = ReadFile(hFile, buffer.data(), static_cast<DWORD>(size.QuadPart), &read, NULL);
    CloseHandle(hFile);

    if (!ok)
    {
        LOG_ERROR("ReadFile(log)");
        return;
    }

    std::wcout << buffer.data() << L"\n";
}

void PrintMenu()
{
    std::wcout << L"\n==== MiniRepo Tracker ====\n";
    std::wcout << L"Repository: " << (g_repoPath.empty() ? L"(not selected)" : g_repoPath) << L"\n";
    std::wcout << L"1. Select/Create repository\n";
    std::wcout << L"2. List files\n";
    std::wcout << L"3. Save snapshot\n";
    std::wcout << L"4. Check status\n";
    std::wcout << L"5. Start realtime watch\n";
    std::wcout << L"6. Stop realtime watch\n";
    std::wcout << L"7. Show change log\n";
    std::wcout << L"0. Exit\n";
    std::wcout << L"> ";
}

bool NeedRepository()
{
    if (g_repoPath.empty())
    {
        std::wcout << L"Select repository first.\n";
        return false;
    }
    return true;
}

int wmain()
{
    setlocale(LC_ALL, "");

    while (true)
    {
        PrintMenu();

        int menu = -1;
        std::wcin >> menu;
        if (!std::wcin)
        {
            std::wcin.clear();
            std::wcin.ignore(1024, L'\n');
            std::wcout << L"Invalid input.\n";
            continue;
        }
        std::wcin.ignore(1024, L'\n');

        if (menu == 0)
        {
            if (g_watchThread != NULL)
                StopWatch();
            break;
        }

        switch (menu)
        {
        case 1:
        {
            std::wcout << L"Repository path: ";
            std::getline(std::wcin, g_repoPath);
            g_repoPath = Trim(g_repoPath);
            if (g_repoPath.empty())
            {
                std::wcout << L"Path is empty.\n";
                break;
            }
            InitRepository(g_repoPath);
            break;
        }
        case 2:
            if (NeedRepository())
                ListFiles(g_repoPath);
            break;
        case 3:
            if (NeedRepository())
                SaveSnapshot(g_repoPath);
            break;
        case 4:
            if (NeedRepository())
                PrintStatus(g_repoPath);
            break;
        case 5:
            if (NeedRepository())
                StartWatch(g_repoPath);
            break;
        case 6:
            StopWatch();
            break;
        case 7:
            if (NeedRepository())
                ShowLog(g_repoPath);
            break;
        default:
            std::wcout << L"Unknown menu.\n";
            break;
        }
    }

    std::wcout << L"Bye.\n";
    return 0;
}
