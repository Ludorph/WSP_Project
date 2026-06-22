#pragma once

#include <Windows.h>

#include <string>
#include <vector>

// 파일 하나를 비교하기 위해 필요한 최소 정보입니다.
// 파일 이름, 크기, 마지막 수정 시간이 스냅샷 저장/비교 기준이 됩니다.
struct FileInfo
{
    std::wstring name;
    ULONGLONG size;
    FILETIME writeTime;
};

// 리포지터리 폴더를 관리하는 간단한 클래스입니다.
// main.cpp는 메뉴만 담당하고, 실제 WinAPI 파일 처리는 이 클래스에서 수행합니다.
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
    // rootPath: 사용자가 입력한 리포지터리 경로
    // metaPath: 내부 관리 폴더(.minirepo)
    // snapshotPath: 기준 상태를 저장하는 스냅샷 파일
    std::wstring rootPath;
    std::wstring metaPath;
    std::wstring snapshotPath;

    // 아래 함수들은 메뉴에서 직접 호출하지 않는 내부 보조 함수입니다.
    std::wstring JoinPath(const std::wstring& left, const std::wstring& right) const;
    bool ScanFiles(std::vector<FileInfo>& files);
    bool GetFileInfo(const std::wstring& fileName, FileInfo& info);
    bool WriteTextFile(const std::wstring& path, const std::wstring& text);
    bool ReadTextFile(const std::wstring& path, std::wstring& text);
    bool LoadSnapshot(std::vector<FileInfo>& files);
    void PrintLastError(const char* apiName);
};
