# MiniRepoTracker_Lite 코드 설명서

이 문서는 기말 프로젝트 시연을 위해 `MiniRepoTracker_Lite`의 구조, 코드 의도, 사용한 수업 내용, 예상 질문 답변을 정리한 설명서입니다.

시연에서는 복잡한 기본 버전보다 `MiniRepoTracker_Lite`를 중심으로 설명하는 것이 좋습니다. 코드량이 적고, 헤더 파일, 구현 파일, `main.cpp`로 나뉘어 있어 학부생 수준의 프로젝트로 설명하기 자연스럽습니다.

## 1. 프로젝트 한 줄 설명

MiniRepoTracker_Lite는 Git의 `status`와 `commit` 개념을 단순화해서, 폴더 안의 파일 상태를 저장하고 나중에 변경 여부를 확인하는 Windows 콘솔 프로그램입니다.

조금 더 자세히 말하면 다음과 같습니다.

> 실제 Git처럼 branch, merge, diff를 구현한 것은 아니고, 수업에서 배운 Windows API를 활용해서 파일 이름, 파일 크기, 마지막 수정 시간을 기준으로 추가, 수정, 삭제 상태를 판단하도록 만들었습니다.

## 2. 전체 동작 흐름

프로그램의 기본 흐름은 다음과 같습니다.

```text
리포지터리 폴더 경로 입력
-> 리포지터리 폴더와 .minirepo 폴더 준비
-> 파일 목록 확인
-> 현재 파일 상태를 snapshot.txt에 저장
-> 파일을 추가/수정/삭제
-> 저장된 snapshot.txt와 현재 상태 비교
-> NEW / MODIFIED / DELETED 출력
```

여기서 스냅샷은 새로운 Windows API가 아니라, 수업에서 배운 파일 처리 API들을 조합해서 만든 기준 상태 저장 기능입니다.

## 3. 수업 코드와의 연결

프로젝트에서 사용한 개념은 기존 수업 소스코드의 여러 내용을 조합한 것입니다.

| 수업 예제 | 배운 내용 | Lite 프로젝트에서 사용한 부분 |
| --- | --- | --- |
| `0317` | 클래스, 생성자, 멤버 함수 | `SimpleRepo` 클래스 |
| `0324` | 오류 처리, `GetLastError` | API 실패 시 오류 코드 출력 |
| `0331`, `0428` | `HANDLE`, `CloseHandle` | 파일 핸들 열고 닫기 |
| `0407` | `CreateFile`, `ReadFile`, `WriteFile` | 스냅샷 파일 읽기/쓰기 |
| `0414` | `LARGE_INTEGER`, 64비트 크기 | `GetFileSizeEx`로 파일 크기 확인 |
| `0519` | `FindFirstFile`, `FindNextFile` | 폴더 안 파일 목록 탐색 |
| `0602` | `FILETIME`, `GetFileTime` | 파일 마지막 수정 시간 확인 |

따라서 이 프로젝트는 수업에서 배운 기능들을 그대로 복사한 것이 아니라, 파일 상태 추적이라는 목적에 맞게 조합한 것입니다.

## 4. 파일 구성

`MiniRepoTracker_Lite`는 다음 세 개의 주요 소스 파일로 구성됩니다.

| 파일 | 역할 |
| --- | --- |
| `SimpleRepo.h` | 파일 정보 구조체와 `SimpleRepo` 클래스 선언 |
| `SimpleRepo.cpp` | Windows API를 이용한 실제 기능 구현 |
| `main.cpp` | 사용자 입력, 메뉴 출력, 프로그램 실행 흐름 |

시연 때는 이렇게 설명하면 좋습니다.

> `main.cpp`는 사용자와 직접 만나는 메뉴 부분이고, 실제 파일 처리 기능은 `SimpleRepo` 클래스에 모았습니다. 헤더 파일에는 선언을 두고, 구현 파일에는 Windows API를 사용하는 실제 코드를 넣었습니다.

## 5. SimpleRepo.h 설명

### `#pragma once`

```cpp
#pragma once
```

헤더 파일이 여러 번 포함되어도 한 번만 처리되게 하는 문법입니다.

`main.cpp`와 `SimpleRepo.cpp`가 모두 `SimpleRepo.h`를 포함하므로, 중복 선언을 막기 위해 사용했습니다.

### `#include <Windows.h>`

```cpp
#include <Windows.h>
```

Windows API를 사용하기 위해 필요합니다.

