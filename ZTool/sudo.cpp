#include "sudo.h"
#include <tlhelp32.h>

// ���õ���Ȩ��
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

// ���� TrustedInstaller ����
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

// ��ȡ TrustedInstaller ���̵� PID
DWORD GetTrustedInstallerPID() {
    DWORD pid = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return 0;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            // �����ִ�Сдƥ�������
            if (_wcsicmp(pe.szExeFile, L"TiWorker.exe") == 0) {
                pid = pe.th32ProcessID;
                break;
            }
        } while (Process32NextW(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);
    return pid;
}

// ��ȡ TrustedInstaller ���̵�����
bool GetTrustedInstallerToken(HANDLE& hToken) {
    // ���� TrustedInstaller ����
    if (!StartTrustedInstallerService()) {
        std::cerr << "Failed to start TrustedInstaller service." << std::endl;
        return false;
    }

    // ��ȡ TrustedInstaller ���̵� PID
    DWORD pid = GetTrustedInstallerPID();
    if (pid == 0) {
        std::cerr << "Failed to find TrustedInstaller process." << std::endl;
        return false;
    }

    // �� TrustedInstaller ����
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!hProcess) {
        std::cerr << "OpenProcess failed: " << GetLastError() << std::endl;
        return false;
    }

    // ��ȡ���̵�����
    if (!OpenProcessToken(hProcess, TOKEN_DUPLICATE | TOKEN_QUERY, &hToken)) {
        std::cerr << "OpenProcessToken failed: " << GetLastError() << std::endl;
        CloseHandle(hProcess);
        return false;
    }

    CloseHandle(hProcess);
    return true;
}

// �� SYSTEM Ȩ�����г���
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

    // ��������Ȩ��
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

// �� TrustedInstaller Ȩ�����г���
bool RunAsTrustedInstaller(const std::wstring& programPath) {
    // ��ȡ TrustedInstaller ����
    HANDLE hToken = nullptr;
    if (!GetTrustedInstallerToken(hToken)) {
        std::cerr << "Failed to get TrustedInstaller token." << std::endl;
        return false;
    }

    // ʹ�����ƴ�������
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