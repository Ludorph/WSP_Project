#include "SimpleRepo.h"

#include <clocale>
#include <iostream>
#include <string>

std::wstring Trim(const std::wstring& text)
{
    // 경로 또는 폴더명 입력 앞뒤에 붙은 공백/개행을 제거합니다.
    const wchar_t* blank = L" \t\r\n";
    size_t first = text.find_first_not_of(blank);
    if (first == std::wstring::npos)
        return L"";

    size_t last = text.find_last_not_of(blank);
    return text.substr(first, last - first + 1);
}

void PrintMenu()
{
    // 사용자가 실행할 수 있는 기능만 단순한 메뉴로 제공합니다.
    std::wcout << L"\n==== MiniRepo Tracker Lite ====\n";
    std::wcout << L"1. List files\n";
    std::wcout << L"2. Create directory\n";
    std::wcout << L"3. Save snapshot\n";
    std::wcout << L"4. Check status\n";
    std::wcout << L"0. Exit\n";
    std::wcout << L"> ";
}

int wmain()
{
    // 콘솔에서 wide character 문자열 입출력을 사용하기 위한 설정입니다.
    setlocale(LC_ALL, "");

    std::wcout << L"Repository path: ";

    std::wstring repoPath;
    // 경로에 공백이 포함될 수 있으므로 getline으로 한 줄 전체를 입력받습니다.
    std::getline(std::wcin, repoPath);
    repoPath = Trim(repoPath);

    if (repoPath.empty())
    {
        std::wcout << L"Empty path.\n";
        return 0;
    }

    // 리포지터리 관련 기능은 SimpleRepo 클래스에 맡깁니다.
    SimpleRepo repo(repoPath);
    if (!repo.Init())
        return 0;

    while (true)
    {
        PrintMenu();

        int menu = -1;
        std::wcin >> menu;

        // 숫자가 아닌 입력이 들어와도 프로그램이 종료되지 않도록 입력 상태를 복구합니다.
        if (!std::wcin)
        {
            std::wcin.clear();
            std::wcin.ignore(1024, L'\n');
            std::wcout << L"Invalid input.\n";
            continue;
        }

        if (menu == 0)
            break;

        switch (menu)
        {
        case 1:
            repo.ListFiles();
            break;
        case 2:
        {
            // 메뉴 번호 입력 뒤 남은 개행을 버리고 폴더명을 한 줄로 입력받습니다.
            std::wcin.ignore(1024, L'\n');
            std::wcout << L"New folder name: ";

            std::wstring folderName;
            std::getline(std::wcin, folderName);
            folderName = Trim(folderName);

            repo.CreateFolder(folderName);
            break;
        }
        case 3:
            repo.SaveSnapshot();
            break;
        case 4:
            repo.ShowStatus();
            break;
        default:
            std::wcout << L"Unknown menu.\n";
            break;
        }
    }

    std::wcout << L"Bye.\n";
    return 0;
}
