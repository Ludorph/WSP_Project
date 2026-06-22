#include "RepoManager.h"

#include <stdio.h>
#include <string.h>
#include <wchar.h>

RepoManager::RepoManager()
{
    // WCHAR 배열은 사용 전에 0으로 초기화합니다.
    ZeroMemory(repoPath, sizeof(repoPath));
    ZeroMemory(metaPath, sizeof(metaPath));
    ZeroMemory(snapshotPath, sizeof(snapshotPath));
}

BOOL RepoManager::Init(const WCHAR* path)
{
    // 리포지터리 폴더와 내부 관리 폴더를 준비합니다.
    if (path == NULL || path[0] == L'\0')
    {
        wprintf(L"Repository path is empty.\n");
        return FALSE;
    }

    wcscpy_s(repoPath, path);

    if (FALSE == MakePath(metaPath, MAX_PATH, repoPath, META_FOLDER_NAME))
        return FALSE;

    if (FALSE == MakePath(snapshotPath, MAX_PATH, metaPath, SNAPSHOT_FILE_NAME))
        return FALSE;

    // 폴더가 이미 있으면 정상 상황으로 처리합니다.
    if (FALSE == CreateDirectoryW(repoPath, NULL))
    {
        DWORD error = GetLastError();
        if (ERROR_ALREADY_EXISTS != error)
        {
            PrintLastError("CreateDirectoryW(repo)");
            return FALSE;
        }
    }

    if (FALSE == CreateDirectoryW(metaPath, NULL))
    {
        DWORD error = GetLastError();
        if (ERROR_ALREADY_EXISTS != error)
        {
            PrintLastError("CreateDirectoryW(.minirepo)");
            return FALSE;
        }
    }

    wprintf(L"Repository ready: %s\n", repoPath);
    return TRUE;
}

void RepoManager::ListFiles()
{
    // 수업 코드 스타일에 맞춰 vector 대신 고정 배열을 사용합니다.
    FileRecord files[MAX_REPO_FILES] = { 0, };
    DWORD count = 0;

    if (FALSE == ScanFiles(files, MAX_REPO_FILES, &count))
        return;

    if (0 == count)
    {
        wprintf(L"No files.\n");
        return;
    }

    for (DWORD i = 0; i < count; i++)
        wprintf(L"- %s (%llu bytes)\n", files[i].name, files[i].size);
}

BOOL RepoManager::CreateFolder(const WCHAR* folderName)
{
    // 복잡한 경로 대신 단순 폴더명만 받습니다.
    if (folderName == NULL || folderName[0] == L'\0')
    {
        wprintf(L"Folder name is empty.\n");
        return FALSE;
    }

    if (wcschr(folderName, L'\\') != NULL || wcschr(folderName, L'/') != NULL)
    {
        wprintf(L"Use a simple folder name only.\n");
        return FALSE;
    }

    WCHAR newPath[MAX_PATH] = { 0, };
    if (FALSE == MakePath(newPath, MAX_PATH, repoPath, folderName))
        return FALSE;

    // CreateDirectoryW로 리포지터리 안에 새 폴더를 만듭니다.
    if (FALSE == CreateDirectoryW(newPath, NULL))
    {
        DWORD error = GetLastError();
        if (ERROR_ALREADY_EXISTS == error)
        {
            wprintf(L"Folder already exists.\n");
            return FALSE;
        }

        PrintLastError("CreateDirectoryW(folder)");
        return FALSE;
    }

    wprintf(L"Folder created: %s\n", folderName);
    return TRUE;
}

BOOL RepoManager::SaveSnapshot()
{
    // 현재 파일 목록을 읽어서 snapshot.dat에 저장합니다.
    FileRecord files[MAX_REPO_FILES] = { 0, };
    DWORD count = 0;

    if (FALSE == ScanFiles(files, MAX_REPO_FILES, &count))
        return FALSE;

    if (FALSE == WriteSnapshot(files, count))
        return FALSE;

    wprintf(L"Snapshot saved. files=%u\n", count);
    return TRUE;
}

