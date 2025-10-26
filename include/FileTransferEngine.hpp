#pragma once
#include <string>
#include <atomic>
#include <functional>

class FileTransferEngine {
public:
    using ProgressCallback = std::function<void(double)>; 
    FileTransferEngine() = default;
    bool upload(const std::string& filePath, int socket, long offset, const std::string& username, ProgressCallback progress, bool compress = false);
    bool download(const std::string& fileName, int socket, long offset, const std::string& username, ProgressCallback progress, bool decompress = false);

    bool sendAll(int socket, const char* buffer, size_t length);
    bool recvAll(int socket, char* buffer, size_t length);

    static const size_t CHUNK_SIZE = 64 * 1024; // 64KB chunks

};