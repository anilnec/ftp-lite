#include "ClientApp.hpp"
#include <iostream>
#include "Logger.hpp"
#include <fstream>
#include <filesystem>
#include "FileTransferEngine.hpp"
#include <nlohmann/json.hpp>
#include <winsock2.h>

namespace fs = std::filesystem;
using json = nlohmann::json;

ClientApp::ClientApp(const std::string& configPath)
    : configPath_(configPath)
{
    loadConfig();
}
bool ClientApp::loadConfig() {
    // TODO: parse configPath_ (JSON)
    std::ifstream file(configPath_);
    if (!file.is_open()) return false;

    json cfg;
    file >> cfg;
    if (cfg.contains("server_ip")) serverAddress_ = cfg["server_ip"];
    if (cfg.contains("server_port")) serverPort_ = cfg["server_port"];
    return true;
}


bool ClientApp::connectToServer() {
    if (connected_) return true;
    std::cout << "Connecting to " << serverAddress_ << ":" << serverPort_ << std::endl;
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        Logger::info("WSAStartup failed");
        return false;
    }

    clientSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket_ == INVALID_SOCKET) {
        Logger::info("Failed to create socket");
        WSACleanup();
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort_);
    serverAddr.sin_addr.s_addr = inet_addr(serverAddress_.c_str());

    if (connect(clientSocket_, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        Logger::info("Failed to connect to server");
        closesocket(clientSocket_);
        WSACleanup();
        return false;
    }

    connected_ = true;
    return true;
}

bool ClientApp::uploadFile(const std::string& filePath, const std::string& username, bool compress) {
    if (!connected_) return false;
    FileTransferEngine engine;

    long offset = getResumeOffset(filePath);

    std::cout << "Uploading file: " << filePath << " compress=" << compress << std::endl;
    bool success = engine.upload(filePath, clientSocket_, offset, username,
        [&](double percent) {
            Logger::info("Upload progress: " + std::to_string((int)percent) + "%");
            saveResumeOffset(filePath, (long)((percent / 100.0) * fs::file_size(filePath)));
        }, compress);
    if(success)
        clearResumeData(filePath);
    return true;
}

bool ClientApp::downloadFile(const std::string& fileName, const std::string& username, bool compress, bool resume)
{
    if (!connected_) return false;

    FileTransferEngine engine;
    long offset = resume ? getResumeOffset(fileName) : 0;

    bool success = engine.download(fileName, clientSocket_, offset, username,[&](double bytesReceived) {
        // Server should send file size first — handle in your protocol
        saveResumeOffset(fileName, (long)bytesReceived);
        }, compress);

    if (success) clearResumeData(fileName);
    return success;
}
bool ClientApp::queryMetadata(const std::string& fileName) {
    if (!connected_) return false;
    std::cout << "Querying metadata: " << fileName << std::endl;
    return true;
}

void ClientApp::disconnect() {
    if (connected_) {
        closesocket(clientSocket_);
        WSACleanup();
        connected_ = false;
    }    
    std::cout << "Disconnected." << std::endl;
}

long ClientApp::getResumeOffset(const std::string& fileName) {
    std::ifstream f("resume.json");
    if (!f.is_open()) return 0;
    json j; f >> j;
    if (j.contains(fileName)) return j[fileName].get<long>();
    return 0;
}
void ClientApp::saveResumeOffset(const std::string& fileName, long offset) {
    json j;
    std::ifstream in("resume.json");
    if (in.is_open()) in >> j;
    j[fileName] = offset;
    std::ofstream out("resume.json");
    out << j.dump(4);
}
void ClientApp::clearResumeData(const std::string& fileName) {
    json j;
    std::ifstream in("resume.json");
    if (in.is_open()) in >> j;
    if (j.contains(fileName)) j.erase(fileName);
    std::ofstream out("resume.json");
    out << j.dump(4);
}

