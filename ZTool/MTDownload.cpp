#include "Downloader.h"
#include <iostream>
#include <sstream>
#include <filesystem>
#include <iomanip>

Downloader::Downloader(const std::string& url, const std::string& outputFile, long numThreads)
    : url(url), outputFile(outputFile), numThreads(numThreads), completedChunks(0) {
    tempFiles.resize(numThreads), downloadedBytes.resize(numThreads);

    // downloadBytes:ÒÑÆúÓÃ
}

Downloader::~Downloader() {
    for (const auto& tempFile : tempFiles) {
        if (!tempFile.empty()) {
            std::filesystem::remove(tempFile);
        }
    }
}

void Downloader::start() {
    size_t fileSize = getFileSize(url);
    if (fileSize <= 0) {
        std::cerr << "Failed to get file size. Please check the URL and your network connection." << std::endl;
        return;
    }

    size_t chunkSize = fileSize / numThreads;
    for (int i = 0; i < numThreads; ++i) {
        size_t startByte = i * chunkSize;
        size_t endByte = (i == numThreads - 1) ? fileSize - 1 : startByte + chunkSize - 1;

        std::ostringstream tempFileName;
        tempFileName << outputFile << ".part" << i;
        tempFiles[i] = tempFileName.str();

        threads.emplace_back(&Downloader::downloadChunk, this, i, startByte, endByte);
    }
}

void Downloader::waitForCompletion() {
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    std::ofstream output(outputFile, std::ios::binary);
    for (const auto& tempFile : tempFiles) {
        std::ifstream input(tempFile, std::ios::binary);
        output << input.rdbuf();
    }

    std::cout << "Download completed: " << outputFile << std::endl;
}

void Downloader::downloadChunk(size_t chunkIndex, size_t startByte, size_t endByte) {
    HINTERNET hInternet = InternetOpenA("Downloader", INTERNET_OPEN_TYPE_DIRECT, nullptr, nullptr, 0);
    if (!hInternet) {
        std::cerr << "Failed to open internet connection. Error code: " << GetLastError() << std::endl;
        return;
    }

    HINTERNET hUrl = InternetOpenUrlA(hInternet, url.c_str(), nullptr, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hUrl) {
        std::cerr << "Failed to open URL. Error code: " << GetLastError() << std::endl;
        InternetCloseHandle(hInternet);
        return;
    }

    std::ostringstream range;
    range << "bytes=" << startByte << "-" << endByte;
    HttpAddRequestHeadersA(hUrl, ("Range: " + range.str()).c_str(), -1, HTTP_ADDREQ_FLAG_ADD);

    std::ofstream outFile(tempFiles[chunkIndex], std::ios::binary);
    char buffer[4096];
    DWORD bytesRead;
    size_t totalBytes = endByte - startByte + 1;

    while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead)) {
        if (bytesRead == 0) break;
        outFile.write(buffer, bytesRead);

        downloadedBytes[chunkIndex] += bytesRead;
        printProgress(chunkIndex, downloadedBytes[chunkIndex], totalBytes);
    }

    outFile.close();
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);

    completedChunks++;
    std::cout << "Chunk " << chunkIndex << " completed (" << completedChunks << "/" << numThreads << ")" << std::endl;
}

size_t Downloader::getFileSize(const std::string& url) {
    HINTERNET hInternet = InternetOpenA("Downloader", INTERNET_OPEN_TYPE_DIRECT, nullptr, nullptr, 0);
    if (!hInternet) {
        std::cerr << "Failed to open internet connection. Error code: " << GetLastError() << std::endl;
        return 0;
    }

    HINTERNET hUrl = InternetOpenUrlA(hInternet, url.c_str(), nullptr, 0, INTERNET_FLAG_RELOAD, 0);
    if (!hUrl) {
        std::cerr << "Failed to open URL. Error code: " << GetLastError() << std::endl;
        InternetCloseHandle(hInternet);
        return 0;
    }

    char buffer[4096];
    DWORD bufferSize = sizeof(buffer);
    if (!HttpQueryInfoA(hUrl, HTTP_QUERY_CONTENT_LENGTH, buffer, &bufferSize, nullptr)) {
        std::cerr << "Failed to query content length. Error code: " << GetLastError() << std::endl;
        InternetCloseHandle(hUrl);
        InternetCloseHandle(hInternet);
        return 0;
    }

    size_t fileSize = 0;
    try {
        fileSize = std::stoul(buffer);
    }
    catch (const std::invalid_argument& e) {
        std::cerr << "Invalid content length: " << e.what() << std::endl;
        fileSize = 0;
    }
    catch (const std::out_of_range& e) {
        std::cerr << "Content length out of range: " << e.what() << std::endl;
        fileSize = 0;
    }

    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);
    return fileSize;
}

void Downloader::printProgress(size_t chunkIndex, size_t downloadedBytes, size_t totalBytes) {
    std::cout << "There are " << getFileSize(url) / numThreads << " threads starts." << std::endl;
    std::cout << "File size: " << getFileSize(url);
}