이 프로젝트에서는 다음과 같은 Windows 자료형과 함수를 사용합니다.

- `HANDLE`
- `DWORD`
- `FILETIME`
- `CreateFileW`
- `CreateDirectoryW`
- `FindFirstFileW`
- `FindNextFileW`
- `GetFileSizeEx`
- `GetFileTime`
- `ReadFile`
- `WriteFile`
- `GetLastError`

### `#include <string>`, `#include <vector>`

```cpp
#include <string>
#include <vector>
```

`std::wstring`과 `std::vector`를 사용하기 위해 포함했습니다.

- `std::wstring`: 유니코드 문자열을 다루기 위한 C++ 문자열
- `std::vector`: 파일 목록을 여러 개 저장하기 위한 동적 배열

Windows API의 `W` 버전 함수들은 wide character 문자열을 사용하므로 `std::wstring`을 사용했습니다.

### `FileInfo` 구조체

```cpp
struct FileInfo
{
    std::wstring name;
    ULONGLONG size;
    FILETIME writeTime;
};
```

파일 하나의 상태를 저장하기 위한 구조체입니다.

| 멤버 변수 | 의미 |
| --- | --- |
| `name` | 파일 이름 |
| `size` | 파일 크기 |
| `writeTime` | 마지막 수정 시간 |

이 구조체는 스냅샷 저장과 상태 비교의 기본 단위입니다.

시연 때는 이렇게 말하면 됩니다.

> 파일 변경 여부를 판단하기 위해 파일 이름, 크기, 마지막 수정 시간을 하나의 구조체로 묶었습니다.

### `SimpleRepo` 클래스

```cpp
class SimpleRepo
```

리포지터리와 관련된 기능을 하나로 묶은 클래스입니다.

수업에서 배운 클래스, 생성자, 멤버 함수 개념을 실제 프로젝트에 적용한 부분입니다.

### 생성자

```cpp
explicit SimpleRepo(const std::wstring& rootPath);
```

사용자가 입력한 리포지터리 경로를 받아 객체를 초기화합니다.

`explicit`은 의도하지 않은 자동 형변환을 막기 위해 붙였습니다. 시연 때 깊게 설명할 필요는 없고, 질문이 나오면 “생성자를 명확하게 호출하도록 하기 위해 사용했습니다”라고 말하면 됩니다.

### public 함수

```cpp
bool Init();
void ListFiles();
bool CreateFolder(const std::wstring& folderName);
bool SaveSnapshot();
void ShowStatus();
```

사용자가 메뉴에서 실행하는 주요 기능입니다.

| 함수 | 역할 |
| --- | --- |
| `Init` | 리포지터리 폴더와 `.minirepo` 폴더 준비 |
| `ListFiles` | 현재 파일 목록 출력 |
| `CreateFolder` | 리포지터리 안에 새 디렉터리 생성 |
| `SaveSnapshot` | 현재 파일 상태를 스냅샷으로 저장 |
| `ShowStatus` | 스냅샷과 현재 상태 비교 |

`bool`을 반환하는 함수는 성공/실패를 구분해야 하는 함수입니다.

### private 변수

```cpp
std::wstring rootPath;
std::wstring metaPath;
std::wstring snapshotPath;
```

| 변수 | 의미 |
| --- | --- |
| `rootPath` | 사용자가 입력한 리포지터리 경로 |
| `metaPath` | 내부 관리 폴더 `.minirepo` 경로 |
| `snapshotPath` | 스냅샷 파일 `.minirepo\snapshot.txt` 경로 |

`.minirepo`는 Git의 `.git` 폴더처럼 프로그램 내부 정보를 저장하는 폴더입니다.

### private 함수

```cpp
std::wstring JoinPath(...);
bool ScanFiles(...);
bool GetFileInfo(...);
bool WriteTextFile(...);
bool ReadTextFile(...);
bool LoadSnapshot(...);
void PrintLastError(...);
```

사용자가 직접 호출하지 않고, 클래스 내부 기능을 구현하기 위해 사용하는 보조 함수입니다.

## 6. SimpleRepo.cpp 설명

### include 문

```cpp
#include "SimpleRepo.h"

#include <algorithm>
#include <cwchar>
#include <iostream>
#include <map>
#include <sstream>
```

각 include의 역할은 다음과 같습니다.

