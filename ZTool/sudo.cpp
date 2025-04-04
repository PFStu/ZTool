#include "sudo.h"
#include <tlhelp32.h>

// 启用调试权限
bool EnableDebugPrivilege() {
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        std::cerr << "OpenProcessToken failed: " << GetLastError() << std::endl;
        return false;
    }

    TOKEN_PRIVILEGES tp;
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!LookupPrivilegeValue(nullptr, SE_DEBUG_NAME, &tp.Privileges[0].Luid)) {
        std::cerr << "LookupPrivilegeValue failed: " << GetLastError() << std::endl;
        CloseHandle(hToken);
        return false;
    }

    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), nullptr, nullptr)) {
        std::cerr << "AdjustTokenPrivileges failed: " << GetLastError() << std::endl;
        CloseHandle(hToken);
        return false;
    }

    CloseHandle(hToken);
    return true;
}

// 启动 TrustedInstaller 服务
bool StartTrustedInstallerService() {
    SC_HANDLE scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    if (!scm) {
        std::cerr << "OpenSCManager failed: " << GetLastError() << std::endl;
        return false;
    }

    SC_HANDLE service = OpenServiceW(scm, L"TrustedInstaller", SERVICE_START);
    if (!service) {
        std::cerr << "OpenService failed: " << GetLastError() << std::endl;
        CloseServiceHandle(scm);
        return false;
    }

    if (!StartService(service, 0, nullptr)) {
        if (GetLastError() == ERROR_SERVICE_ALREADY_RUNNING) {
            std::cout << "TrustedInstaller service is already running." << std::endl;
            CloseServiceHandle(service);
            CloseServiceHandle(scm);
            return true;
        }
        else {
            std::cerr << "StartService failed: " << GetLastError() << std::endl;
            CloseServiceHandle(service);
            CloseServiceHandle(scm);
            return false;
        }
    }

    CloseServiceHandle(service);
    CloseServiceHandle(scm);
    return true;
}

// 获取 TrustedInstaller 进程的 PID
DWORD GetTrustedInstallerPID() {
    DWORD pid = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            // 不区分大小写匹配进程名
            if (_wcsicmp(pe.szExeFile, L"TiWorker.exe") == 0) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);
    return pid;
}

// 获取 TrustedInstaller 进程的令牌
bool GetTrustedInstallerToken(HANDLE& hToken) {
    // 启动 TrustedInstaller 服务
    if (!StartTrustedInstallerService()) {
        std::cerr << "Failed to start TrustedInstaller service." << std::endl;
        return false;
    }

    // 获取 TrustedInstaller 进程的 PID
    DWORD pid = GetTrustedInstallerPID();
    if (pid == 0) {
        std::cerr << "Failed to find TrustedInstaller process." << std::endl;
        return false;
    }

    // 打开 TrustedInstaller 进程
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!hProcess) {
        std::cerr << "OpenProcess failed: " << GetLastError() << std::endl;
        return false;
    }

    // 获取进程的令牌
    if (!OpenProcessToken(hProcess, TOKEN_DUPLICATE | TOKEN_QUERY, &hToken)) {
        std::cerr << "OpenProcessToken failed: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    CloseHandle(hProcess);
    return true;
}

// 以 SYSTEM 权限运行程序
bool RunAsSystem(const std::wstring& programPath) {
    HANDLE hToken = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken)) {
        std::cerr << "OpenProcessToken failed: " << GetLastError() << std::endl;
        return false;
    }

    HANDLE hDupToken = nullptr;
    if (!DuplicateTokenEx(hToken, TOKEN_ALL_ACCESS, nullptr, SecurityImpersonation, TokenPrimary, &hDupToken)) {
        std::cerr << "DuplicateTokenEx failed: " << GetLastError() << std::endl;
        CloseHandle(hToken);
        return false;
    }

    // 设置令牌权限
    TOKEN_PRIVILEGES tp;
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if (!LookupPrivilegeValue(nullptr, SE_ASSIGNPRIMARYTOKEN_NAME, &tp.Privileges[0].Luid) ||
        !AdjustTokenPrivileges(hDupToken, FALSE, &tp, sizeof(tp), nullptr, nullptr)) {
        std::cerr << "AdjustTokenPrivileges failed: " << GetLastError() << std::endl;
        CloseHandle(hDupToken);
        CloseHandle(hToken);
        return false;
    }

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    if (!CreateProcessWithTokenW(hDupToken, LOGON_WITH_PROFILE, nullptr, const_cast<wchar_t*>(programPath.c_str()), CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &pi)) {
        std::cerr << "CreateProcessWithTokenW failed: " << GetLastError() << std::endl;
        CloseHandle(hDupToken);
        CloseHandle(hToken);
        return false;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hDupToken);
    CloseHandle(hToken);
    return true;
}

// 以 TrustedInstaller 权限运行程序
bool RunAsTrustedInstaller(const std::wstring& programPath) {
    // 获取 TrustedInstaller 令牌
    HANDLE hToken = nullptr;
    if (!GetTrustedInstallerToken(hToken)) {
        std::cerr << "Failed to get TrustedInstaller token." << std::endl;
        return false;
    }

    // 使用令牌创建进程
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    if (!CreateProcessAsUserW(hToken, nullptr, const_cast<wchar_t*>(programPath.c_str()), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
        std::cerr << "CreateProcessAsUserW failed: " << GetLastError() << std::endl;
        CloseHandle(hToken);
        return false;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(hToken);
    return true;
}