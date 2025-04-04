#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
#include <iostream>
#include <format> // 需要 C++20 支持
#include <filesystem>
#include <sstream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>
#include <codecvt>
#include <locale>
#include <Windows.h>
#include <string>
#include <cstring>
#include <wininet.h>
#include "printColorText.h"
#include "sudo.h"
#include "Downloader.h"
#include "disupdt.h"

std::string Cmd;

bool waitForCmd() {
    return static_cast<bool>(std::getline(std::cin, Cmd));
}

void showAllCommands() {
    printColorText("======All Commands======", "blue", true);
    printColorText("CMD                  Info", "green", true);
    printColorText("-------------------------", "green", true);
    printColorText("exit      quit from ztool", "green", true);
    printColorText("about         about ztool", "green", true);
    printColorText("cmds    show all commands", "green", true);
    printColorText("sudo   run as SYSTEM user", "green", true);
    printColorText("dd  Multi-thread Download", "green", true);
    printColorText("disupdt    Disable Update", "green", true);
}

std::wstring stringToWstring(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(str);
}

std::wstring formatPath(const std::wstring& path) {
    std::wstring formattedPath;
    for (wchar_t ch : path) {
        formattedPath += (ch == L'/') ? L'\\' : ch;
    }
    return formattedPath;
}

std::wstring resolvePath(const std::wstring& path) {
    std::filesystem::path fsPath(path);
    if (fsPath.is_relative()) {
        fsPath = std::filesystem::absolute(fsPath);
    }
    return fsPath.wstring();
}

std::string exec(const std::string& cmd) {
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    HANDLE hRead, hWrite;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) {
        throw std::runtime_error("CreatePipe failed");
    }

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;

    // 将 std::string 转换为 std::wstring
    std::wstring command = std::wstring(L"cmd.exe /c ") + std::wstring(cmd.begin(), cmd.end());
    std::vector<wchar_t> cmdBuffer(command.begin(), command.end());
    cmdBuffer.push_back(L'\0');

    if (!CreateProcessW(NULL, &cmdBuffer[0], NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        CloseHandle(hRead);
        CloseHandle(hWrite);
        throw std::runtime_error("CreateProcess failed");
    }

    CloseHandle(hWrite);

    std::string output;
    char buffer[1024];
    DWORD bytesRead;
    while (ReadFile(hRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        output += buffer;
    }

    CloseHandle(hRead);
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return output;
}

int processUserCmd(const std::string& cmd) {
    if (cmd == "exit") {
        return 1;
    }
    else if (cmd == "about") {
        std::cout << "ZTool by PixelForge, Bow\n";
    }
    else if (cmd == "cmds") {
        showAllCommands();
    }
    else if (cmd == "sudo") {
        std::cout << "Enter program path: ";
        std::string inputPath;
        std::getline(std::cin, inputPath);

        std::wstring widePath = stringToWstring(inputPath);
        std::wstring formattedPath = formatPath(widePath);
        std::wstring resolvedPath = resolvePath(formattedPath);

        if (!std::filesystem::exists(resolvedPath)) {
            printColorText("Error: Path does not exist.", "red", true);
            return 0;
        }

        if (RunAsTrustedInstaller(resolvedPath)) {
            printColorText("Success: Program started with TrustedInstller privileges.", "green", true);
        }
        else if (RunAsSystem(resolvedPath)) {
            printColorText("Success: Program started with SYSTEM privileges.", "green", true);
        }
        else {
            printColorText("Failed: Unable to start program with SYSTEM privileges.", "red", true);
        }
    }
    else if (cmd == "dd") {
        std::cout << "Enter the URL: ";
        std::string inputURL;
        std::getline(std::cin, inputURL);

        // 检查 URL 是否为空
        if (inputURL.empty()) {
            std::cerr << "URL must not be empty." << std::endl;
            return false;
        }

        // 检查 URL 是否可访问
        HINTERNET hInternet = InternetOpenA("URLChecker", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (!hInternet) {
            std::cerr << "InternetOpen failed: " << GetLastError() << std::endl;
            return false;
        }

        HINTERNET hUrl = InternetOpenUrlA(hInternet, inputURL.c_str(), NULL, 0, INTERNET_FLAG_RELOAD, 0);
        if (!hUrl) {
            std::cerr << "InternetOpenUrl failed: " << GetLastError() << std::endl;
            InternetCloseHandle(hInternet);
            return false;
        }

        InternetCloseHandle(hUrl);
        InternetCloseHandle(hInternet);

        std::cout << "URL is accessible." << std::endl;

        // 获取文件名
        std::cout << "Enter the file name: ";
        std::string inputFile;
        std::getline(std::cin, inputFile);

        // 检查文件名是否为空
        if (inputFile.empty()) {
            std::cerr << "File name must not be empty." << std::endl;
            return false;
        }

        // 获取线程数
        std::cout << "How many threads: ";
        std::string inputThreads;
        std::getline(std::cin, inputThreads);
        // 转换线程数为整数
        char* endPtr;
        long threadsNum = std::strtol(inputThreads.c_str(), &endPtr, 10);

        // 检查线程数是否有效
        if (endPtr == inputThreads.c_str() || *endPtr != '\0' || threadsNum <= 0) {
            std::cerr << "Invalid number of threads. Please enter a positive integer." << std::endl;
            return false;
        }

        Downloader dd(inputURL, inputFile, threadsNum);
        dd.start();
        dd.waitForCompletion();
    }
    else if (cmd == "disupdt") {
        DisableWindowsUpdateViaRegistry();
    }
    else {
        /*printColorText("Unknown Command!", "red", true);*/
        try {
            std::cout << exec(cmd) << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
        }
    }
    return 0;
}

int startShell() {
    wchar_t userName[256];
    DWORD usize = sizeof(userName) / sizeof(wchar_t);
    std::string userNameStr;

    if (GetUserNameW(userName, &usize)) {
        std::wstring userNameWStr(userName);
        userNameStr.assign(userNameWStr.begin(), userNameWStr.end());
    }
    else {
        userNameStr = "Unknown";
    }

    std::filesystem::path currentPath = std::filesystem::current_path();
    std::string currentPathStr = currentPath.string();

    std::string output = std::format("{} @ ZTool ({}) $ ", userNameStr, currentPathStr);
    printColorText(output, "red", false);

    while (waitForCmd()) {
        if (processUserCmd(Cmd) == 1) {
            break;
        }
        printColorText(output, "red", false);
    }

    return 0;
}