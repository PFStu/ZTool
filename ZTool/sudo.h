#ifndef SUDO_H
#define SUDO_H

#include <windows.h>
#include <string>
#include <iostream>

// ��������
bool RunAsSystem(const std::wstring& programPath);
bool RunAsTrustedInstaller(const std::wstring& programPath);

#endif // SUDO_H