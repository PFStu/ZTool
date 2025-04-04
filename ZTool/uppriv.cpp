#include <windows.h>
#include <shellapi.h>
#include <iostream>

bool IsRunAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;

    // 检查当前进程是否在管理员组
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    if (!AllocateAndInitializeSid(
        &NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0, &adminGroup)) {
        return false;
    }

    if (!CheckTokenMembership(NULL, adminGroup, &isAdmin)) {
        isAdmin = FALSE;
    }

    FreeSid(adminGroup);
    return isAdmin == TRUE;
}

void RestartAsAdmin() {
    wchar_t szPath[MAX_PATH];
    if (GetModuleFileNameW(NULL, szPath, MAX_PATH)) {
        SHELLEXECUTEINFO sei = { sizeof(sei) };
        sei.lpVerb = L"runas";  // 关键：请求提权
        sei.lpFile = szPath;
        sei.hwnd = NULL;
        sei.nShow = SW_NORMAL;

        if (!ShellExecuteExW(&sei)) {
            DWORD dwError = GetLastError();
            if (dwError == ERROR_CANCELLED) {
                std::cerr << "用户拒绝 UAC 提权！" << std::endl;
            }
        }
    }
}

void _default() {
    if (!IsRunAsAdmin()) RestartAsAdmin();
}