# MiniRepoTracker_Lite2 코드 설명서

이 문서는 `MiniRepoTracker_Lite2`의 전체 구조와 코드 의도를 설명하는 시연 대비 문서입니다.

`MiniRepoTracker_Lite2`는 기존 `MiniRepoTracker_Lite`와 기능은 같지만, 문법과 스타일을 수업 예제 코드에 더 가깝게 맞춘 버전입니다. `std::vector`, `std::map`, `std::wstring`, `wstringstream` 같은 현대 C++ 스타일을 줄이고, 수업에서 자주 보였던 `WCHAR` 배열, `HANDLE`, `DWORD`, `BOOL`, `FILETIME`, `LARGE_INTEGER`, `ReadFile`, `WriteFile` 중심으로 작성했습니다.

## 1. 프로젝트 한 줄 설명

MiniRepoTracker_Lite2는 Git의 `status`와 `commit` 개념을 단순화해서, 폴더 안의 파일 상태를 저장하고 나중에 변경 여부를 확인하는 Windows 콘솔 프로그램입니다.

## 2. Lite2를 만든 이유

기존 Lite 버전도 기능은 충분하지만, 일부 문법이 수업 코드보다 조금 더 현대 C++에 가깝습니다.

예:

- `std::vector`
- `std::map`
- `std::wstring`
- `std::wstringstream`
- range-based for
- `try-catch`

Lite2는 교수님이 제공한 수업 예제 코드와 더 비슷하게 보이도록 다음 방식으로 바꿨습니다.

- 파일 목록을 `FileRecord files[MAX_REPO_FILES]` 고정 배열에 저장
- 파일 개수는 `DWORD count`로 관리
- 문자열은 `WCHAR path[MAX_PATH]` 배열 사용
- 성공/실패는 `BOOL`, `TRUE`, `FALSE` 사용
- 콘솔 출력은 `printf`, `wprintf` 사용
- 파일 저장/읽기는 구조체를 `WriteFile`, `ReadFile`로 직접 처리

## 3. 전체 동작 흐름

```text
리포지터리 폴더 경로 입력
-> 리포지터리 폴더와 .minirepo 폴더 생성
-> 파일 목록 확인
-> 현재 파일 상태를 snapshot.dat에 저장
-> 파일을 추가/수정/삭제
-> snapshot.dat와 현재 상태 비교
-> NEW / MODIFIED / DELETED 출력
```

## 4. 파일 구성

| 파일 | 역할 |
| --- | --- |
| `RepoManager.h` | 구조체, 상수, `RepoManager` 클래스 선언 |
| `RepoManager.cpp` | WinAPI를 사용한 실제 기능 구현 |
| `main.cpp` | 콘솔 입력, 메뉴 출력, 프로그램 실행 흐름 |

## 5. 수업 코드와의 연결

| 수업 예제 | 배운 내용 | Lite2에서 사용한 부분 |
| --- | --- | --- |
| `0317` | 클래스, 생성자, 멤버 함수 | `RepoManager` 클래스 |
| `0324` | 오류 처리, `GetLastError` | `PrintLastError` |
| `0331`, `0428` | `HANDLE`, `CloseHandle` | 파일 핸들 열고 닫기 |
| `0407` | `CreateFile`, `ReadFile`, `WriteFile` | 스냅샷 파일 저장/읽기 |
| `0414` | `LARGE_INTEGER` | `GetFileSizeEx` 결과 저장 |
| `0519` | `FindFirstFile`, `FindNextFile` | 파일 목록 탐색 |
| `0602` | `FILETIME`, `GetFileTime` | 마지막 수정 시간 비교 |

## 6. RepoManager.h 설명

### include와 상수

```cpp
#include <Windows.h>
```

Windows API 자료형과 함수를 사용하기 위해 포함합니다.

```cpp
#define MAX_REPO_FILES      128
#define SNAPSHOT_MAGIC      0x4C525031
#define META_FOLDER_NAME    L".minirepo"
#define SNAPSHOT_FILE_NAME  L"snapshot.dat"
```

- `MAX_REPO_FILES`: 최대 저장할 파일 개수입니다. `vector` 대신 고정 배열을 사용하기 때문에 필요합니다.
- `SNAPSHOT_MAGIC`: 스냅샷 파일이 이 프로그램에서 만든 파일인지 확인하기 위한 값입니다.
- `META_FOLDER_NAME`: 내부 관리 폴더 이름입니다.
- `SNAPSHOT_FILE_NAME`: 스냅샷 저장 파일 이름입니다.

