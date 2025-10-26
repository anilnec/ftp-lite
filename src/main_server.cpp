#include <QApplication>
#include "ServerWindow.hpp"
#include "ServerApp.hpp"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    std::string configPath = (argc > 1) ? argv[1] : "config/server_config.json";

    // Create the server logic layer with config
    auto serverApp = std::make_shared<ServerApp>(configPath);

    ServerWindow window;
    window.setServerApp(serverApp);
    window.setWindowTitle("FTP-Lite Server");
    window.resize(600, 400);
    window.show();

    return app.exec();
}
