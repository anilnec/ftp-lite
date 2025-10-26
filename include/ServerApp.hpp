#pragma once
#include <QObject>
#include <string>
#include <thread>
#include <atomic>
#include <winsock2.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class ServerApp : public QObject
{
    Q_OBJECT
public:
    explicit ServerApp(QObject* parent = nullptr);
    ServerApp(const std::string& configPath) : configPath_(configPath) {}
    ~ServerApp();

    bool start();   // called when thread starts
    void stop();

signals:
    void logMessage(const QString& msg);   // for UI logs
    void clientConnected(const QString& addr);
    void clientDisconnected(const QString& addr);
    void fileUploaded(const QString& fileName);

private:
    bool loadConfig();
    void handleClient(SOCKET clientSocket);

    std::atomic<bool> running_{ false };
    SOCKET serverSocket_ = INVALID_SOCKET;
    int serverPort_ = 2121;
    std::string storagePath_ = "storage";
    std::string configPath_ = "config/server_config.json";
};
