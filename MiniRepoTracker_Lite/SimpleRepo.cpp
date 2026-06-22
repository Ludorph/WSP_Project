#include "SimpleRepo.h"

#include <algorithm>
#include <cwchar>
#include <iostream>
#include <map>
#include <sstream>

SimpleRepo::SimpleRepo(const std::wstring& rootPathValue)
    : rootPath(rootPathValue)
{
    // Git의 .git 폴더처럼, 프로그램 내부 정보를 보관할 폴더를 따로 둡니다.
    metaPath = JoinPath(rootPath, L".minirepo");
    snapshotPath = JoinPath(metaPath, L"snapshot.txt");
}

bool SimpleRepo::Init()
{
    // 사용자가 입력한 리포지터리 폴더를 준비합니다.
    // 이미 존재하는 폴더는 오류가 아니라 정상 상황으로 처리합니다.
    if (!CreateDirectoryW(rootPath.c_str(), NULL))
    {
        DWORD error = GetLastError();
        if (error != ERROR_ALREADY_EXISTS)
        {
            PrintLastError("CreateDirectoryW(root)");
            return false;
        }
    }

    // snapshot.txt를 저장할 내부 관리 폴더입니다.
    if (!CreateDirectoryW(metaPath.c_str(), NULL))
    {
        DWORD error = GetLastError();
        if (error != ERROR_ALREADY_EXISTS)
        {
            PrintLastError("CreateDirectoryW(.minirepo)");
            return false;
        }
    }

    std::wcout << L"Repository ready: " << rootPath << L"\n";
    return true;
}

void SimpleRepo::ListFiles()
{
    // 파일 탐색 결과는 vector에 모아서 출력합니다.
    std::vector<FileInfo> files;
    if (!ScanFiles(files))
        return;

    if (files.empty())
    {
        std::wcout << L"No files.\n";
        return;
    }

    for (const FileInfo& file : files)
        std::wcout << L"- " << file.name << L" (" << file.size << L" bytes)\n";
}

bool SimpleRepo::CreateFolder(const std::wstring& folderName)
{
    // 시연과 오류 방지를 위해 복잡한 경로가 아닌 단순 폴더명만 허용합니다.
    if (folderName.empty())
    {
        std::wcout << L"Folder name is empty.\n";
        return false;
    }

    if (folderName.find(L'\\') != std::wstring::npos ||
        folderName.find(L'/') != std::wstring::npos)
    {
        std::wcout << L"Use a simple folder name only.\n";
        return false;
    }

    std::wstring newPath = JoinPath(rootPath, folderName);
    // 수업에서 배운 CreateDirectoryW를 사용해 리포지터리 안에 폴더를 만듭니다.
    if (!CreateDirectoryW(newPath.c_str(), NULL))
    {
        DWORD error = GetLastError();
        if (error == ERROR_ALREADY_EXISTS)
        {
            std::wcout << L"Folder already exists.\n";
            return false;
        }

        PrintLastError("CreateDirectoryW(new folder)");
        return false;
    }

    std::wcout << L"Folder created: " << folderName << L"\n";
    return true;
}

bool SimpleRepo::SaveSnapshot()
{
    // 현재 파일 상태를 읽어 와서 이후 비교 기준으로 저장합니다.
    std::vector<FileInfo> files;
    if (!ScanFiles(files))
        return false;

    std::wstringstream ss;
    for (const FileInfo& file : files)
    {
        // 저장 형식: 파일이름|파일크기|수정시간Low|수정시간High
        // FILETIME은 Low/High 두 DWORD로 이루어져 있어 두 값을 모두 저장합니다.
        ss << file.name << L"|"
            << file.size << L"|"
            << file.writeTime.dwLowDateTime << L"|"
            << file.writeTime.dwHighDateTime << L"\n";
    }

    if (!WriteTextFile(snapshotPath, ss.str()))
        return false;

    std::wcout << L"Snapshot saved. files=" << files.size() << L"\n";
    return true;
}

