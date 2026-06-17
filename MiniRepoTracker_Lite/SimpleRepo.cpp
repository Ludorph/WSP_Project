#include "SimpleRepo.h"

#include <algorithm>
#include <cwchar>
#include <iostream>
#include <map>
#include <sstream>

SimpleRepo::SimpleRepo(const std::wstring& rootPathValue)
    : rootPath(rootPathValue)
{
    metaPath = JoinPath(rootPath, L".minirepo");
    snapshotPath = JoinPath(metaPath, L"snapshot.txt");
}

bool SimpleRepo::Init()
{
    if (!CreateDirectoryW(rootPath.c_str(), NULL))
    {
        DWORD error = GetLastError();
        if (error != ERROR_ALREADY_EXISTS)
        {
            PrintLastError("CreateDirectoryW(root)");
            return false;
        }
    }

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
    std::vector<FileInfo> files;
    if (!ScanFiles(files))
        return false;

    std::wstringstream ss;
    for (const FileInfo& file : files)
    {
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
    std::vector<FileInfo> oldFiles;
    if (!LoadSnapshot(oldFiles))
        return;

    std::vector<FileInfo> nowFiles;
    if (!ScanFiles(nowFiles))
        return;

    std::map<std::wstring, FileInfo> oldMap;
    std::map<std::wstring, FileInfo> nowMap;

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
            std::wcout << L"[DELETED]  " << item.first << L"\n";
            ++changeCount;
        }
    }

    if (changeCount == 0)
        std::wcout << L"No changes.\n";
}

std::wstring SimpleRepo::JoinPath(const std::wstring& left, const std::wstring& right) const
{
    if (left.empty())
        return right;
    if (left.back() == L'\\' || left.back() == L'/')
        return left + right;
    return left + L"\\" + right;
}

bool SimpleRepo::ScanFiles(std::vector<FileInfo>& files)
{
    files.clear();

    WIN32_FIND_DATAW findData = { 0, };
    std::wstring searchPath = JoinPath(rootPath, L"*");

    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        PrintLastError("FindFirstFileW");
        return false;
    }

    do
    {
        if (wcscmp(findData.cFileName, L".") == 0 ||
            wcscmp(findData.cFileName, L"..") == 0 ||
            wcscmp(findData.cFileName, L".minirepo") == 0)
        {
            continue;
        }

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        FileInfo info = { };
        if (GetFileInfo(findData.cFileName, info))
            files.push_back(info);

    } while (FindNextFileW(hFind, &findData));

    DWORD error = GetLastError();
    if (error != ERROR_NO_MORE_FILES)
        PrintLastError("FindNextFileW");

    FindClose(hFind);

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

    if (!GetFileTime(hFile, &createTime, &accessTime, &writeTime))
    {
        PrintLastError("GetFileTime");
        CloseHandle(hFile);
        return false;
    }

    CloseHandle(hFile);

    info.name = fileName;
    info.size = static_cast<ULONGLONG>(fileSize.QuadPart);
    info.writeTime = writeTime;
    return true;
}

bool SimpleRepo::WriteTextFile(const std::wstring& path, const std::wstring& text)
{
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

    if (fileSize.QuadPart > 1024 * 1024)
    {
        std::wcout << L"Snapshot file is too large.\n";
        CloseHandle(hFile);
        return false;
    }

    std::vector<wchar_t> buffer(static_cast<size_t>(fileSize.QuadPart / sizeof(wchar_t)) + 1);

    DWORD readSize = 0;
    BOOL result = ReadFile(hFile, buffer.data(), static_cast<DWORD>(fileSize.QuadPart), &readSize, NULL);
    CloseHandle(hFile);

    if (!result)
    {
        PrintLastError("ReadFile");
        return false;
    }

    buffer[readSize / sizeof(wchar_t)] = L'\0';
    text = buffer.data();
    return true;
}

bool SimpleRepo::LoadSnapshot(std::vector<FileInfo>& files)
{
    files.clear();

    std::wstring text;
    if (!ReadTextFile(snapshotPath, text))
        return false;

    std::wistringstream input(text);
    std::wstring line;

    while (std::getline(input, line))
    {
        if (line.empty())
            continue;

        size_t p1 = line.find(L'|');
        size_t p2 = line.find(L'|', p1 + 1);
        size_t p3 = line.find(L'|', p2 + 1);

        if (p1 == std::wstring::npos ||
            p2 == std::wstring::npos ||
            p3 == std::wstring::npos)
        {
            continue;
        }

        FileInfo info = { };
        info.name = line.substr(0, p1);
        info.size = std::stoull(line.substr(p1 + 1, p2 - p1 - 1));
        info.writeTime.dwLowDateTime = static_cast<DWORD>(std::stoul(line.substr(p2 + 1, p3 - p2 - 1)));
        info.writeTime.dwHighDateTime = static_cast<DWORD>(std::stoul(line.substr(p3 + 1)));

        files.push_back(info);
    }

    return true;
}

void SimpleRepo::PrintLastError(const char* apiName)
{
    std::cout << "[ERROR] " << apiName << " failed. code=" << GetLastError() << "\n";
}
