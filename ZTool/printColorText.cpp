#include <iostream>
#include <Windows.h>

bool isAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroupSid = nullptr;

    // 分配并初始化一个 SID 用于管理员组
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(
        &NtAuthority,
        2,
        SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS,
        0, 0, 0, 0, 0, 0,
        &adminGroupSid)) {
        // 检查当前用户是否属于管理员组
        if (!CheckTokenMembership(nullptr, adminGroupSid, &isAdmin)) {
            isAdmin = FALSE;
        }
        FreeSid(adminGroupSid);
    }

    return isAdmin != FALSE;
}

void printColorText(const std::string& text, std::string color, bool enter) {
    // ANSI转义序列，用于设置文本颜色
    std::string colorCode = "\033[";
    if (isAdmin()) {
        colorCode = "";
    }
    else if (color == "black") {
        colorCode += "30m";
    }
    else if (color == "red") {
        colorCode += "31m";
    }
    else if (color == "green") {
        colorCode += "32m";
    }
    else if (color == "yellow") {
        colorCode += "33m";
    }
    else if (color == "blue") {
        colorCode += "34m";
    }
    else if (color == "magenta") {
        colorCode += "35m";
    }
    else if (color == "cyan") {
        colorCode += "36m";
    }
    else if (color == "white") {
        colorCode += "37m";
    }
    else {
        colorCode += "0m";
    }
    // 0m表示重置颜色
    if (isAdmin()) {
        std::cout << text;
    }
    else {
        std::cout << colorCode << text << "\033[0m";
    }
    if (enter) {
        std::cout << "\n";
    }
}
