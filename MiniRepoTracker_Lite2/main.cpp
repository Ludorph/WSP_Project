#include "RepoManager.h"

#include <locale.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

void TrimInput(WCHAR* text)
{
    // 입력 문자열 앞뒤의 공백과 개행을 제거합니다.
    if (text == NULL)
        return;

    size_t len = wcslen(text);
    while (len > 0 &&
        (text[len - 1] == L'\n' ||
            text[len - 1] == L'\r' ||
            text[len - 1] == L' ' ||
            text[len - 1] == L'\t'))
    {
        text[len - 1] = L'\0';
        len--;
    }

    WCHAR* start = text;
    while (*start == L' ' || *start == L'\t')
        start++;

    if (start != text)
        memmove(text, start, (wcslen(start) + 1) * sizeof(WCHAR));
}

void PrintMenu()
{
    // 콘솔 메뉴 출력입니다.
    wprintf(L"\n==== MiniRepo Tracker Lite2 ====\n");
    wprintf(L"1. List files\n");
    wprintf(L"2. Create directory\n");
    wprintf(L"3. Save snapshot\n");
    wprintf(L"4. Check status\n");
    wprintf(L"0. Exit\n");
    wprintf(L"> ");
}

int wmain()
{
    // 한글/유니코드 콘솔 출력을 위한 설정입니다.
    setlocale(LC_ALL, "");

    WCHAR repoPath[MAX_PATH] = { 0, };
    wprintf(L"Repository path: ");

    // 경로에 공백이 들어갈 수 있으므로 한 줄 전체를 입력받습니다.
    if (NULL == fgetws(repoPath, MAX_PATH, stdin))
    {
        wprintf(L"Input error.\n");
        return 0;
    }

    TrimInput(repoPath);
    if (repoPath[0] == L'\0')
    {
        wprintf(L"Empty path.\n");
        return 0;
    }

    // 실제 기능은 RepoManager 클래스가 담당합니다.
    RepoManager repo;
    if (FALSE == repo.Init(repoPath))
        return 0;

    while (TRUE)
    {
        WCHAR input[32] = { 0, };
        int menu = -1;

        PrintMenu();

        // 메뉴 입력도 문자열로 받은 뒤 숫자로 변환합니다.
        if (NULL == fgetws(input, 32, stdin))
        {
            wprintf(L"Input error.\n");
            continue;
        }

        menu = _wtoi(input);

        if (0 == menu)
            break;

        switch (menu)
        {
        case 1:
            repo.ListFiles();
            break;

        case 2:
        {
            // 새로 만들 폴더 이름을 입력받습니다.
            WCHAR folderName[MAX_PATH] = { 0, };
            wprintf(L"New folder name: ");

            if (NULL == fgetws(folderName, MAX_PATH, stdin))
            {
                wprintf(L"Input error.\n");
                break;
            }

            TrimInput(folderName);
            repo.CreateFolder(folderName);
            break;
        }

        case 3:
            repo.SaveSnapshot();
            break;

        case 4:
            repo.CheckStatus();
            break;

        default:
            wprintf(L"Unknown menu.\n");
            break;
        }
    }

    wprintf(L"Bye.\n");
    return 0;
}
