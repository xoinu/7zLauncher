#include "framework.h"
#include "7zLauncher.h"

using namespace std;
constexpr wchar_t sevenZip[] = L"C:\\Program Files\\7-Zip\\7zG.exe";

//-----------------------------------------------------------------------------
static wstring parseArgument(const wstring& cmdLine)
{
    wstring res;

    if (cmdLine.empty())
        return res;

    if (cmdLine.front() == L'"' && cmdLine.back() == L'"')
    {
        if (cmdLine.size() > 2)
            res = cmdLine.substr(1, cmdLine.length() - 2);
    }
    else
    {
        res = cmdLine;
    }

    return res;
}

//-----------------------------------------------------------------------------
static void resolveDuplicateRoot(const wstring& outDir)
{
    vector<wchar_t> fullPath(1024);
    wchar_t* base = nullptr;
    
    if (!GetFullPathNameW(outDir.c_str(), (DWORD)fullPath.size(), fullPath.data(), &base) || !base)
        return;

    WIN32_FIND_DATAW findData = {};
    auto handle = FindFirstFileW((outDir + L"\\*").c_str(), &findData);
    bool found = false;

    if (handle != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                if (!wcscmp(findData.cFileName, L".") || !wcscmp(findData.cFileName, L"..") || !_wcsicmp(findData.cFileName, base))
                    continue;

            found = true;
            break;
        }
        while (FindNextFileW(handle, &findData));
        FindClose(handle);
    }

    if (found)
        return;

    const wstring outDir2 = outDir + L"_7z_dup";
    if (!MoveFileW(outDir.c_str(), outDir2.c_str()))
        return;

    if (!MoveFileW((outDir2 + L"\\" + base).c_str(), outDir.c_str()))
        return;

    if (!RemoveDirectoryW(outDir2.c_str()))
        return;
}

//-----------------------------------------------------------------------------
int APIENTRY wWinMain
(
    _In_ HINSTANCE      hInstance,
    _In_opt_ HINSTANCE  hPrevInstance,
    _In_ LPWSTR         lpCmdLine,
    _In_ int            nCmdShow
)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nCmdShow);

    const wstring path = parseArgument(lpCmdLine);
    const size_t pos = path.find_last_of('.');

    wostringstream oss;
    if (path.empty())
    {
        oss << L"Invalid command line: " << lpCmdLine;
        MessageBoxW(nullptr, oss.str().c_str(), L"7zLauncher", MB_OK | MB_ICONEXCLAMATION);
        return 0;
    }

    const wstring outDir = pos == wstring::npos ? L"." : path.substr(0, pos);
    oss << L'"' << sevenZip << L"\" x \"" << path << L"\" -r -aoa -o\"" << outDir << L'"';

    PROCESS_INFORMATION info = {};
    STARTUPINFOW startupInfo = {};
    startupInfo.cb = sizeof(startupInfo);
    if (!CreateProcessW(nullptr, const_cast<wchar_t*>(oss.str().c_str()), nullptr, nullptr, FALSE, NORMAL_PRIORITY_CLASS, nullptr, nullptr, &startupInfo, &info))
    {
        MessageBoxW(nullptr, L"Failed to launch 7-zip GUI", L"7zLauncher", MB_OK | MB_ICONEXCLAMATION);
        return 0;
    }

    WaitForSingleObject(info.hProcess, INFINITE);
    CloseHandle(info.hProcess);
    CloseHandle(info.hThread);
    resolveDuplicateRoot(outDir);
    return 0;
}
