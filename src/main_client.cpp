#include <QApplication>
#include "ClientWindow.hpp"
#include "ClientApp.hpp"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    // Create backend logic
    auto clientApp = std::make_shared<ClientApp>("client_config.json");

    // Create GUI
    ClientWindow window;
    window.setClientApp(clientApp);
    window.setWindowTitle("FTP-Lite Client");
    window.resize(600, 400);
    window.show();

    return app.exec();
}
