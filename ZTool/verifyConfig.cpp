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
        // ����Ƿ�Ϊ��Чconfig�ļ�
        std::unordered_map<std::string, std::string> config;
        while (std::getline(file, lineStr)) {
            size_t pos = lineStr.find(":");
            if (pos != std::string::npos) {
                std::string key = lineStr.substr(0, pos);
                std::string value = lineStr.substr(pos + 1);
                config[key] = value;
            }
            else {
                std::cerr << "��Ч�������У�" << lineStr << std::endl;
                return false;
            }
        }
        // ����Ҫ�ı����Ƿ����
        if (config.find("version") == config.end()) {
            std::cerr << "ȱ�ٰ汾��" << std::endl;
            return false;
        }
        if (config.find("colortext") == config.end()) {
            std::cerr << "ȱ����ɫ�ı�" << std::endl;
            return false;
        }
        // ������������
        if (config["version"] != "1.0") {
            std::cerr << "��Ч�İ汾�ţ�" << config["version"] << std::endl;
            return false;
        }
        if (config["colortext"] != "true" && config["colortext"] != "false") {
            std::cerr << "��Ч����ɫ�ı���" << config["colortext"] << std::endl;
            return false;
        }
    }
    else {
        // std::cerr << "�޷����ļ�" << std::endl;
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
        std::cerr << "�޷������ļ�" << std::endl;
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
            std::cerr << "��Ч�������У�" << lineStr << std::endl;
        }
    }
    return config["version"];
}