void SimpleRepo::ShowStatus()
{
    // oldFiles는 저장된 기준 상태, nowFiles는 현재 폴더 상태입니다.
    std::vector<FileInfo> oldFiles;
    if (!LoadSnapshot(oldFiles))
        return;

    std::vector<FileInfo> nowFiles;
    if (!ScanFiles(nowFiles))
        return;

    std::map<std::wstring, FileInfo> oldMap;
    std::map<std::wstring, FileInfo> nowMap;

    // 파일 이름을 key로 두면 NEW/MODIFIED/DELETED 비교가 단순해집니다.
    for (const FileInfo& file : oldFiles)
        oldMap[file.name] = file;

    for (const FileInfo& file : nowFiles)
        nowMap[file.name] = file;

    int changeCount = 0;

    for (const auto& item : nowMap)
    {
        auto oldItem = oldMap.find(item.first);
        if (oldItem == oldMap.end())
        {
            // 현재에는 있는데 스냅샷에는 없으면 새 파일입니다.
            std::wcout << L"[NEW]      " << item.first << L"\n";
            ++changeCount;
            continue;
        }

        const FileInfo& oldFile = oldItem->second;
        const FileInfo& nowFile = item.second;

        bool sizeChanged = oldFile.size != nowFile.size;
        bool timeChanged =
            oldFile.writeTime.dwLowDateTime != nowFile.writeTime.dwLowDateTime ||
            oldFile.writeTime.dwHighDateTime != nowFile.writeTime.dwHighDateTime;

        // 파일 내용 전체 대신 크기와 마지막 수정 시간으로 수정 여부를 판단합니다.
        if (sizeChanged || timeChanged)
        {
            std::wcout << L"[MODIFIED] " << item.first << L"\n";
            ++changeCount;
        }
    }

    for (const auto& item : oldMap)
    {
        if (nowMap.find(item.first) == nowMap.end())
        {
            // 스냅샷에는 있는데 현재 없으면 삭제된 파일입니다.
            std::wcout << L"[DELETED]  " << item.first << L"\n";
            ++changeCount;
        }
    }

    if (changeCount == 0)
        std::wcout << L"No changes.\n";
}

std::wstring SimpleRepo::JoinPath(const std::wstring& left, const std::wstring& right) const
{
    // 경로 끝에 '\'가 이미 있으면 중복해서 붙이지 않습니다.
    if (left.empty())
        return right;
    if (left.back() == L'\\' || left.back() == L'/')
        return left + right;
    return left + L"\\" + right;
}

bool SimpleRepo::ScanFiles(std::vector<FileInfo>& files)
{
    files.clear();

    // WIN32_FIND_DATAW는 FindFirstFileW/FindNextFileW가 파일 정보를 채워 주는 구조체입니다.
    WIN32_FIND_DATAW findData = { 0, };
    std::wstring searchPath = JoinPath(rootPath, L"*");

    // '*' 패턴으로 리포지터리 폴더의 모든 항목을 탐색합니다.
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        PrintLastError("FindFirstFileW");
        return false;
    }

    do
    {
        // '.', '..'은 실제 파일이 아니고, .minirepo는 내부 관리 폴더라 비교 대상에서 제외합니다.
        if (wcscmp(findData.cFileName, L".") == 0 ||
            wcscmp(findData.cFileName, L"..") == 0 ||
            wcscmp(findData.cFileName, L".minirepo") == 0)
        {
            continue;
        }

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        // 찾은 파일의 크기와 마지막 수정 시간을 따로 읽어 FileInfo로 저장합니다.
        FileInfo info = { };
        if (GetFileInfo(findData.cFileName, info))
            files.push_back(info);

    } while (FindNextFileW(hFind, &findData));

    DWORD error = GetLastError();
    if (error != ERROR_NO_MORE_FILES)
        PrintLastError("FindNextFileW");

    FindClose(hFind);

    // 출력과 스냅샷 저장 순서를 일정하게 하기 위해 이름순으로 정렬합니다.
    std::sort(files.begin(), files.end(),
        [](const FileInfo& a, const FileInfo& b)
        {
            return a.name < b.name;
        });

    return true;
}

bool SimpleRepo::GetFileInfo(const std::wstring& fileName, FileInfo& info)
{
    std::wstring fullPath = JoinPath(rootPath, fileName);

    // 파일 크기와 시간 정보를 얻기 위해 읽기 권한으로 파일을 엽니다.
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
        PrintLastError("CreateFileW(file)");
        return false;
    }

    // 파일 크기는 64비트 값을 담을 수 있는 LARGE_INTEGER로 받습니다.
    LARGE_INTEGER fileSize = { 0, };
    if (!GetFileSizeEx(hFile, &fileSize))
    {
        PrintLastError("GetFileSizeEx");
        CloseHandle(hFile);
        return false;
    }

    FILETIME createTime = { 0, };
    FILETIME accessTime = { 0, };
    FILETIME writeTime = { 0, };

    // create/access/write 시간 중 이 프로젝트는 마지막 수정 시간(writeTime)을 비교에 사용합니다.
    if (!GetFileTime(hFile, &createTime, &accessTime, &writeTime))
    {
        PrintLastError("GetFileTime");
        CloseHandle(hFile);
        return false;
    }

    // CreateFileW로 얻은 핸들은 반드시 CloseHandle로 닫아야 합니다.
    CloseHandle(hFile);

    info.name = fileName;
    info.size = static_cast<ULONGLONG>(fileSize.QuadPart);
    info.writeTime = writeTime;
    return true;
}

