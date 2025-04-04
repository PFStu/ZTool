#include <iostream>
#include <format>
#include <chrono>
#include <thread>
#include <string>
#include <iomanip>
#include <Windows.h>
#include "printColorText.h"
#include "verifyConfig.h"
#include "shell.h"
#include "uppriv.h"

int main() {
    _default();
    std::string asciiStr = R"( oooooooooooo         ooooooooooooo                     oooo  
d'"""""d888'          8'   888   '8                     '888  
      .888P                888       .ooooo.   .ooooo.   888  
     d888'                 888      d88' '88b d88' '88b  888  
   .888P      8888888      888      888   888 888   888  888  
  d888'    .P              888      888   888 888   888  888  
.8888888888P              o888o     'Y8bod8P' 'Y8bod8P' o888o )";
    printColorText(asciiStr, "red", true);
    printColorText("Welcome to ZTool", "green", true);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    if (verify() == false) {
        printColorText("Config file not found,create now.", "blue", true);
        createConfig();
    }
    else {
        printColorText("Config file found and verified.", "blue", true);
    }
    printColorText("Version: " + getVersion(), "red", true);
    wchar_t userName[256];
    DWORD usize = sizeof(userName) / sizeof(wchar_t);
    if (GetUserNameW(userName, &usize)) {
        // 佛祖保佑编译通过
        std::wstring userNameWStr(userName);
        std::string userNameStr(userNameWStr.begin(), userNameWStr.end());
        printColorText(std::format("Your system name is: {}", userNameStr), "yellow", true);
        printColorText("For show all commands, enter cmds", "green", true);
    }
    else {
        printColorText("Failed to retrieve system username.", "red", true);
    }
    startShell();
    printColorText("Exiting...", "red", true);
}