#include <windows.h>
#include <tchar.h>

void DisableWindowsUpdateViaRegistry() {
    HKEY hKey;
    LPCSTR subKey = "SOFTWARE\\Policies\\Microsoft\\Windows\\WindowsUpdate\\AU";
    DWORD disableUpdate = 1; // 1 = 禁用自动更新

    if (RegCreateKeyExA(HKEY_LOCAL_MACHINE, subKey, 0, NULL, REG_OPTION_NON_VOLATILE,
        KEY_WRITE, NULL, &hKey, NULL) != ERROR_SUCCESS) {
        _tprintf(_T("RegCreateKeyEx Failed: %d\n"), GetLastError());
        return;
    }

    // 设置 NoAutoUpdate 值
    if (RegSetValueExA(hKey, "NoAutoUpdate", 0, REG_DWORD, (BYTE*)&disableUpdate, sizeof(disableUpdate)) != ERROR_SUCCESS) {
        _tprintf(_T("RegSetValueEx Failed: %d\n"), GetLastError());
    }

    RegCloseKey(hKey);
    _tprintf(_T("Changed registry,Update is disabled.\n"));
}