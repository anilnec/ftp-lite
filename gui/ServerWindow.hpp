#pragma once
#include <QMainWindow>
#include <QThread>
#include <QTreeWidgetItem>
#include "ServerApp.hpp"
#include "MetadataManager.hpp"

QT_BEGIN_NAMESPACE
namespace Ui { class ServerWindow; }
QT_END_NAMESPACE

class ServerWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ServerWindow(QWidget* parent = nullptr);
    ~ServerWindow();
    void setServerApp(std::shared_ptr<ServerApp> app);

private slots:
    void onStartServerClicked();
    void onStopServerClicked();
  //  void onFileSelected(QTreeWidgetItem* item, int column);
    void refreshFileList();
    void onMetadataClicked();
    void onDownloadHistoryClicked();

    void appendLogMessage(const QString& msg);
    void onClientConnected(const QString& addr);
    void onClientDisconnected(const QString& addr);

private:
    Ui::ServerWindow* ui;
    QThread* serverThread_ = nullptr;
    std::shared_ptr<ServerApp> serverApp_;
    MetadataManager metadataDB_;

    void showMessage(const QString& msg);
};
