# MiniRepo Tracker

MiniRepo Tracker는 Git의 `repository`, `status`, `commit` 개념을 단순화해서 구현한 Windows 콘솔 프로그램입니다.
사용자가 지정한 폴더를 하나의 저장소처럼 관리하고, 파일 상태를 스냅샷으로 저장한 뒤 현재 상태와 비교하여 추가, 수정, 삭제된 파일을 확인할 수 있습니다.

GUI는 채점 항목에 포함되지 않으므로 사용하지 않았고, 콘솔 메뉴 방식으로 구현했습니다.

## 폴더 구성

이 작업 디렉터리에는 프로젝트 소스 폴더와 테스트용 리포지터리 폴더가 함께 들어 있습니다.

| 폴더 | 역할 |
| --- | --- |
| `MiniRepoTracker` | 실제 제출용 프로그램 소스, Visual Studio 프로젝트 파일, 실행 파일이 들어 있는 메인 프로젝트 폴더입니다. |
| `MiniRepoTracker_Lite` | 학부생 수준에 맞춰 기능과 코드량을 줄인 간소화 버전입니다. 디렉터리 생성, 파일 목록 확인, 스냅샷 저장, 상태 비교 기능을 `SimpleRepo.h`, `SimpleRepo.cpp`, `main.cpp` 구조로 분리했습니다. |
| `MiniRepoTracker_TestRepo` | MiniRepo Tracker 기능을 테스트하기 위한 샘플 리포지터리 폴더입니다. 파일 추가, 수정, 삭제, 스냅샷 저장, 상태 비교, 실시간 감시를 시연할 때 사용합니다. |

`MiniRepoTracker_TestRepo`는 프로그램의 기능 확인을 위한 테스트 공간입니다.
실제 프로그램은 사용자가 입력한 다른 폴더도 리포지터리로 사용할 수 있습니다.

## 프로젝트 목표

이 프로젝트의 목표는 Windows Programming 수업에서 배운 WinAPI 기능들을 실제 파일 관리 프로그램에 적용하는 것입니다.
특히 교수님이 예시로 언급한 Git의 동작 방식에서 아이디어를 얻어, 폴더 안의 파일 변화를 추적하는 기능을 구현했습니다.

MiniRepo Tracker는 실제 Git처럼 branch, merge, diff를 구현하지는 않습니다.
대신 과제 안정성을 위해 파일의 크기와 마지막 수정 시간을 기준으로 상태 변화를 판단합니다.

## 주요 기능

### 1. 리포지터리 선택 및 생성

사용자가 입력한 경로를 작업 리포지터리로 설정합니다.
폴더가 없으면 새로 만들고, 내부에 관리 폴더인 `.minirepo`를 생성합니다.

### 2. 파일 목록 탐색

리포지터리 안의 파일들을 재귀적으로 탐색합니다.
`.minirepo` 폴더는 프로그램 내부 관리용이므로 탐색 대상에서 제외합니다.

### 3. 스냅샷 저장

현재 파일들의 상태를 `.minirepo/snapshot.dat` 파일에 저장합니다.
저장되는 정보는 다음과 같습니다.

- 파일 상대 경로
- 파일 크기
- 마지막 수정 시간

이 기능은 Git의 `commit` 또는 기준 상태 저장 기능을 단순화한 것입니다.

### 4. 변경사항 확인

저장된 스냅샷과 현재 파일 상태를 비교합니다.
비교 결과는 다음과 같이 출력됩니다.

- `[NEW]`: 새로 추가된 파일
- `[MODIFIED]`: 크기 또는 마지막 수정 시간이 바뀐 파일
- `[DELETED]`: 스냅샷에는 있었지만 현재는 삭제된 파일

이 기능은 Git의 `status` 기능을 단순화한 것입니다.

### 5. 실시간 폴더 감시

리포지터리 안에서 파일이 생성, 수정, 삭제, 이름 변경되면 실시간으로 감지합니다.
감지된 내용은 콘솔에 출력되고 `.minirepo/change.log`에도 기록됩니다.

