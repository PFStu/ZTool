#include <string>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include "printColorText.h"
#include "verifyConfig.h"

bool verify() {
    std::string lineStr;
    std::ifstream file("config.cfg");
    if (file.is_open()) {
        // 检测是否为有效config文件
        std::unordered_map<std::string, std::string> config;
        while (std::getline(file, lineStr)) {
            size_t pos = lineStr.find(":");
            if (pos != std::string::npos) {
                std::string key = lineStr.substr(0, pos);
                std::string value = lineStr.substr(pos + 1);
                config[key] = value;
            }
            else {
                std::cerr << "无效的配置行：" << lineStr << std::endl;
                return false;
            }
        }
        // 检查必要的变量是否存在
        if (config.find("version") == config.end()) {
            std::cerr << "缺少版本号" << std::endl;
            return false;
        }
        if (config.find("colortext") == config.end()) {
            std::cerr << "缺少颜色文本" << std::endl;
            return false;
        }
        // 检查变量的类型
        if (config["version"] != "1.0") {
            std::cerr << "无效的版本号：" << config["version"] << std::endl;
            return false;
        }
        if (config["colortext"] != "true" && config["colortext"] != "false") {
            std::cerr << "无效的颜色文本：" << config["colortext"] << std::endl;
            return false;
        }
    }
    else {
        // std::cerr << "无法打开文件" << std::endl;
        return false;
    }
    std::unordered_map<std::string, std::string> config;
    int colortext = config["colortext"] == "true" ? 1 : 0;
    std::string version = config["version"];
    return true;
}

void createConfig() {
    std::ofstream file("config.cfg");
    if (file.is_open()) {
        file << "version:1.0" << std::endl;
        file << "colortext:true" << std::endl;
        file.close();
    }
    else {
        std::cerr << "无法创建文件" << std::endl;
    }
}

std::string getVersion() {
    std::ifstream file("config.cfg");
    std::string lineStr;
    std::unordered_map<std::string, std::string> config;
    while (std::getline(file, lineStr)) {
        size_t pos = lineStr.find(":");
        if (pos != std::string::npos) {
            std::string key = lineStr.substr(0, pos);
            std::string value = lineStr.substr(pos + 1);
            config[key] = value;
        }
        else {
            std::cerr << "无效的配置行：" << lineStr << std::endl;
        }
    }
    return config["version"];
}
