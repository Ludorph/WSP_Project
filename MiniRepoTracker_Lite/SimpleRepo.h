#pragma once

#include <Windows.h>

#include <string>
#include <vector>

struct FileInfo
{
    std::wstring name;
    ULONGLONG size;
    FILETIME writeTime;
};

class SimpleRepo
{
public:
    explicit SimpleRepo(const std::wstring& rootPath);

    bool Init();
    void ListFiles();
    bool CreateFolder(const std::wstring& folderName);
    bool SaveSnapshot();
    void ShowStatus();

private:
    std::wstring rootPath;
    std::wstring metaPath;
    std::wstring snapshotPath;

    std::wstring JoinPath(const std::wstring& left, const std::wstring& right) const;
    bool ScanFiles(std::vector<FileInfo>& files);
    bool GetFileInfo(const std::wstring& fileName, FileInfo& info);
    bool WriteTextFile(const std::wstring& path, const std::wstring& text);
    bool ReadTextFile(const std::wstring& path, std::wstring& text);
    bool LoadSnapshot(std::vector<FileInfo>& files);
    void PrintLastError(const char* apiName);
};
