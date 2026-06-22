# MiniRepo Tracker Lite

MiniRepo Tracker Lite는 Git의 `status`와 `commit` 개념을 단순화해서 구현한 Windows 콘솔 프로그램입니다.
사용자가 지정한 폴더를 하나의 저장소처럼 관리하고, 현재 파일 상태를 저장한 뒤 나중에 변경 여부를 확인할 수 있습니다.

GUI는 채점 항목에 포함되지 않으므로 사용하지 않았고, 콘솔 메뉴 방식으로 구현했습니다.

## 폴더 구성

| 경로 | 역할 |
| --- | --- |
| `MiniRepoTracker_Lite` | 제출용 메인 프로젝트 폴더입니다. `SimpleRepo.h`, `SimpleRepo.cpp`, `main.cpp`로 구성되어 있습니다. |
| `2026_wsp` | 수업 시간에 제공된 예제 코드 모음입니다. 프로젝트 설명과 구현 근거를 확인할 때 참고할 수 있습니다. |
| `README.md` | 프로젝트 개요와 실행 방법을 설명하는 문서입니다. |
| `explain.md` | 시연 대비용 상세 코드 설명 문서입니다. |

이전 테스트용 폴더는 삭제했기 때문에, 시연할 때는 프로그램에서 새 테스트 폴더 경로를 입력하면 됩니다.
예를 들어 `C:\Users\Ludorph\Documents\WSP_Project\DemoRepo` 같은 폴더를 입력하면 프로그램이 자동으로 생성합니다.

## 프로젝트 목표

이 프로젝트의 목표는 Windows Programming 수업에서 배운 WinAPI 기능들을 실제 파일 관리 프로그램에 적용하는 것입니다.
교수님이 예시로 언급한 Git의 동작 방식에서 아이디어를 얻어, 폴더 안의 파일 변화를 추적하는 기능을 구현했습니다.

실제 Git처럼 branch, merge, diff를 구현하지는 않습니다.
대신 학부생 수준에 맞춰 파일 이름, 파일 크기, 마지막 수정 시간을 기준으로 상태 변화를 판단합니다.

## 주요 기능

### 1. 리포지터리 폴더 준비

프로그램 시작 시 사용자가 리포지터리로 사용할 폴더 경로를 입력합니다.
폴더가 없으면 `CreateDirectoryW`로 새로 만들고, 내부 관리 폴더인 `.minirepo`도 생성합니다.

### 2. 파일 목록 보기

리포지터리 폴더 바로 아래의 파일 목록을 출력합니다.
`.minirepo` 폴더는 프로그램 내부 관리용이므로 탐색 대상에서 제외합니다.

### 3. 디렉터리 생성

`Create directory` 메뉴를 통해 리포지터리 안에 `aaa` 같은 새 하위 폴더를 만들 수 있습니다.
복잡한 경로 대신 단순한 폴더명만 받도록 제한했습니다.

### 4. 현재 상태 저장

`Save snapshot` 메뉴는 현재 파일들의 상태를 `.minirepo\snapshot.txt`에 저장합니다.
저장되는 정보는 다음과 같습니다.

- 파일 이름
- 파일 크기
- 마지막 수정 시간

이 기능은 Git의 commit 개념을 단순화한 기준 상태 저장 기능입니다.

### 5. 변경사항 확인

`Check status` 메뉴는 저장된 스냅샷과 현재 파일 상태를 비교합니다.

- `[NEW]`: 스냅샷에는 없고 현재 새로 생긴 파일
- `[MODIFIED]`: 파일 크기 또는 마지막 수정 시간이 바뀐 파일
- `[DELETED]`: 스냅샷에는 있었지만 현재 삭제된 파일

## 콘솔 메뉴

```text
==== MiniRepo Tracker Lite ====
1. List files
2. Create directory
3. Save snapshot
4. Check status
0. Exit
```

## 사용한 수업 기능

| 기능 | 사용 목적 |
| --- | --- |
| `CreateDirectoryW` | 리포지터리 폴더, `.minirepo` 폴더, 새 하위 폴더 생성 |
| `FindFirstFileW`, `FindNextFileW` | 폴더 내부 파일 탐색 |
| `FindClose` | 파일 탐색 핸들 정리 |
| `CreateFileW`, `CloseHandle` | 파일 및 스냅샷 파일 핸들 관리 |
| `ReadFile`, `WriteFile` | 스냅샷 파일 읽기/쓰기 |
| `GetFileSizeEx` | 파일 크기 확인 및 변경 여부 비교 |
| `GetFileTime` | 파일 마지막 수정 시간 확인 및 변경 여부 비교 |
| `GetLastError` | WinAPI 실패 원인 확인 및 오류 처리 |

## 안정성 처리

과제 조건에 맞춰 프로그램이 실행 중 오류로 종료되지 않도록 작성했습니다.

- 잘못된 메뉴 입력이 들어오면 다시 입력을 받습니다.
- WinAPI 호출 실패 시 `GetLastError()`로 오류 코드를 출력합니다.
- 파일 또는 폴더 접근 실패 시 프로그램을 종료하지 않고 메뉴로 돌아갑니다.
- 스냅샷 파일 형식이 깨진 경우에도 예외로 종료되지 않고 잘못된 줄을 건너뜁니다.
- Visual Studio 프로젝트의 warning level은 `Level4`로 설정했습니다.

## 빌드 및 실행

Visual Studio에서 다음 파일을 열어 빌드합니다.

```text
MiniRepoTracker_Lite\MiniRepoTracker_Lite.slnx
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
2. 리포지터리 경로로 새 테스트 폴더를 입력합니다.
   예: `C:\Users\Ludorph\Documents\WSP_Project\DemoRepo`
3. `Create directory`로 `aaa` 폴더를 생성합니다.
4. 탐색기에서 테스트 폴더 안에 `memo.txt` 파일을 만듭니다.
5. `List files`로 파일 목록을 확인합니다.
6. `Save snapshot`으로 현재 상태를 저장합니다.
7. `memo.txt` 내용을 수정합니다.
8. `Check status`로 `[MODIFIED]`를 확인합니다.
9. 새 파일을 추가한 뒤 `Check status`로 `[NEW]`를 확인합니다.
10. 파일을 삭제한 뒤 `Check status`로 `[DELETED]`를 확인합니다.

## 프로젝트 요약

MiniRepo Tracker Lite는 Git의 핵심 아이디어 중 하나인 "파일 상태 추적"을 Windows API로 단순화해서 구현한 콘솔 프로그램입니다.
파일 탐색, 파일 정보 확인, 파일 읽기/쓰기, 디렉터리 생성, 오류 처리 등 수업에서 배운 기능을 하나의 프로그램 안에 통합했습니다.
