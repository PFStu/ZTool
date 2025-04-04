#include <windows.h>
#include <iostream>
#include <string>

SERVICE_STATUS serviceStatus;
SERVICE_STATUS_HANDLE serviceStatusHandle;
std::wstring programPath;

void RunProgram() {
    STARTUPINFOW startupInfo = { sizeof(startupInfo) };
    PROCESS_INFORMATION processInfo = {};

    if (CreateProcessW(
        programPath.c_str(),
        nullptr,
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        nullptr,
        &startupInfo,
        &processInfo)) {
        WaitForSingleObject(processInfo.hProcess, INFINITE);
        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);
    }
    else {
        std::cerr << "CreateProcessW failed: " << GetLastError() << std::endl;
    }
}

void WINAPI ServiceMain(DWORD argc, LPWSTR* argv) {
    serviceStatusHandle = RegisterServiceCtrlHandlerW(L"TrustedInstallerService", [](DWORD control) {
        if (control == SERVICE_CONTROL_STOP) {
            serviceStatus.dwCurrentState = SERVICE_STOPPED;
            SetServiceStatus(serviceStatusHandle, &serviceStatus);
        }
        });

    if (!serviceStatusHandle) {
        return;
    }

    serviceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    serviceStatus.dwCurrentState = SERVICE_RUNNING;
    serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    SetServiceStatus(serviceStatusHandle, &serviceStatus);

    RunProgram();

    serviceStatus.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(serviceStatusHandle, &serviceStatus);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: TrustedInstallerService <program_path>" << std::endl;
        return 1;
    }

    programPath = std::wstring(argv[1], argv[1] + strlen(argv[1]));

    SERVICE_TABLE_ENTRYW serviceTable[] = {
        { L"TrustedInstallerService", ServiceMain },
        { nullptr, nullptr }
    };

    if (!StartServiceCtrlDispatcherW(serviceTable)) {
        std::cerr << "StartServiceCtrlDispatcherW failed: " << GetLastError() << std::endl;
        return 1;
    }

    return 0;
}