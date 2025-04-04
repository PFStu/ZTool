#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#define WIN32_LEAN_AND_MEAN // ���� WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wininet.h> // ȷ�� wininet.h �� windows.h ֮��

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <memory>

class Downloader {
public:
    Downloader(const std::string& url, const std::string& outputFile, long numThreads = 4);
    ~Downloader();

    void start();
    void waitForCompletion();

private:
    void downloadChunk(size_t chunkIndex, size_t startByte, size_t endByte);
    size_t getFileSize(const std::string& url);
    void printProgress(size_t chunkIndex, size_t downloadedBytes, size_t totalBytes);

    std::string url;
    std::string outputFile;
    long numThreads;
    std::vector<std::thread> threads;
    std::atomic<size_t> completedChunks;
    std::vector<std::string> tempFiles;
    std::vector<long> downloadedBytes;
};

#endif // DOWNLOADER_H