| 헤더 | 사용 목적 |
| --- | --- |
| `SimpleRepo.h` | 클래스 선언 가져오기 |
| `<algorithm>` | `std::sort` 사용 |
| `<cwchar>` | `wcscmp` 사용 |
| `<iostream>` | `std::wcout`, `std::cout` 사용 |
| `<map>` | 파일 이름 기준으로 이전/현재 상태 비교 |
| `<sstream>` | 문자열 조립 및 파싱 |

### 생성자

```cpp
SimpleRepo::SimpleRepo(const std::wstring& rootPathValue)
    : rootPath(rootPathValue)
{
    metaPath = JoinPath(rootPath, L".minirepo");
    snapshotPath = JoinPath(metaPath, L"snapshot.txt");
}
```

생성자는 사용자가 입력한 경로를 `rootPath`에 저장하고, 내부 관리 폴더와 스냅샷 파일 경로를 만듭니다.

`L".minirepo"`에서 `L`은 wide string을 의미합니다. `CreateDirectoryW`, `CreateFileW` 같은 Windows API의 `W` 버전과 맞추기 위해 사용했습니다.

### `Init`

```cpp
bool SimpleRepo::Init()
```

리포지터리 폴더와 `.minirepo` 폴더를 준비하는 함수입니다.

```cpp
CreateDirectoryW(rootPath.c_str(), NULL)
```

사용자가 입력한 폴더가 없으면 생성합니다.

`std::wstring`은 C++ 문자열이고, Windows API는 `const wchar_t*`를 받기 때문에 `.c_str()`을 사용했습니다.

```cpp
DWORD error = GetLastError();
if (error != ERROR_ALREADY_EXISTS)
```

폴더 생성에 실패했을 때, 이미 폴더가 있어서 실패한 것인지 진짜 오류인지 구분합니다.

- `ERROR_ALREADY_EXISTS`: 이미 폴더가 있으므로 정상으로 처리
- 그 외 오류: 오류 메시지 출력 후 실패 처리

시연 설명:

> 폴더가 이미 있는 것은 오류가 아니기 때문에 `GetLastError()`로 원인을 확인해서 처리했습니다.

### `ListFiles`

```cpp
void SimpleRepo::ListFiles()
```

현재 리포지터리 폴더 안의 파일 목록을 출력합니다.

```cpp
std::vector<FileInfo> files;
if (!ScanFiles(files))
    return;
```

`ScanFiles`가 파일들을 찾아 `files` 벡터에 넣습니다.

실패하면 프로그램을 종료하지 않고 메뉴로 돌아갑니다.

```cpp
for (const FileInfo& file : files)
```

파일 목록을 출력하는 반복문입니다.

`const FileInfo&`를 쓴 이유:

- `const`: 출력만 하고 수정하지 않음
- `&`: 구조체 복사를 피함

### `CreateFolder`

```cpp
bool SimpleRepo::CreateFolder(const std::wstring& folderName)
```

리포지터리 안에 새 폴더를 만드는 기능입니다.

교수님 힌트였던 `mkdir aaa`와 직접 연결되는 기능입니다.

```cpp
if (folderName.empty())
```

빈 이름으로 폴더를 만들지 않게 막습니다.

```cpp
folderName.find(L'\\') != std::wstring::npos ||
folderName.find(L'/') != std::wstring::npos
```

`aaa\bbb` 같은 복잡한 경로 입력을 막습니다.

학부생 수준의 간소화 버전이므로, 단순한 폴더명만 받도록 제한했습니다.

```cpp
CreateDirectoryW(newPath.c_str(), NULL)
```

실제로 디렉터리를 생성하는 Windows API입니다.

### `SaveSnapshot`

```cpp
bool SimpleRepo::SaveSnapshot()
```

현재 파일 상태를 `.minirepo\snapshot.txt`에 저장하는 함수입니다.

```cpp
std::vector<FileInfo> files;
ScanFiles(files);
```

현재 리포지터리 폴더의 파일 정보를 가져옵니다.

```cpp
std::wstringstream ss;
```

문자열을 조립하기 위한 스트림입니다.

```cpp
ss << file.name << L"|"
   << file.size << L"|"
   << file.writeTime.dwLowDateTime << L"|"
   << file.writeTime.dwHighDateTime << L"\n";
```

스냅샷 저장 형식입니다.

예:

```text
memo.txt|14|123456789|123456
```

각 항목의 의미:

```text
파일 이름 | 파일 크기 | 수정 시간 Low | 수정 시간 High
```

`FILETIME`은 `dwLowDateTime`, `dwHighDateTime` 두 값으로 구성되어 있으므로 두 값을 저장했습니다.