### 6. 변경 로그 확인

프로그램이 기록한 변경 로그를 콘솔에서 확인할 수 있습니다.

## 콘솔 메뉴

```text
==== MiniRepo Tracker ====
1. Select/Create repository
2. List files
3. Save snapshot
4. Check status
5. Start realtime watch
6. Stop realtime watch
7. Show change log
0. Exit
```

## 사용한 수업 기능

| 기능 | 사용 목적 |
| --- | --- |
| `CreateDirectoryW` | 리포지터리 폴더와 `.minirepo` 폴더 생성 |
| `FindFirstFileW`, `FindNextFileW` | 폴더 내부 파일 탐색 |
| `CreateFileW`, `CloseHandle` | 파일, 로그, 스냅샷, 디렉터리 핸들 관리 |
| `ReadFile`, `WriteFile` | 스냅샷 파일과 로그 파일 읽기/쓰기 |
| `GetFileSizeEx` | 파일 크기 확인 및 변경 여부 비교 |
| `GetFileTime` | 파일 마지막 수정 시간 확인 및 변경 여부 비교 |
| `ReadDirectoryChangesW` | 실시간 디렉터리 변경 감시 |
| `CreateThread` | 실시간 감시 기능을 별도 스레드에서 실행 |
| `CreateEventW`, `WaitForSingleObject` | 감시 스레드 종료 제어 |
| `GetLastError` | WinAPI 실패 원인 확인 및 오류 처리 |

## 안정성 처리

과제 조건에 맞춰 프로그램이 실행 중 오류로 종료되지 않도록 작성했습니다.

- 잘못된 메뉴 입력이 들어오면 다시 입력을 받습니다.
- WinAPI 호출 실패 시 `GetLastError()`로 오류 코드를 출력합니다.
- 파일 또는 폴더 접근 실패 시 프로그램을 종료하지 않고 메뉴로 돌아갑니다.
- 실시간 감시 기능은 별도 스레드에서 실행됩니다.
- 종료 시 감시 스레드를 정리한 뒤 프로그램을 종료합니다.
- Visual Studio 프로젝트의 warning level은 `Level4`로 설정했습니다.

## 빌드 및 실행

Visual Studio에서 다음 파일을 열어 빌드합니다.

```text
MiniRepoTracker\MiniRepoTracker.slnx
```

권장 빌드 설정은 다음과 같습니다.

```text
Configuration: Debug 또는 Release
Platform: x64
Warning Level: Level4
Character Set: Unicode
```

## 추천 시연 순서

1. 프로그램을 실행합니다.
2. `Select/Create repository`를 선택합니다.
3. 예시 경로로 `C:\Users\Ludorph\Documents\WSP_Project\MiniRepoTracker_TestRepo`를 입력합니다.
4. Lite 버전에서는 `Create directory`로 `aaa` 같은 하위 폴더를 생성할 수 있습니다.
5. `List files`로 파일 목록을 확인합니다.
6. `Save snapshot`으로 현재 상태를 저장합니다.
7. `MiniRepoTracker_TestRepo` 안의 파일을 수정하거나 새 파일을 추가하거나 기존 파일을 삭제합니다.
8. `Check status`로 변경사항을 확인합니다.
9. 기본 버전에서는 `Start realtime watch`를 실행한 뒤 탐색기에서 `MiniRepoTracker_TestRepo` 안의 파일을 변경합니다.
10. 기본 버전에서는 `Show change log`로 기록된 변경 로그를 확인합니다.

## 프로젝트 요약

MiniRepo Tracker는 Git의 핵심 아이디어 중 하나인 "파일 상태 추적"을 Windows API로 구현한 콘솔 프로그램입니다.
파일 탐색, 파일 정보 확인, 파일 읽기/쓰기, 디렉터리 변경 감시, 스레드, 이벤트, 오류 처리 등 수업에서 배운 기능을 하나의 프로그램 안에 통합했습니다.
