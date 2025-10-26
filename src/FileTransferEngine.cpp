#include "FileTransferEngine.hpp"
#include "CompressionHelper.hpp"
#include "Logger.hpp"
#include <fstream>
#include <filesystem>
#include <cstring>
#include <winsock2.h>

namespace fs = std::filesystem;

bool FileTransferEngine::upload(const std::string& filePath, int socket, long offset,
    const std::string& username, ProgressCallback progress, bool compress)
{
    if (!fs::exists(filePath)) {
        Logger::error("File not found: " + filePath);
        return false;
    }
    std::string pathToSend = filePath;

    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        Logger::error("Unable to open file: " + filePath);
        return false;
    }

    if (compress) {
        pathToSend = filePath + ".gz";
        if (!CompressionHelper::compressFile(filePath, pathToSend))
            return false;
    }
    const auto totalSize = fs::file_size(filePath);

    // Send UPLOAD command first
    std::string fileName = fs::path(pathToSend).filename().string();
    std::string command = "UPLOAD " +
        fs::path(pathToSend).filename().string() + " " +
        std::to_string(fs::file_size(pathToSend)) + " " +
        std::to_string(offset) + " " +
        username + " " +
        (compress ? "1" : "0") + "\n";
    if (!sendAll(socket, command.c_str(), command.size())) {
        Logger::error("Failed to send upload command.");
        return false;
    }

    if (offset > 0 && offset < totalSize) {
        Logger::info("Resuming upload from offset " + std::to_string(offset));
        file.seekg(offset);
    }

    char buffer[CHUNK_SIZE];
    long bytesSent = offset;

    while (!file.eof()) {
        file.read(buffer, CHUNK_SIZE);
        std::streamsize bytesRead = file.gcount();
        if (bytesRead <= 0) break;

        if (!sendAll(socket, buffer, static_cast<size_t>(bytesRead))) {
            Logger::error("Upload interrupted.");
            return false;
        }

        bytesSent += bytesRead;
        if (progress) progress((bytesSent * 100.0) / totalSize);
    }

    Logger::info("Upload completed: " + filePath);
    return true;
}

bool FileTransferEngine::download(const std::string& fileName, int socket, long offset,
    const std::string& username, ProgressCallback progress, bool decompress)
{

    // Send DOWNLOAD command first
    std::string command = "DOWNLOAD " + fileName + " " + std::to_string(offset) + " " + username + " " + (decompress ? "1" : "0") + "\n";

    if (!sendAll(socket, command.c_str(), command.size())) {
        Logger::error("Failed to send download command.");
        return false;
    }

    const std::string tempPath = "downloads/temp_" + fileName;

    fs::create_directories("downloads");

    std::ofstream file;
    if (offset > 0 && fs::exists(tempPath)) {
        Logger::info("Resuming download of " + fileName + " from offset " + std::to_string(offset));
        file.open(tempPath, std::ios::binary | std::ios::app);
    }
    else {
        file.open(tempPath, std::ios::binary);
    }

    if (!file.is_open()) {
        Logger::error("Failed to open temporary file: " + tempPath);
        return false;
    }
    char buffer[CHUNK_SIZE];
    long bytesReceived = offset;

    while (true) {
        int received = recv(socket, buffer, CHUNK_SIZE, 0);
        if (received <= 0) break;

        file.write(buffer, received);
        bytesReceived += received;

        if (progress && bytesReceived % (CHUNK_SIZE * 2) == 0)
            progress((double)bytesReceived); // caller will compute %
    }

    file.close();
    // Decompress if needed
    const std::string localPath = "downloads/" + fileName;
    if (decompress) {
        if (!CompressionHelper::decompressFile(tempPath, localPath)) {
            Logger::error("Failed to decompress file: " + tempPath);
            return false;
        }
        fs::remove(tempPath); // remove temporary compressed file
    }
    else {
        fs::rename(tempPath, localPath); // move temp file to final name
    }
    Logger::info("Download completed: " + fileName);
    return true;
}

bool FileTransferEngine::sendAll(int socket, const char* buffer, size_t length)
{
    size_t totalSent = 0;
    while (totalSent < length) {
        int sent = send(socket, buffer + totalSent, (int)(length - totalSent), 0);
        if (sent == SOCKET_ERROR) {
            Logger::info( "FileTransferEngine send failed");
            return false;
        }
        totalSent += sent;
    }
    return true;
}

bool FileTransferEngine::recvAll(int socket, char* buffer, size_t length)
{
    size_t totalReceived = 0;
    while (totalReceived < length) {
        int received = recv(socket, buffer + totalReceived, (int)(length - totalReceived), 0);
        if (received <= 0) return false;
        totalReceived += received;
    }
    return true;
}