시연 설명:

> 현재 파일 이름, 크기, 수정 시간을 한 줄씩 텍스트로 저장해서 나중에 비교 기준으로 사용했습니다.

### `ShowStatus`

```cpp
void SimpleRepo::ShowStatus()
```

스냅샷과 현재 파일 상태를 비교하는 핵심 함수입니다.

```cpp
LoadSnapshot(oldFiles);
ScanFiles(nowFiles);
```

- `oldFiles`: 예전에 저장한 스냅샷
- `nowFiles`: 현재 파일 상태

```cpp
std::map<std::wstring, FileInfo> oldMap;
std::map<std::wstring, FileInfo> nowMap;
```

`map`은 key-value 구조입니다.

여기서는 파일 이름을 key로 사용하고, 파일 정보를 value로 저장합니다.

이유:

> 파일 이름을 기준으로 과거에도 있었는지, 현재도 있는지 빠르게 확인하기 위해 사용했습니다.

```cpp
if (oldItem == oldMap.end())
```

현재 파일이 과거 스냅샷에 없으면 새 파일입니다.

출력:

```text
[NEW]
```

```cpp
bool sizeChanged = oldFile.size != nowFile.size;
bool timeChanged = ...
```

파일 크기나 마지막 수정 시간이 바뀌었는지 확인합니다.

둘 중 하나라도 바뀌면:

```text
[MODIFIED]
```

```cpp
if (nowMap.find(item.first) == nowMap.end())
```

과거 스냅샷에는 있었는데 현재 없으면 삭제된 파일입니다.

출력:

```text
[DELETED]
```

시연 설명:

> 저장된 상태와 현재 상태를 각각 map에 넣고 파일 이름을 기준으로 비교했습니다. 현재에만 있으면 NEW, 둘 다 있는데 크기나 수정 시간이 다르면 MODIFIED, 과거에만 있으면 DELETED로 판단했습니다.

### `JoinPath`

```cpp
std::wstring SimpleRepo::JoinPath(...)
```

두 경로를 안전하게 이어 붙이는 보조 함수입니다.

예:

```text
C:\Temp + aaa -> C:\Temp\aaa
C:\Temp\ + aaa -> C:\Temp\aaa
```

