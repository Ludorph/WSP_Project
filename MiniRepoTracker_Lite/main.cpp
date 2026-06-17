#include "SimpleRepo.h"

#include <clocale>
#include <iostream>
#include <string>

std::wstring Trim(const std::wstring& text)
{
    const wchar_t* blank = L" \t\r\n";
    size_t first = text.find_first_not_of(blank);
    if (first == std::wstring::npos)
        return L"";

    size_t last = text.find_last_not_of(blank);
    return text.substr(first, last - first + 1);
}

void PrintMenu()
{
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
    setlocale(LC_ALL, "");

    std::wcout << L"Repository path: ";

    std::wstring repoPath;
    std::getline(std::wcin, repoPath);
    repoPath = Trim(repoPath);

    if (repoPath.empty())
    {
        std::wcout << L"Empty path.\n";
        return 0;
    }

    SimpleRepo repo(repoPath);
    if (!repo.Init())
        return 0;

    while (true)
    {
        PrintMenu();

        int menu = -1;
        std::wcin >> menu;

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
