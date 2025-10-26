#pragma once
#include <string>
#include <atomic>
#include <unordered_map>

class ClientApp {
public:
    explicit ClientApp(const std::string& configPath);

    bool connectToServer();                         // Connect to TCP server
    bool uploadFile(const std::string& filePath, const std::string& user, bool compress = false);
    bool downloadFile(const std::string& fileName, const std::string& user, bool compress = false, bool resume = false);
    bool queryMetadata(const std::string& fileName); // Ask server for metadata
    void disconnect();
    void setServerAddress(const std::string& ip) { serverAddress_ = ip; }
    void setServerPort(int port) { serverPort_ = port; }
    bool isConnected() const { return connected_; }

private:
    bool loadConfig();                              // Load config (IP, port, etc.)
    long getResumeOffset(const std::string& fileName);
    void saveResumeOffset(const std::string& fileName, long offset);
    void clearResumeData(const std::string& fileName);
    std::string configPath_;
    std::string serverAddress_ = "127.0.0.1";
    int serverPort_{2121};
    int clientSocket_{-1};
    std::atomic<bool> connected_{ false };
    std::unordered_map<std::string, long> resumeMap_;  // in-memory resume offsets
};