사용자가 입력한 경로 끝에 이미 `\`가 있으면 중복해서 붙이지 않게 처리했습니다.

### `ScanFiles`

```cpp
bool SimpleRepo::ScanFiles(std::vector<FileInfo>& files)
```

리포지터리 폴더 안의 파일을 탐색하는 함수입니다.

수업 `0519`의 `FindFirstFile`, `FindNextFile` 예제와 가장 직접적으로 연결됩니다.

```cpp
WIN32_FIND_DATAW findData = { 0, };
```

파일 정보를 받을 구조체입니다.

`{ 0, }`는 구조체를 0으로 초기화하는 방식입니다.

```cpp
std::wstring searchPath = JoinPath(rootPath, L"*");
```

`*`는 해당 폴더 안의 모든 항목을 찾겠다는 의미입니다.

```cpp
HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
```

첫 번째 파일 또는 폴더를 찾는 API입니다.

실패하면 `INVALID_HANDLE_VALUE`가 반환됩니다.

```cpp
do { ... } while (FindNextFileW(hFind, &findData));
```

첫 번째 항목은 `FindFirstFileW`가 이미 가져왔으므로 `do-while`을 사용합니다.

그 다음 항목부터는 `FindNextFileW`로 가져옵니다.

```cpp
wcscmp(findData.cFileName, L".") == 0
wcscmp(findData.cFileName, L"..") == 0
wcscmp(findData.cFileName, L".minirepo") == 0
```

제외하는 항목입니다.

- `.`: 현재 폴더
- `..`: 상위 폴더
- `.minirepo`: 프로그램 내부 관리 폴더

`.minirepo`를 제외하는 이유:

> 스냅샷 파일 자체가 변경사항으로 잡히는 것을 막기 위해서입니다.

```cpp
if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    continue;
```

Lite 버전에서는 하위 폴더 내부까지 재귀 탐색하지 않고, 리포지터리 루트에 있는 파일만 비교합니다.

```cpp
FindClose(hFind);
```

파일 탐색 핸들을 닫습니다.

파일 핸들은 `CloseHandle`, 탐색 핸들은 `FindClose`를 사용합니다.

```cpp
std::sort(files.begin(), files.end(), ...)
```

파일 목록을 이름순으로 정렬합니다.

출력 순서와 스냅샷 저장 순서를 일정하게 하기 위해 사용했습니다.

### `GetFileInfo`

```cpp
bool SimpleRepo::GetFileInfo(const std::wstring& fileName, FileInfo& info)
```

파일 하나의 크기와 수정 시간을 가져오는 함수입니다.

```cpp
HANDLE hFile = CreateFileW(...)
```

파일을 여는 API입니다.

수업 `0407`, `0428`의 파일 열기 예제와 연결됩니다.

중요한 인자:

| 인자 | 의미 |
| --- | --- |
| `GENERIC_READ` | 읽기 권한으로 파일 열기 |
| `FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE` | 다른 프로그램이 파일을 사용 중이어도 최대한 열 수 있게 공유 |
| `OPEN_EXISTING` | 이미 존재하는 파일만 열기 |
| `FILE_ATTRIBUTE_NORMAL` | 일반 파일 속성 |

```cpp
if (hFile == INVALID_HANDLE_VALUE)
```

파일 열기 실패 확인입니다.

```cpp
LARGE_INTEGER fileSize = { 0, };
GetFileSizeEx(hFile, &fileSize);
```

파일 크기를 구합니다.

`LARGE_INTEGER`는 64비트 크기를 저장할 수 있는 구조체입니다.

```cpp
GetFileTime(hFile, &createTime, &accessTime, &writeTime)
```

파일 시간 정보를 가져옵니다.

| 변수 | 의미 |
| --- | --- |
| `createTime` | 생성 시간 |
| `accessTime` | 마지막 접근 시간 |
| `writeTime` | 마지막 수정 시간 |

이 프로젝트에서는 `writeTime`만 비교에 사용합니다.

```cpp
CloseHandle(hFile);
```

파일 핸들을 닫습니다.

핸들을 닫지 않으면 자원 누수가 생길 수 있으므로, 성공/실패 경로에서 모두 닫아야 합니다.

### `WriteTextFile`

```cpp
bool SimpleRepo::WriteTextFile(...)
```

스냅샷 텍스트를 파일에 저장하는 함수입니다.

```cpp
CreateFileW(..., GENERIC_WRITE, ..., CREATE_ALWAYS, ...)
```

쓰기용으로 파일을 엽니다.

`CREATE_ALWAYS`는 파일이 있으면 덮어쓰고, 없으면 새로 만듭니다.

스냅샷은 항상 현재 상태로 새로 저장해야 하므로 이 옵션을 사용했습니다.

```cpp
DWORD writeSize = static_cast<DWORD>(text.size() * sizeof(wchar_t));
```

문자열 길이를 바이트 수로 바꿉니다.

`std::wstring`은 `wchar_t` 기반이므로 문자 개수에 `sizeof(wchar_t)`를 곱해야 합니다.

```cpp
WriteFile(hFile, text.c_str(), writeSize, &written, NULL)
```

실제로 파일에 쓰는 API입니다.

```cpp
if (!result || written != writeSize)
```

쓰기 실패뿐 아니라 요청한 만큼 전부 쓰지 못한 경우도 실패로 처리했습니다.

### `ReadTextFile`

```cpp
bool SimpleRepo::ReadTextFile(...)
```

스냅샷 파일을 읽는 함수입니다.

```cpp
CreateFileW(..., GENERIC_READ, ..., OPEN_EXISTING, ...)
```

이미 존재하는 스냅샷 파일을 읽기용으로 엽니다.

```cpp
if (hFile == INVALID_HANDLE_VALUE)
{
    std::wcout << L"Snapshot does not exist. Save snapshot first.\n";
    return false;
}
```

스냅샷이 없으면 프로그램을 종료하지 않고 안내 메시지만 출력합니다.

```cpp
if (fileSize.QuadPart > 1024 * 1024)
```

스냅샷 파일이 너무 큰 경우를 막는 안전장치입니다.

이 과제에서는 스냅샷 파일이 작아야 정상이므로 1MB 이상이면 비정상으로 봅니다.

```cpp
if ((fileSize.QuadPart % sizeof(wchar_t)) != 0)
```

스냅샷 파일의 바이트 크기가 `wchar_t` 단위와 맞지 않는 경우를 막는 검사입니다.

이 프로젝트는 `std::wstring` 내용을 `WriteFile`로 저장하므로, 정상적인 스냅샷 파일은 `wchar_t` 크기의 배수여야 합니다.
이 조건이 맞지 않으면 파일 형식이 깨진 것으로 보고 안내 메시지를 출력한 뒤 메뉴 흐름으로 돌아갑니다.

```cpp
std::vector<wchar_t> buffer(...)
```

파일 내용을 저장할 버퍼입니다.

수업 예제에서는 `char buf[128]` 같은 고정 배열을 많이 썼지만, 여기서는 파일 크기에 맞춰 동적으로 준비했습니다.

```cpp
ReadFile(hFile, buffer.data(), ...)
```

파일 내용을 읽습니다.

```cpp
buffer[readSize / sizeof(wchar_t)] = L'\0';
```

문자열 끝을 표시하는 널 문자를 넣습니다.

### `LoadSnapshot`

```cpp
bool SimpleRepo::LoadSnapshot(std::vector<FileInfo>& files)
```

`snapshot.txt` 내용을 읽어 다시 `FileInfo` 목록으로 변환하는 함수입니다.

```cpp
std::wistringstream input(text);
```

문자열을 입력 스트림처럼 다루기 위해 사용합니다.

```cpp
while (std::getline(input, line))
```

스냅샷 파일을 한 줄씩 읽습니다.

```cpp
size_t p1 = line.find(L'|');
```

`|` 구분자의 위치를 찾습니다.

저장 형식이 다음과 같기 때문입니다.

```text
파일명|크기|수정시간Low|수정시간High
```

구분자는 순서대로 확인합니다. 첫 번째 구분자가 없으면 두 번째 구분자를 찾지 않고 바로 다음 줄로 넘어갑니다.
이렇게 한 이유는 깨진 스냅샷 파일을 읽더라도 프로그램이 이상한 위치를 기준으로 문자열을 자르지 않게 하기 위해서입니다.

```cpp
std::stoull(...)
std::stoul(...)
```

문자열을 숫자로 바꿉니다.

- `stoull`: 파일 크기처럼 큰 정수 변환
- `stoul`: `DWORD` 값 변환

숫자 변환 부분은 `try-catch`로 감쌌습니다.

```cpp
try
{
    ...
}
catch (...)
{
    std::wcout << L"Invalid snapshot line skipped.\n";
    continue;
}
```

사용자가 실수로 `snapshot.txt`를 수정해서 숫자 부분이 깨져도 프로그램이 종료되지 않게 하기 위한 처리입니다.
깨진 줄은 건너뛰고, 나머지 정상적인 줄만 상태 비교에 사용합니다.

### `PrintLastError`

```cpp
void SimpleRepo::PrintLastError(const char* apiName)
{
    std::cout << "[ERROR] " << apiName << " failed. code=" << GetLastError() << "\n";
}
```

Windows API 실패 시 오류 코드를 출력하는 함수입니다.

수업에서 배운 `GetLastError` 사용 방식과 연결됩니다.

시연 설명:

> API가 실패해도 프로그램을 바로 죽이지 않고, `GetLastError()`로 오류 코드를 출력하고 메뉴 흐름으로 돌아가게 했습니다.

## 7. main.cpp 설명

### include 문

```cpp
#include "SimpleRepo.h"
#include <clocale>
#include <iostream>
#include <string>
```

| 헤더 | 사용 목적 |
| --- | --- |
| `SimpleRepo.h` | `SimpleRepo` 클래스 사용 |
| `<clocale>` | `setlocale` 사용 |
| `<iostream>` | 콘솔 입출력 |
| `<string>` | `std::wstring` 사용 |

### `Trim`

```cpp
std::wstring Trim(const std::wstring& text)
```

사용자가 입력한 문자열의 앞뒤 공백과 개행 문자를 제거합니다.

```cpp
const wchar_t* blank = L" \t\r\n";
```

제거할 문자 목록입니다.

- 공백
- 탭
- `\r`
- `\n`

경로 입력 끝에 불필요한 문자가 붙으면 Windows API가 경로를 찾지 못할 수 있으므로 정리해 줍니다.

### `PrintMenu`

```cpp
void PrintMenu()
```

메뉴를 출력하는 함수입니다.

메뉴 출력 코드를 `wmain` 안에 직접 넣지 않고 분리해서, 프로그램 흐름을 보기 쉽게 만들었습니다.

현재 메뉴:

```text
1. List files
2. Create directory
3. Save snapshot
4. Check status
0. Exit
```

### `wmain`

```cpp
int wmain()
```

프로그램 시작 함수입니다.

일반 `main` 대신 `wmain`을 사용한 이유는 프로그램이 `std::wstring`, `std::wcout`, `CreateFileW`처럼 유니코드 기반으로 작성되었기 때문입니다.

```cpp
setlocale(LC_ALL, "");
```

콘솔에서 한글과 wide character 출력이 조금 더 자연스럽게 동작하도록 설정합니다.

```cpp
std::getline(std::wcin, repoPath);
```

리포지터리 경로를 한 줄 입력받습니다.

경로에 공백이 들어갈 수 있으므로 `std::wcin >> repoPath`보다 `getline`이 안전합니다.

```cpp
SimpleRepo repo(repoPath);
if (!repo.Init())
    return 0;
