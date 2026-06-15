# MiniRepo Tracker

MiniRepo Tracker는 Git의 repository/status/commit 개념을 단순화한 Windows console project입니다.
사용자가 지정한 폴더를 repository처럼 관리하고, 파일의 크기와 마지막 수정 시간을 snapshot으로 저장한 뒤 현재 상태와 비교합니다.
GUI는 사용하지 않고 console menu 방식으로 구현했습니다.
Visual Studio warning level은 Level4로 설정했습니다.

## Menu

1. Select/Create repository
2. List files
3. Save snapshot
4. Check status
5. Start realtime watch
6. Stop realtime watch
7. Show change log
0. Exit

## Used Lecture Features

- CreateDirectoryW: repository folder and .minirepo folder creation
- FindFirstFileW / FindNextFileW: recursive file search
- CreateFileW / CloseHandle: file, snapshot, log, directory handle management
- ReadFile / WriteFile: snapshot and log read/write
- GetFileSizeEx: file size comparison
- GetFileTime: last write time comparison
- ReadDirectoryChangesW: realtime directory change watch
- CreateThread / WaitForSingleObject / CreateEventW: watch thread and stop signal
- GetLastError: error handling without program crash

## Recommended Demo Flow

1. Run the program.
2. Select/Create a test repository path, for example `C:\Temp\aaa`.
3. Create a text file in that folder.
4. Run `List files`.
5. Run `Save snapshot`.
6. Edit, add, or delete files.
7. Run `Check status`.
8. Run `Start realtime watch`, then change files in Explorer.
9. Run `Show change log`.