void RepoManager::CheckStatus()
{
    // oldFiles는 저장된 기준 상태, nowFiles는 현재 상태입니다.
    FileRecord oldFiles[MAX_REPO_FILES] = { 0, };
    FileRecord nowFiles[MAX_REPO_FILES] = { 0, };
    DWORD oldCount = 0;
    DWORD nowCount = 0;
    DWORD changeCount = 0;

    if (FALSE == ReadSnapshot(oldFiles, MAX_REPO_FILES, &oldCount))
        return;

    if (FALSE == ScanFiles(nowFiles, MAX_REPO_FILES, &nowCount))
        return;

    for (DWORD i = 0; i < nowCount; i++)
    {
        // 현재 파일이 과거 스냅샷에 없으면 새 파일입니다.
        int oldIndex = FindFile(oldFiles, oldCount, nowFiles[i].name);
        if (oldIndex < 0)
        {
            wprintf(L"[NEW]      %s\n", nowFiles[i].name);
            changeCount++;
            continue;
        }

        if (oldFiles[oldIndex].size != nowFiles[i].size ||
            FALSE == IsSameTime(&oldFiles[oldIndex].writeTime, &nowFiles[i].writeTime))
        {
            wprintf(L"[MODIFIED] %s\n", nowFiles[i].name);
            changeCount++;
        }
    }

    for (DWORD i = 0; i < oldCount; i++)
    {
        // 과거에는 있었지만 현재 없으면 삭제된 파일입니다.
        if (FindFile(nowFiles, nowCount, oldFiles[i].name) < 0)
        {
            wprintf(L"[DELETED]  %s\n", oldFiles[i].name);
            changeCount++;
        }
    }

    if (0 == changeCount)
        wprintf(L"No changes.\n");
}

BOOL RepoManager::MakePath(WCHAR* outPath, DWORD outCount, const WCHAR* left, const WCHAR* right)
{
    // 경로 끝의 '\' 유무를 확인해서 중복 없이 이어 붙입니다.
    if (outPath == NULL || left == NULL || right == NULL)
        return FALSE;

    size_t leftLen = wcslen(left);
    if (0 == leftLen)
        return FALSE;

    if (left[leftLen - 1] == L'\\' || left[leftLen - 1] == L'/')
        swprintf_s(outPath, outCount, L"%s%s", left, right);
    else
        swprintf_s(outPath, outCount, L"%s\\%s", left, right);

    return TRUE;
}

BOOL RepoManager::ScanFiles(FileRecord files[], DWORD maxCount, DWORD* outCount)
{
    // FindFirstFileW / FindNextFileW로 폴더의 파일들을 탐색합니다.
    if (files == NULL || outCount == NULL)
        return FALSE;

    *outCount = 0;

    WCHAR searchPath[MAX_PATH] = { 0, };
    if (FALSE == MakePath(searchPath, MAX_PATH, repoPath, L"*"))
        return FALSE;

    WIN32_FIND_DATAW findData = { 0, };
    HANDLE hFind = FindFirstFileW(searchPath, &findData);
    if (INVALID_HANDLE_VALUE == hFind)
    {
        PrintLastError("FindFirstFileW");
        return FALSE;
    }

    do
    {
        // 현재 폴더, 상위 폴더, 내부 관리 폴더는 제외합니다.
        if (0 == wcscmp(findData.cFileName, L".") ||
            0 == wcscmp(findData.cFileName, L"..") ||
            0 == wcscmp(findData.cFileName, META_FOLDER_NAME))
        {
            continue;
        }

        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;

        // 고정 배열 범위를 넘지 않도록 개수를 제한합니다.
        if (*outCount >= maxCount)
        {
            wprintf(L"Too many files. Max count is %u.\n", maxCount);
            break;
        }

        if (TRUE == GetOneFileInfo(findData.cFileName, &files[*outCount]))
            (*outCount)++;

    } while (FindNextFileW(hFind, &findData));

    DWORD error = GetLastError();
    if (ERROR_NO_MORE_FILES != error)
        PrintLastError("FindNextFileW");

    FindClose(hFind);
    return TRUE;
}