```

입력받은 경로로 `SimpleRepo` 객체를 만들고 초기화합니다.

초기화에 실패해도 비정상 종료가 아니라 `return 0`으로 정상 종료합니다.

```cpp
while (true)
```

사용자가 `0`을 입력할 때까지 메뉴를 반복합니다.

```cpp
std::wcin >> menu;
```

메뉴 번호를 입력받습니다.

```cpp
if (!std::wcin)
{
    std::wcin.clear();
    std::wcin.ignore(1024, L'\n');
    std::wcout << L"Invalid input.\n";
    continue;
}
```

숫자가 아닌 입력이 들어왔을 때 처리하는 코드입니다.

- `clear`: 입력 스트림 오류 상태 해제
- `ignore`: 잘못 입력된 나머지 문자 버리기
- `continue`: 메뉴 다시 출력

이 부분은 “프로그램이 죽지 않게 처리한 부분”입니다.

### `switch`

```cpp
switch (menu)
```

메뉴 번호에 따라 기능을 실행합니다.

| 번호 | 호출 함수 | 기능 |
| --- | --- | --- |
| 1 | `repo.ListFiles()` | 파일 목록 출력 |
| 2 | `repo.CreateFolder(folderName)` | 디렉터리 생성 |
| 3 | `repo.SaveSnapshot()` | 스냅샷 저장 |
| 4 | `repo.ShowStatus()` | 변경사항 확인 |
| 0 | `break` | 종료 |

## 8. 핵심 기능별 설명 문장

### 파일 목록 보기

> `FindFirstFileW`와 `FindNextFileW`를 사용해서 리포지터리 폴더 안의 파일을 탐색하고, 각 파일의 크기와 수정 시간을 읽어 출력합니다.

### 디렉터리 생성

> `CreateDirectoryW`를 사용해서 리포지터리 안에 새 폴더를 생성합니다. 복잡한 경로 입력은 막고 간단한 폴더명만 받도록 했습니다.

### 스냅샷 저장

> 현재 파일들의 이름, 크기, 마지막 수정 시간을 `.minirepo\snapshot.txt`에 저장합니다. 이 파일이 이후 상태 비교의 기준이 됩니다.

### 상태 비교

> 저장된 스냅샷과 현재 파일 상태를 비교합니다. 현재에만 있으면 `[NEW]`, 둘 다 있는데 크기나 수정 시간이 다르면 `[MODIFIED]`, 스냅샷에만 있으면 `[DELETED]`로 출력합니다.

## 9. 사용한 수업 기능 정리

| 수업 기능/API | 프로젝트에서 사용한 이유 |
| --- | --- |
| `CreateDirectoryW` | 리포지터리 폴더, `.minirepo` 폴더, 새 하위 폴더 생성 |
| `FindFirstFileW` | 폴더 안 첫 번째 파일 탐색 |
| `FindNextFileW` | 다음 파일 계속 탐색 |
| `FindClose` | 탐색 핸들 닫기 |
| `CreateFileW` | 파일 또는 스냅샷 파일 열기 |
| `CloseHandle` | 파일 핸들 닫기 |
| `ReadFile` | 스냅샷 파일 읽기 |
| `WriteFile` | 스냅샷 파일 쓰기 |
| `GetFileSizeEx` | 파일 크기 확인 |
| `GetFileTime` | 파일 수정 시간 확인 |
| `GetLastError` | API 실패 이유 확인 |
| `HANDLE` | Windows 커널 객체 핸들 관리 |
| `FILETIME` | 파일 시간 정보 저장 |
| `LARGE_INTEGER` | 64비트 파일 크기 저장 |

## 10. 시연 순서 추천

시연 전 테스트 폴더를 하나 준비합니다.

예:

```text
C:\Users\Ludorph\Documents\WSP_Project\DemoRepo
```

추천 시연 순서:

1. 프로그램 실행
2. 리포지터리 경로 입력
3. `List files`로 현재 파일 목록 확인
4. `Create directory`로 `aaa` 폴더 생성
5. `Save snapshot` 실행
6. 테스트 폴더에 새 파일 추가
7. `Check status` 실행 후 `[NEW]` 확인
8. 기존 파일 수정
9. `Check status` 실행 후 `[MODIFIED]` 확인
10. 파일 삭제
11. `Check status` 실행 후 `[DELETED]` 확인

## 11. 예상 질문과 답변

### Q. 스냅샷 기능은 수업에서 배웠나요?

아니요. 스냅샷이라는 이름의 기능을 직접 배운 것은 아닙니다.

하지만 수업에서 배운 파일 탐색, 파일 크기 확인, 수정 시간 확인, 파일 읽기/쓰기 API를 조합해서 기준 상태 저장 기능으로 만들었습니다.

답변 예시:

> 스냅샷은 새로운 API가 아니라, 수업에서 배운 파일 처리 API들을 조합해서 만든 기준 상태 저장 기능입니다.

### Q. 왜 실제 Git처럼 diff를 구현하지 않았나요?

답변 예시:

> Git 전체 기능을 구현하기보다는 수업 범위 안에서 파일 상태 추적 기능을 단순화했습니다. 파일 내용 전체 비교 대신 파일 크기와 마지막 수정 시간을 기준으로 변경 여부를 판단했습니다.

### Q. 왜 GUI가 없나요?

답변 예시:

> GUI는 채점 조건에 없었고, 수업에서 배운 Windows API 파일 처리 기능을 명확히 보여주기 위해 콘솔 프로그램으로 만들었습니다.

### Q. 왜 `.minirepo` 폴더를 제외하나요?

답변 예시:

> `.minirepo`는 프로그램이 스냅샷을 저장하는 내부 관리 폴더입니다. 이 폴더까지 비교하면 `snapshot.txt` 자체가 변경사항으로 잡힐 수 있어서 제외했습니다.

### Q. 왜 `W`가 붙은 API를 사용했나요?

답변 예시:

> Visual Studio 프로젝트가 Unicode 설정이고, 문자열도 `std::wstring`을 사용했기 때문에 wide character 기반의 `CreateFileW`, `FindFirstFileW` 같은 API를 사용했습니다.

### Q. 프로그램이 죽지 않도록 어떤 처리를 했나요?

답변 예시:

> Windows API 호출 뒤에는 실패 여부를 확인하고, 실패하면 `GetLastError()`로 오류 코드를 출력했습니다. 잘못된 메뉴 입력도 `wcin.clear()`와 `ignore()`로 처리해서 다시 메뉴로 돌아가게 했습니다.

### Q. 하위 폴더 안의 파일도 비교하나요?

답변 예시:

> Lite 버전은 학부생 수준에 맞춰 간소화했기 때문에 하위 폴더 내부까지 재귀 탐색하지 않고, 리포지터리 루트의 파일만 비교합니다. 대신 디렉터리 생성 기능은 별도 메뉴로 넣었습니다.

## 12. 직접 만들었다는 인상을 주는 설명 방식

시연 때 너무 “완벽한 Git을 만들었다”고 말하면 질문이 어려워질 수 있습니다.

대신 다음처럼 설명하는 것이 좋습니다.

> Git에서 아이디어를 얻어서, 수업에서 배운 Windows API로 파일 상태 추적 기능을 단순화해서 구현했습니다.

또는:

> 수업에서 배운 파일 탐색, 파일 읽기/쓰기, 파일 크기와 수정 시간 확인 기능을 조합해서 현재 상태 저장과 변경사항 확인 기능을 만들었습니다.

## 13. 최종 요약

MiniRepoTracker_Lite는 다음을 보여주는 프로젝트입니다.

- 수업에서 배운 Windows API를 실제 프로그램에 적용함
- Git의 상태 추적 아이디어를 단순화함
- 파일 이름, 크기, 수정 시간을 기준으로 변경 여부를 판단함
- 스냅샷 파일을 직접 읽고 씀
- 오류가 나도 프로그램이 죽지 않도록 처리함
- 헤더 파일, 구현 파일, `main.cpp`로 구조를 나눔

가장 중요한 한 문장:

> 이 프로젝트는 Git을 완전히 구현한 것이 아니라, 수업에서 배운 Windows API로 Git의 파일 상태 추적 아이디어를 단순화해서 구현한 프로그램입니다.
