#pragma once
#include <QMainWindow>
#include <memory>

class ClientApp;

namespace Ui {
    class ClientWindow;
}

class ClientWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit ClientWindow(QWidget* parent = nullptr);
    ~ClientWindow();

    void setClientApp(std::shared_ptr<ClientApp> clientApp);
    std::string getUsername() const;
private slots:
    void onConnectClicked();
    void onUploadClicked();
    void onDownloadClicked();
    void onMetadataClicked();

private:
    void showMessage(const QString& msg);
    std::unique_ptr<Ui::ClientWindow> ui;
    std::shared_ptr<ClientApp> clientApp_;

};