bool SimpleRepo::WriteTextFile(const std::wstring& path, const std::wstring& text)
{
    // CREATE_ALWAYS는 기존 스냅샷을 현재 상태로 덮어쓰기 위해 사용합니다.
    HANDLE hFile = CreateFileW(
        path.c_str(),
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        PrintLastError("CreateFileW(write)");
        return false;
    }

    // wstring은 wchar_t 단위이므로 문자 수를 바이트 수로 변환해서 WriteFile에 전달합니다.
    DWORD writeSize = static_cast<DWORD>(text.size() * sizeof(wchar_t));
    DWORD written = 0;
    BOOL result = WriteFile(hFile, text.c_str(), writeSize, &written, NULL);
    CloseHandle(hFile);

    if (!result || written != writeSize)
    {
        PrintLastError("WriteFile");
        return false;
    }

    return true;
}

bool SimpleRepo::ReadTextFile(const std::wstring& path, std::wstring& text)
{
    // 저장된 snapshot.txt를 읽기 전용으로 엽니다.
    HANDLE hFile = CreateFileW(
        path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE)
    {
        std::wcout << L"Snapshot does not exist. Save snapshot first.\n";
        return false;
    }

    // 파일 크기를 먼저 알아야 그만큼의 버퍼를 준비할 수 있습니다.
    LARGE_INTEGER fileSize = { 0, };
    if (!GetFileSizeEx(hFile, &fileSize))
    {
        PrintLastError("GetFileSizeEx(snapshot)");
        CloseHandle(hFile);
        return false;
    }

    if (fileSize.QuadPart == 0)
    {
        text.clear();
        CloseHandle(hFile);
        return true;
    }

    // 과제용 스냅샷 파일은 작아야 하므로 비정상적으로 큰 파일은 읽지 않습니다.
    if (fileSize.QuadPart > 1024 * 1024)
    {
        std::wcout << L"Snapshot file is too large.\n";
        CloseHandle(hFile);
        return false;
    }

    // wstring으로 저장한 파일이므로 파일 크기는 wchar_t 크기의 배수여야 합니다.
    if ((fileSize.QuadPart % sizeof(wchar_t)) != 0)
    {
        std::wcout << L"Snapshot file format is invalid.\n";
        CloseHandle(hFile);
        return false;
    }

    std::vector<wchar_t> buffer(static_cast<size_t>(fileSize.QuadPart / sizeof(wchar_t)) + 1);

    // ReadFile로 snapshot.txt 전체를 읽어 문자열로 복원합니다.
    DWORD readSize = 0;
    BOOL result = ReadFile(hFile, buffer.data(), static_cast<DWORD>(fileSize.QuadPart), &readSize, NULL);
    CloseHandle(hFile);

    if (!result)
    {
        PrintLastError("ReadFile");
        return false;
    }

    // 문자열 끝을 표시하기 위해 널 문자를 직접 넣습니다.
    buffer[readSize / sizeof(wchar_t)] = L'\0';
    text = buffer.data();
    return true;
}

bool SimpleRepo::LoadSnapshot(std::vector<FileInfo>& files)
{
    files.clear();

    // snapshot.txt 내용을 읽은 뒤 한 줄씩 FileInfo로 변환합니다.
    std::wstring text;
    if (!ReadTextFile(snapshotPath, text))
        return false;

    std::wistringstream input(text);
    std::wstring line;

    while (std::getline(input, line))
    {
        if (line.empty())
            continue;

        // 저장 형식은 파일이름|크기|수정시간Low|수정시간High 입니다.
        // 구분자가 부족한 줄은 깨진 줄로 보고 건너뜁니다.
        size_t p1 = line.find(L'|');
        if (p1 == std::wstring::npos)
            continue;

        size_t p2 = line.find(L'|', p1 + 1);
        if (p2 == std::wstring::npos)
            continue;

        size_t p3 = line.find(L'|', p2 + 1);
        if (p3 == std::wstring::npos)
            continue;

        FileInfo info = { };
        try
        {
            // 숫자 문자열을 다시 정수로 바꿔 스냅샷 정보를 복원합니다.
            info.name = line.substr(0, p1);
            info.size = std::stoull(line.substr(p1 + 1, p2 - p1 - 1));
            info.writeTime.dwLowDateTime = static_cast<DWORD>(std::stoul(line.substr(p2 + 1, p3 - p2 - 1)));
            info.writeTime.dwHighDateTime = static_cast<DWORD>(std::stoul(line.substr(p3 + 1)));
        }
        catch (...)
        {
            // 사용자가 snapshot.txt를 잘못 수정해도 프로그램이 종료되지 않도록 방어합니다.
            std::wcout << L"Invalid snapshot line skipped.\n";
            continue;
        }

        files.push_back(info);
    }

    return true;
}

void SimpleRepo::PrintLastError(const char* apiName)
{
    // WinAPI 실패 원인을 확인하기 위한 수업 내용 활용입니다.
    std::cout << "[ERROR] " << apiName << " failed. code=" << GetLastError() << "\n";
}
