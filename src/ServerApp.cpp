#include "ServerApp.hpp"
#include "FileTransferEngine.hpp"
#include "MetadataManager.hpp"
#include "CommandParser.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <QMetaObject>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "CompressionHelper.hpp"


namespace fs = std::filesystem;

ServerApp::ServerApp(QObject* parent)
    : QObject(parent)
{
}

ServerApp::~ServerApp()
{
    stop();
}

bool ServerApp::loadConfig()
{
    std::ifstream file(configPath_);
    if (!file.is_open()) {
        emit logMessage("[Server] Config not found. Using defaults (port 2121, storage folder).");
        return true;
    }

    try {
        json cfg;
        file >> cfg;
        file.close();
        if (cfg.contains("serverPort")) serverPort_ = cfg["serverPort"];
        if (cfg.contains("storagePath")) storagePath_ = cfg["storagePath"];
        emit logMessage(QString("[Server] Config loaded. Port=%1, Storage=%2")
            .arg(serverPort_).arg(QString::fromStdString(storagePath_)));
        return true;
    }
    catch (std::exception& e) {
        emit logMessage(QString("[Server] Config parse error: %1").arg(e.what()));
        return false;
    }
}

bool ServerApp::start()
{
    if (!loadConfig()) return false;

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        emit logMessage("[Server] WSAStartup failed!");
        return false;
    }

    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ == INVALID_SOCKET) {
        emit logMessage("[Server] Failed to create socket.");
        WSACleanup();
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(serverPort_);

    BOOL opt = TRUE;
    setsockopt(serverSocket_, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (const char*)&opt, sizeof(opt));

    if (bind(serverSocket_, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        emit logMessage("[Server] Bind failed! Port may be in use.");
        closesocket(serverSocket_);
        WSACleanup();
        return false;
    }

    if (listen(serverSocket_, SOMAXCONN) == SOCKET_ERROR) {
        emit logMessage("[Server] Listen failed!");
        closesocket(serverSocket_);
        WSACleanup();
        return false;
    }

    emit logMessage(QString("[Server] Listening on port %1...").arg(serverPort_));
    running_ = true;

    while (running_) {
        sockaddr_in clientAddr{};
        int clientLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket_, (sockaddr*)&clientAddr, &clientLen);
        if (clientSocket == INVALID_SOCKET) continue;

    //    char clientIp[INET_ADDRSTRLEN];
      //  inet_ntoa(AF_INET, &(clientAddr.sin_addr), clientIp, INET_ADDRSTRLEN);
        
        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
        std::cout << "Client connected: " << ipStr << std::endl;

        emit clientConnected(QString::fromUtf8(ipStr));

        std::thread(&ServerApp::handleClient, this, clientSocket).detach();
    }

    closesocket(serverSocket_);
    WSACleanup();
    emit logMessage("[Server] Server stopped.");
    return true;
}

void ServerApp::stop()
{
    running_ = false;
    if (serverSocket_ != INVALID_SOCKET) {
        closesocket(serverSocket_);
        serverSocket_ = INVALID_SOCKET;
    }
}

void ServerApp::handleClient(SOCKET clientSocket)
{
    FileTransferEngine engine;
    MetadataManager metadataDB("server_metadata.db");

    char buffer[1024];
    int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived <= 0) {
        emit logMessage("[Server] Client disconnected.");
        closesocket(clientSocket);
        return;
    }

    buffer[bytesReceived] = '\0';
    std::string commandStr(buffer);
    CommandParser parser(commandStr);
    std::string command = parser.getCommand();

    if (command == "UPLOAD") {
        std::string fileName = parser.getArg(0);
        std::string fileSizeStr = parser.getArg(1);
        std::string offsetStr = parser.getArg(2);
        std::string uploader = parser.getArg(3);
        std::string compressedStr = parser.getArg(4);

        bool compressed = (compressedStr == "1");
        size_t fileSize = std::stoull(fileSizeStr);
        size_t offset = std::stoull(offsetStr);

        std::string filePath = storagePath_ + "/" + fileName;
        std::ofstream outFile(filePath, std::ios::binary | std::ios::app);
        if (!outFile.is_open()) {
            emit logMessage(QString("[Server] Failed to open file for writing: %1")
                .arg(QString::fromStdString(filePath)));
            closesocket(clientSocket);
            return;
        }

        size_t totalReceived = offset;
        char chunk[FileTransferEngine::CHUNK_SIZE];
        while (totalReceived < fileSize) {
            int received = recv(clientSocket, chunk, FileTransferEngine::CHUNK_SIZE, 0);
            if (received <= 0) break;
            outFile.write(chunk, received);
            totalReceived += received;
        }
        outFile.close();

        if (compressed) {
            std::string decompressedPath = storagePath_ + "/" + fileName + "_decompressed";
            CompressionHelper::decompressFile(filePath, decompressedPath);
            fs::remove(filePath);
            fs::rename(decompressedPath, filePath);
        }

        metadataDB.updateFileMetadata(fileName, uploader, fileSize);
        emit logMessage(QString("[Server] Upload complete: %1 by %2")
            .arg(QString::fromStdString(fileName))
            .arg(QString::fromStdString(uploader)));

        // 🔔 Notify UI to refresh list
        emit fileUploaded(QString::fromStdString(fileName));
    }

    closesocket(clientSocket);
}