### FileRecord

```cpp
struct FileRecord
{
    WCHAR name[MAX_PATH];
    ULONGLONG size;
    FILETIME writeTime;
};
```

파일 하나의 상태를 저장하는 구조체입니다.

| 멤버 | 의미 |
| --- | --- |
| `name` | 파일 이름 |
| `size` | 파일 크기 |
| `writeTime` | 마지막 수정 시간 |

기존 Lite에서는 `std::wstring`을 썼지만, Lite2에서는 수업 코드 스타일에 맞춰 `WCHAR name[MAX_PATH]` 배열을 사용했습니다.

### SnapshotHeader

```cpp
struct SnapshotHeader
{
    DWORD magic;
    DWORD count;
};
```

스냅샷 파일 맨 앞에 저장되는 헤더입니다.

- `magic`: 파일 형식 확인용 값
- `count`: 뒤에 저장된 `FileRecord` 개수

이 구조 덕분에 `snapshot.dat`를 읽을 때 파일 형식이 맞는지 먼저 확인할 수 있습니다.

### RepoManager 클래스

```cpp
class RepoManager
```

리포지터리 기능을 담당하는 클래스입니다.

public 함수:

| 함수 | 역할 |
| --- | --- |
| `Init` | 리포지터리 경로와 `.minirepo` 폴더 준비 |
| `ListFiles` | 파일 목록 출력 |
| `CreateFolder` | 하위 디렉터리 생성 |
| `SaveSnapshot` | 현재 상태 저장 |
| `CheckStatus` | 저장된 상태와 현재 상태 비교 |

private 변수:

```cpp
WCHAR repoPath[MAX_PATH];
WCHAR metaPath[MAX_PATH];
WCHAR snapshotPath[MAX_PATH];
```

`std::wstring` 대신 `WCHAR` 고정 배열로 경로를 저장합니다. 수업 코드에서 `WCHAR filePath[]`를 사용한 방식과 비슷합니다.

## 7. RepoManager.cpp 설명

### 생성자

```cpp
RepoManager::RepoManager()
{
    ZeroMemory(repoPath, sizeof(repoPath));
    ZeroMemory(metaPath, sizeof(metaPath));
    ZeroMemory(snapshotPath, sizeof(snapshotPath));
}
```

경로 배열을 0으로 초기화합니다.

수업 코드에서 구조체나 배열을 `{ 0, }`로 초기화하던 것과 같은 목적입니다.

### Init

```cpp
BOOL RepoManager::Init(const WCHAR* path)
```

사용자가 입력한 경로를 리포지터리로 준비합니다.

주요 동작:

1. 입력 경로가 비어 있는지 확인
2. `repoPath`에 경로 복사
3. `.minirepo` 경로 생성
4. `snapshot.dat` 경로 생성
5. `CreateDirectoryW`로 폴더 생성
6. 이미 존재하는 폴더는 정상 처리

```cpp
wcscpy_s(repoPath, path);
```

`WCHAR` 문자열을 복사합니다.

```cpp
CreateDirectoryW(repoPath, NULL)
```

폴더를 생성합니다.

```cpp
DWORD error = GetLastError();
if (ERROR_ALREADY_EXISTS != error)
```

이미 존재하는 폴더는 오류가 아니라 정상 상황으로 처리합니다.

### ListFiles

```cpp
void RepoManager::ListFiles()
```

현재 리포지터리 폴더의 파일 목록을 출력합니다.

```cpp
FileRecord files[MAX_REPO_FILES] = { 0, };
DWORD count = 0;
```

`vector` 대신 고정 배열과 개수 변수를 사용합니다.

```cpp
ScanFiles(files, MAX_REPO_FILES, &count)
```

파일 목록을 배열에 채웁니다.

### CreateFolder

```cpp
BOOL RepoManager::CreateFolder(const WCHAR* folderName)
```

리포지터리 안에 새 폴더를 만듭니다.

```cpp
wcschr(folderName, L'\\')
```