BOOL RepoManager::GetOneFileInfo(const WCHAR* fileName, FileRecord* outRecord)
{
    // 파일 하나를 열어서 크기와 마지막 수정 시간을 얻습니다.
    if (fileName == NULL || outRecord == NULL)
        return FALSE;

    WCHAR fullPath[MAX_PATH] = { 0, };
    if (FALSE == MakePath(fullPath, MAX_PATH, repoPath, fileName))
        return FALSE;

    HANDLE hFile = CreateFileW(
        fullPath,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (INVALID_HANDLE_VALUE == hFile)
    {
        PrintLastError("CreateFileW(file)");
        return FALSE;
    }

    LARGE_INTEGER fileSize = { 0, };
    if (FALSE == GetFileSizeEx(hFile, &fileSize))
    {
        PrintLastError("GetFileSizeEx");
        CloseHandle(hFile);
        return FALSE;
    }

    FILETIME createTime = { 0, };
    FILETIME accessTime = { 0, };
    FILETIME writeTime = { 0, };
    // create/access/write 중 writeTime만 변경 여부 비교에 사용합니다.
    if (FALSE == GetFileTime(hFile, &createTime, &accessTime, &writeTime))
    {
        PrintLastError("GetFileTime");
        CloseHandle(hFile);
        return FALSE;
    }

    CloseHandle(hFile);

    ZeroMemory(outRecord, sizeof(FileRecord));
    wcscpy_s(outRecord->name, fileName);
    outRecord->size = static_cast<ULONGLONG>(fileSize.QuadPart);
    outRecord->writeTime = writeTime;
    return TRUE;
}

BOOL RepoManager::WriteSnapshot(FileRecord files[], DWORD count)
{
    // 구조체를 그대로 WriteFile로 저장합니다.
    HANDLE hFile = CreateFileW(
        snapshotPath,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (INVALID_HANDLE_VALUE == hFile)
    {
        PrintLastError("CreateFileW(snapshot write)");
        return FALSE;
    }

    SnapshotHeader header = { 0, };
    header.magic = SNAPSHOT_MAGIC;
    header.count = count;

    // 먼저 헤더를 쓰고, 그 뒤에 파일 정보 배열을 씁니다.
    DWORD written = 0;
    if (FALSE == WriteFile(hFile, &header, sizeof(header), &written, NULL) ||
        written != sizeof(header))
    {
        PrintLastError("WriteFile(snapshot header)");
        CloseHandle(hFile);
        return FALSE;
    }

    for (DWORD i = 0; i < count; i++)
    {
        written = 0;
        if (FALSE == WriteFile(hFile, &files[i], sizeof(FileRecord), &written, NULL) ||
            written != sizeof(FileRecord))
        {
            PrintLastError("WriteFile(snapshot record)");
            CloseHandle(hFile);
            return FALSE;
        }
    }

    CloseHandle(hFile);
    return TRUE;
}

BOOL RepoManager::ReadSnapshot(FileRecord files[], DWORD maxCount, DWORD* outCount)
{
    // snapshot.dat를 읽어서 FileRecord 배열로 복원합니다.
    if (files == NULL || outCount == NULL)
        return FALSE;

    *outCount = 0;

    HANDLE hFile = CreateFileW(
        snapshotPath,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (INVALID_HANDLE_VALUE == hFile)
    {
        wprintf(L"Snapshot does not exist. Save snapshot first.\n");
        return FALSE;
    }

    SnapshotHeader header = { 0, };
    DWORD readSize = 0;
    if (FALSE == ReadFile(hFile, &header, sizeof(header), &readSize, NULL) ||
        readSize != sizeof(header))
    {
        PrintLastError("ReadFile(snapshot header)");
        CloseHandle(hFile);
        return FALSE;
    }

    if (SNAPSHOT_MAGIC != header.magic || header.count > maxCount)
    {
        // magic 값이나 개수가 이상하면 잘못된 스냅샷으로 판단합니다.
        wprintf(L"Snapshot file format is invalid.\n");
        CloseHandle(hFile);
        return FALSE;
    }

    for (DWORD i = 0; i < header.count; i++)
    {
        readSize = 0;
        if (FALSE == ReadFile(hFile, &files[i], sizeof(FileRecord), &readSize, NULL) ||
            readSize != sizeof(FileRecord))
        {
            PrintLastError("ReadFile(snapshot record)");
            CloseHandle(hFile);
            return FALSE;
        }
    }

    CloseHandle(hFile);
    *outCount = header.count;
    return TRUE;
}

int RepoManager::FindFile(FileRecord files[], DWORD count, const WCHAR* name)
{
    // map 대신 배열을 직접 순회하며 파일 이름을 찾습니다.
    if (files == NULL || name == NULL)
        return -1;

    for (DWORD i = 0; i < count; i++)
    {
        if (0 == wcscmp(files[i].name, name))
            return static_cast<int>(i);
    }

    return -1;
}

BOOL RepoManager::IsSameTime(const FILETIME* left, const FILETIME* right)
{
    // FILETIME은 Low/High 두 값이 모두 같아야 같은 시간입니다.
    if (left == NULL || right == NULL)
        return FALSE;

    return left->dwLowDateTime == right->dwLowDateTime &&
        left->dwHighDateTime == right->dwHighDateTime;
}

void RepoManager::PrintLastError(const char* apiName)
{
    // WinAPI 실패 시 마지막 오류 코드를 출력합니다.
    printf("[ERROR] %s failed. code=%u\n", apiName, GetLastError());
}
