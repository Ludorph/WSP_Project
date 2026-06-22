#pragma once

#include <Windows.h>

#define MAX_REPO_FILES      128
#define SNAPSHOT_MAGIC      0x4C525031
#define META_FOLDER_NAME    L".minirepo"
#define SNAPSHOT_FILE_NAME  L"snapshot.dat"

// 파일 하나의 상태를 저장하는 구조체입니다.
struct FileRecord
{
    WCHAR name[MAX_PATH];
    ULONGLONG size;
    FILETIME writeTime;
};

// snapshot.dat 앞부분에 저장해서 파일 형식을 확인합니다.
struct SnapshotHeader
{
    DWORD magic;
    DWORD count;
};

// 리포지터리 폴더 관리 기능을 모아 둔 클래스입니다.
class RepoManager
{
public:
    RepoManager();

    BOOL Init(const WCHAR* path);
    void ListFiles();
    BOOL CreateFolder(const WCHAR* folderName);
    BOOL SaveSnapshot();
    void CheckStatus();

private:
    // 사용자가 입력한 경로, 내부 폴더, 스냅샷 파일 경로입니다.
    WCHAR repoPath[MAX_PATH];
    WCHAR metaPath[MAX_PATH];
    WCHAR snapshotPath[MAX_PATH];

    // 내부에서만 사용하는 보조 함수들입니다.
    BOOL MakePath(WCHAR* outPath, DWORD outCount, const WCHAR* left, const WCHAR* right);
    BOOL ScanFiles(FileRecord files[], DWORD maxCount, DWORD* outCount);
    BOOL GetOneFileInfo(const WCHAR* fileName, FileRecord* outRecord);
    BOOL WriteSnapshot(FileRecord files[], DWORD count);
    BOOL ReadSnapshot(FileRecord files[], DWORD maxCount, DWORD* outCount);
    int FindFile(FileRecord files[], DWORD count, const WCHAR* name);
    BOOL IsSameTime(const FILETIME* left, const FILETIME* right);
    void PrintLastError(const char* apiName);
};