폴더명 안에 `\` 또는 `/`가 있는지 검사합니다.

이 프로젝트에서는 복잡한 경로가 아니라 `aaa` 같은 단순 폴더명만 허용합니다.

```cpp
CreateDirectoryW(newPath, NULL)
```

실제로 폴더를 생성합니다.

### SaveSnapshot

```cpp
BOOL RepoManager::SaveSnapshot()
```

현재 파일 상태를 스냅샷으로 저장합니다.

```cpp
FileRecord files[MAX_REPO_FILES] = { 0, };
DWORD count = 0;
```

현재 파일 정보를 배열에 저장합니다.

```cpp
WriteSnapshot(files, count)
```

배열 내용을 `snapshot.dat`에 저장합니다.

Lite2의 중요한 차이:

> 기존 Lite는 텍스트 형식으로 `파일명|크기|시간`을 저장했지만, Lite2는 `SnapshotHeader`와 `FileRecord` 구조체를 그대로 파일에 저장합니다.

이 방식이 수업의 `WriteFile` 예제와 더 비슷합니다.

### CheckStatus

```cpp
void RepoManager::CheckStatus()
```

저장된 스냅샷과 현재 상태를 비교합니다.

```cpp
FileRecord oldFiles[MAX_REPO_FILES] = { 0, };
FileRecord nowFiles[MAX_REPO_FILES] = { 0, };
```

- `oldFiles`: 스냅샷에서 읽은 이전 상태
- `nowFiles`: 현재 폴더 상태

```cpp
int oldIndex = FindFile(oldFiles, oldCount, nowFiles[i].name);
```

현재 파일이 이전 스냅샷에 있었는지 찾습니다.

결과 판단:

- 없으면 `[NEW]`
- 있는데 크기나 시간이 다르면 `[MODIFIED]`
- 예전에는 있는데 현재 없으면 `[DELETED]`

### MakePath

```cpp
BOOL RepoManager::MakePath(...)
```

두 경로를 이어 붙입니다.

예:

```text
C:\Temp + aaa -> C:\Temp\aaa
C:\Temp\ + aaa -> C:\Temp\aaa
```

`swprintf_s`를 사용해 `WCHAR` 배열에 결과를 만듭니다.

### ScanFiles

```cpp
BOOL RepoManager::ScanFiles(FileRecord files[], DWORD maxCount, DWORD* outCount)
```

폴더 안의 파일을 탐색합니다.

```cpp
WIN32_FIND_DATAW findData = { 0, };
HANDLE hFind = FindFirstFileW(searchPath, &findData);
```

수업 `0519`의 파일 탐색 예제와 가장 직접적으로 연결됩니다.

```cpp
FindNextFileW(hFind, &findData)
```

다음 파일을 계속 찾습니다.

```cpp
FindClose(hFind);
```

탐색 핸들을 닫습니다.

제외 대상:

- `.`
- `..`
- `.minirepo`
- 디렉터리

Lite2는 루트 폴더 바로 아래의 파일만 비교합니다.

### GetOneFileInfo

```cpp
BOOL RepoManager::GetOneFileInfo(const WCHAR* fileName, FileRecord* outRecord)
```

파일 하나의 크기와 마지막 수정 시간을 구합니다.

```cpp
HANDLE hFile = CreateFileW(...)
```

파일을 엽니다.

```cpp
GetFileSizeEx(hFile, &fileSize)
```

파일 크기를 가져옵니다.

```cpp
GetFileTime(hFile, &createTime, &accessTime, &writeTime)
```

파일 시간 정보를 가져옵니다.

이 프로젝트에서는 `writeTime`만 비교에 사용합니다.

```cpp
CloseHandle(hFile)
```

파일 핸들을 닫습니다.

### WriteSnapshot

```cpp
BOOL RepoManager::WriteSnapshot(FileRecord files[], DWORD count)
```

현재 파일 상태를 `snapshot.dat`에 저장합니다.

```cpp
CreateFileW(snapshotPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, ...)
```

스냅샷 파일을 쓰기용으로 열고, 기존 파일이 있으면 덮어씁니다.

```cpp
SnapshotHeader header = { 0, };
header.magic = SNAPSHOT_MAGIC;
header.count = count;
```

스냅샷 파일 맨 앞에 헤더를 저장합니다.

```cpp
WriteFile(hFile, &header, sizeof(header), &written, NULL)
```

헤더 구조체를 파일에 씁니다.

```cpp
WriteFile(hFile, &files[i], sizeof(FileRecord), &written, NULL)
```

각 파일 정보를 구조체 그대로 저장합니다.

### ReadSnapshot

```cpp
BOOL RepoManager::ReadSnapshot(FileRecord files[], DWORD maxCount, DWORD* outCount)
```

저장된 `snapshot.dat`를 읽어 `FileRecord` 배열로 복원합니다.

```cpp
ReadFile(hFile, &header, sizeof(header), &readSize, NULL)
```

먼저 헤더를 읽습니다.

```cpp
if (SNAPSHOT_MAGIC != header.magic || header.count > maxCount)
```

스냅샷 파일 형식이 맞는지 확인합니다.

```cpp
ReadFile(hFile, &files[i], sizeof(FileRecord), &readSize, NULL)
```

파일 정보를 구조체 단위로 읽습니다.

### FindFile

```cpp
int RepoManager::FindFile(FileRecord files[], DWORD count, const WCHAR* name)
```

배열에서 파일 이름이 같은 항목을 찾습니다.

```cpp
wcscmp(files[i].name, name)
```

`WCHAR` 문자열을 비교합니다.

기존 Lite의 `map.find()` 대신, Lite2에서는 수업 코드 스타일에 맞춰 배열을 직접 순회합니다.

### IsSameTime

```cpp
BOOL RepoManager::IsSameTime(const FILETIME* left, const FILETIME* right)
```

두 파일 시간이 같은지 비교합니다.

`FILETIME`은 다음 두 값으로 구성됩니다.

- `dwLowDateTime`
- `dwHighDateTime`

두 값이 모두 같아야 같은 시간으로 판단합니다.

### PrintLastError

```cpp
void RepoManager::PrintLastError(const char* apiName)
{
    printf("[ERROR] %s failed. code=%u\n", apiName, GetLastError());
}
```

Windows API 실패 시 오류 코드를 출력합니다.

수업에서 배운 `GetLastError()` 사용 방식입니다.

## 8. main.cpp 설명

### TrimInput

```cpp
void TrimInput(WCHAR* text)
```

사용자 입력의 앞뒤 공백, 탭, 개행 문자를 제거합니다.

자동 입력이나 콘솔 입력에서 경로 뒤에 공백이 붙으면 `CreateDirectoryW`가 실패할 수 있으므로 방어용으로 넣었습니다.

### PrintMenu

```cpp
void PrintMenu()
```

콘솔 메뉴를 출력합니다.

```text
1. List files
2. Create directory
3. Save snapshot
4. Check status
0. Exit
```

### wmain

```cpp
int wmain()
```

프로그램 시작 함수입니다.

```cpp
setlocale(LC_ALL, "");
```

wide character 콘솔 입출력을 위해 사용합니다.

```cpp
WCHAR repoPath[MAX_PATH] = { 0, };
fgetws(repoPath, MAX_PATH, stdin);
```

사용자가 입력한 리포지터리 경로를 `WCHAR` 배열로 받습니다.

```cpp
RepoManager repo;
repo.Init(repoPath);
```

리포지터리 관리 객체를 만들고 초기화합니다.

```cpp
while (TRUE)
```

사용자가 `0`을 입력할 때까지 메뉴를 반복합니다.

```cpp
menu = _wtoi(input);
```

문자열로 받은 메뉴 입력을 정수로 바꿉니다.

## 9. Lite와 Lite2의 차이

| 항목 | Lite | Lite2 |
| --- | --- | --- |
| 파일 목록 저장 | `std::vector` | `FileRecord files[128]` |
| 파일 비교 | `std::map` | 배열 순회 + `wcscmp` |
| 문자열 | `std::wstring` | `WCHAR[MAX_PATH]` |
| 스냅샷 저장 | 텍스트 파싱 | 구조체 binary 저장 |
| 출력 | `std::wcout` | `wprintf`, `printf` |
| 성공/실패 | `bool` | `BOOL`, `TRUE`, `FALSE` |
| 스타일 | 현대 C++에 가까움 | 수업 예제 코드에 가까움 |

## 10. 사용한 수업 기능/API

| API/자료형 | 사용 목적 |
| --- | --- |
| `CreateDirectoryW` | 리포지터리 폴더, `.minirepo`, 하위 폴더 생성 |
| `FindFirstFileW` | 폴더 안 첫 번째 항목 탐색 |
| `FindNextFileW` | 다음 항목 탐색 |
| `FindClose` | 탐색 핸들 닫기 |
| `CreateFileW` | 파일 또는 스냅샷 파일 열기 |
| `CloseHandle` | 파일 핸들 닫기 |
| `WriteFile` | 스냅샷 파일 저장 |
| `ReadFile` | 스냅샷 파일 읽기 |
| `GetFileSizeEx` | 파일 크기 확인 |
| `GetFileTime` | 파일 시간 정보 확인 |
| `GetLastError` | API 실패 이유 확인 |
| `HANDLE` | 파일/탐색 핸들 관리 |
| `DWORD` | WinAPI 정수형 값 |
| `FILETIME` | 파일 시간 정보 |
| `LARGE_INTEGER` | 64비트 파일 크기 |
| `WCHAR` | Unicode 문자열 배열 |

## 11. 시연 추천 순서

1. `MiniRepoTracker_Lite2.slnx`를 Visual Studio에서 엽니다.
2. 빌드 후 실행합니다.
3. 리포지터리 경로로 새 테스트 폴더를 입력합니다.
   예: `C:\Users\Ludorph\Documents\WSP_Project\DemoRepo`
4. `Create directory`로 `aaa` 폴더를 생성합니다.
5. 테스트 폴더에 `memo.txt` 파일을 만듭니다.
6. `List files`로 파일이 보이는지 확인합니다.
7. `Save snapshot`으로 현재 상태를 저장합니다.
8. `memo.txt` 내용을 수정합니다.
9. `Check status`로 `[MODIFIED]`를 확인합니다.
10. 새 파일을 추가한 뒤 `[NEW]`를 확인합니다.
11. 파일을 삭제한 뒤 `[DELETED]`를 확인합니다.

## 12. 예상 질문 답변

### Q. 왜 Lite2를 따로 만들었나요?

> 같은 기능이라도 수업 예제 코드와 더 비슷한 방식으로 구현해 보기 위해 만들었습니다. `vector`, `map` 대신 고정 배열과 `DWORD count`를 사용했고, 파일 저장도 구조체를 `WriteFile`로 직접 저장하도록 바꿨습니다.

### Q. snapshot.dat는 무슨 파일인가요?

> 현재 파일 이름, 크기, 마지막 수정 시간을 저장한 기준 상태 파일입니다. 나중에 현재 상태와 비교해서 새 파일, 수정된 파일, 삭제된 파일을 판단합니다.

### Q. snapshot.dat를 사람이 읽기 어려운 이유는?

> Lite2는 수업 코드 스타일에 맞춰 `FileRecord` 구조체를 그대로 `WriteFile`로 저장합니다. 그래서 텍스트 파일처럼 읽는 용도보다는 프로그램이 다시 `ReadFile`로 읽어 비교하는 용도입니다.

### Q. 왜 하위 폴더 내부 파일은 비교하지 않나요?

> 간소화 버전이기 때문에 리포지터리 루트에 있는 파일만 비교하도록 했습니다. 대신 `Create directory` 메뉴로 디렉터리 생성 기능은 확인할 수 있습니다.

### Q. 프로그램이 죽지 않게 한 부분은?

> WinAPI 호출 결과를 확인하고 실패하면 `GetLastError()`로 오류 코드를 출력합니다. 스냅샷 파일도 `SNAPSHOT_MAGIC`과 개수를 확인해서 잘못된 파일이면 읽지 않도록 했습니다.

## 13. 최종 요약

MiniRepoTracker_Lite2는 수업에서 배운 WinAPI 파일 처리 내용을 바탕으로 만든 Git 스타일 파일 상태 추적 프로그램입니다.

핵심은 다음과 같습니다.

- 파일 목록을 WinAPI로 탐색
- 파일 크기와 마지막 수정 시간을 저장
- 구조체 배열을 스냅샷 파일에 저장
- 저장된 상태와 현재 상태 비교
- `[NEW]`, `[MODIFIED]`, `[DELETED]` 출력
- 교수님 수업 코드와 비슷하게 `WCHAR`, `HANDLE`, `DWORD`, 고정 배열 중심으로 구현
