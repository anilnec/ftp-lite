#include "ClientWindow.hpp"
#include "ui_ClientWindow.h"
#include "ClientApp.hpp"
#include <QMessageBox>
#include <QFileDialog>
#include <iostream>

ClientWindow::ClientWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(std::make_unique<Ui::ClientWindow>())
{
    ui->setupUi(this);
    connect(ui->uploadButton, &QPushButton::clicked, this, &ClientWindow::onUploadClicked);
    connect(ui->downloadButton, &QPushButton::clicked, this, &ClientWindow::onDownloadClicked);
    connect(ui->metadataButton, &QPushButton::clicked, this, &ClientWindow::onMetadataClicked);
    connect(ui->connectButton, &QPushButton::clicked, this, &ClientWindow::onConnectClicked);
}

ClientWindow::~ClientWindow() = default;

void ClientWindow::setClientApp(std::shared_ptr<ClientApp> clientApp) {
    clientApp_ = std::move(clientApp);
}

std::string ClientWindow::getUsername() const {
    return ui->usernameEdit->text().toStdString();
}

void ClientWindow::onConnectClicked() {
    std::cout << "Connecting to FTP-Lite server..." << ui->serverAddressInput->text().toStdString() << ui->serverPortInput->text().toInt() << std::endl;
    if (!clientApp_) return;

    QString ip = ui->serverAddressInput->text();
    int port = ui->serverPortInput->text().toInt();

    clientApp_->setServerAddress(ip.toStdString());
    clientApp_->setServerPort(port);

    bool success = clientApp_->connectToServer();
    showMessage(success ? "Connected to server." : "Failed to connect to server!");
}


void ClientWindow::onUploadClicked() {
    if (!clientApp_ || !clientApp_->isConnected()) {
        showMessage("Please connect to the server first!");
        return;
    }


    QString filePath = QFileDialog::getOpenFileName(this, "Select File to Upload");
    if (!filePath.isEmpty()) {
        bool compress = ui->compressCheck->isChecked();
        std::string username = ui->usernameEdit->text().toStdString();
        bool success = clientApp_->uploadFile(filePath.toStdString(), username, compress);
        showMessage(success ? "Upload successful" : "Upload failed");
    }
}

void ClientWindow::onDownloadClicked() {
    if (!clientApp_ || !clientApp_->isConnected()) {
        showMessage("Please connect to the server first!");
        return;
    }
    std::string username = ui->usernameEdit->text().toStdString();
    QString fileName = ui->fileNameInput->text();
    bool compress = ui->compressCheck->isChecked();
    bool resume = ui->resumeCheck->isChecked();
    if (!fileName.isEmpty()) {
        bool success = clientApp_->downloadFile(fileName.toStdString(), username, compress, resume);
        showMessage(success ? "Download complete" : "Download failed");
    }
}

void ClientWindow::onMetadataClicked() {
    if (!clientApp_ || !clientApp_->isConnected()) {
        showMessage("Please connect to the server first!");
        return;
    }

    QString fileName = ui->fileNameInput->text();
    if (!fileName.isEmpty()) {
        bool success = clientApp_->queryMetadata(fileName.toStdString());
        showMessage(success ? "Metadata retrieved" : "Failed to get metadata");
    }
}

void ClientWindow::showMessage(const QString& msg) {
    QMessageBox::information(this, "FTP-Lite Client", msg);
}
