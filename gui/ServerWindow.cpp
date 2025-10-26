#include "ServerWindow.hpp"
#include "ui_ServerWindow.h"
#include <QMessageBox>
#include <QDebug>
#include <Logger.hpp>

ServerWindow::ServerWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::ServerWindow), metadataDB_("server_metadata.db")
{
    ui->setupUi(this);

    ui->metadataTable->setColumnCount(2);
    ui->metadataTable->setHorizontalHeaderLabels({ "Field", "Value" });
    ui->metadataTable->horizontalHeader()->setStretchLastSection(true);

    connect(ui->startButton, &QPushButton::clicked, this, &ServerWindow::onStartServerClicked);
    connect(ui->stopButton, &QPushButton::clicked, this, &ServerWindow::onStopServerClicked);
   // connect(ui->fileListWidget, &QTreeWidget::itemClicked, this, &ServerWindow::onFileSelected);
    connect(ui->refreshFilesButton, &QPushButton::clicked, this, &ServerWindow::refreshFileList);
    connect(ui->downloadHistoryButton, &QPushButton::clicked, this, &ServerWindow::onDownloadHistoryClicked);
    connect(ui->metadataButton, &QPushButton::clicked, this, &ServerWindow::onMetadataClicked);

    refreshFileList();
}

ServerWindow::~ServerWindow()
{
    if (serverThread_) {
        serverThread_->quit();
        serverThread_->wait();
        delete serverThread_;
    }
    delete ui;
}
void ServerWindow::setServerApp(std::shared_ptr<ServerApp> app) {
    serverApp_ = std::move(app);
}
/*
void ServerWindow::onStartServerClicked()
{
    if (serverThread_) {
        showMessage("Server already running.");
        return;
    }

    serverThread_ = new QThread(this);
    serverApp_ = std::make_shared<ServerApp>();
    serverApp_->moveToThread(serverThread_);

    connect(serverThread_, &QThread::started, [this]() {
        serverApp_->start();
        });

    connect(serverThread_, &QThread::finished, serverApp_.get(), &QObject::deleteLater);
    connect(serverApp_.get(), &ServerApp::logMessage, this, &ServerWindow::appendLogMessage);
    connect(serverApp_.get(), &ServerApp::clientConnected, this, &ServerWindow::onClientConnected);
    connect(serverApp_.get(), &ServerApp::clientDisconnected, this, &ServerWindow::onClientDisconnected);

    serverThread_->start();
    showMessage("Server started successfully.");
}
*/
void ServerWindow::onStopServerClicked()
{
    if (serverApp_) serverApp_->stop();
    if (serverThread_) {
        serverThread_->quit();
        serverThread_->wait();
        delete serverThread_;
        serverThread_ = nullptr;
    }
    showMessage("Server stopped.");
    ui->statusLabel->setText("Stopped");
}

void ServerWindow::refreshFileList() {
    ui->fileListWidget->clear();
    auto files = metadataDB_.getAllFileNames();
    for (const auto& name : files) {
        auto* item = new QTreeWidgetItem();
        item->setText(0, QString::fromStdString(name));
        ui->fileListWidget->addTopLevelItem(item);
    }
}

void ServerWindow::onMetadataClicked() {
    auto* item = ui->fileListWidget->currentItem();
    if (!item) return;
    std::string fileName = item->text(0).toStdString();

    FileMetadata meta = metadataDB_.getFileMetadataRecord(fileName);

    ui->metadataTable->clearContents();
    ui->metadataTable->setRowCount(0);

    auto addRow = [&](const QString& field, const QString& value) {
        int r = ui->metadataTable->rowCount();
        ui->metadataTable->insertRow(r);
        ui->metadataTable->setItem(r, 0, new QTableWidgetItem(field));
        ui->metadataTable->setItem(r, 1, new QTableWidgetItem(value));
        };

    addRow("File Name", QString::fromStdString(meta.fileName));
    addRow("Uploader", QString::fromStdString(meta.uploader));
    addRow("Size (bytes)", QString::number(meta.fileSize));
    addRow("Size (KB)", QString::number(meta.fileSize / 1024.0, 'f', 2));
    addRow("Last Updated", QString::fromStdString(meta.uploadTimestamp));
    addRow("Download Count", QString::number(meta.downloadCount));

    ui->metadataTable->resizeColumnsToContents();
}
void ServerWindow::onDownloadHistoryClicked() {
    auto* item = ui->fileListWidget->currentItem();
    if (!item) return;
    std::string fileName = item->text(0).toStdString();

    ui->metadataTable->clearContents();
    ui->metadataTable->setRowCount(0);

    auto history = metadataDB_.getDownloaders(fileName);
    for (const auto& [user, time] : history) {
        int row = ui->metadataTable->rowCount();
        ui->metadataTable->insertRow(row);
        ui->metadataTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(user)));
        ui->metadataTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(time)));
    }
    ui->metadataTable->resizeColumnsToContents();
}

/*
void ServerWindow::onFileSelected(QTreeWidgetItem* item, int coloumn)
{
    Logger::info("first line.");
    if (!item) return;
    Logger::info("first line.");
    std::string fileName = item->text(0).toStdString();
    QMessageBox::information(this, "Metadata", " metadata Request recieved for file: " + QString::fromStdString(fileName));
    FileMetadata meta = metadataDB_.getFileMetadataRecord(fileName);
    if (meta.fileName.empty()) {
        QMessageBox::warning(this, "Metadata", "No metadata found for file: " + QString::fromStdString(fileName));
        return;
    }
    Logger::info("Before Setting coloumn.");
    ui->metadataTable->setColumnCount(2);
    ui->metadataTable->setRowCount(0);
    Logger::info("after Setting coloumn.");
    auto addRow = [&](const QString& field, const QString& value) {
        int r = ui->metadataTable->rowCount();
        ui->metadataTable->insertRow(r);
        ui->metadataTable->setItem(r, 0, new QTableWidgetItem(field));
        ui->metadataTable->setItem(r, 1, new QTableWidgetItem(value));
        };
    Logger::info("Going to add row.");
    addRow("File Name", QString::fromStdString(meta.fileName));
    addRow("Uploader", QString::fromStdString(meta.uploader));
    addRow("Size (bytes)", QString::number(meta.fileSize));
    addRow("Uploaded On", QString::fromStdString(meta.uploadTimestamp));
    addRow("Download Count", QString::number(meta.downloadCount));
}
*/
void ServerWindow::appendLogMessage(const QString& msg)
{
    
    ui->logTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded); // Show scrollbar if needed
    ui->logTextEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->logTextEdit->append(msg);
}

void ServerWindow::onClientConnected(const QString& addr)
{
    appendLogMessage(QString("[+] Client connected: %1").arg(addr));
}

void ServerWindow::onClientDisconnected(const QString& addr)
{
    appendLogMessage(QString("[-] Client disconnected: %1").arg(addr));
}

void ServerWindow::showMessage(const QString& msg)
{
    QMessageBox::information(this, "FTP-Lite Server", msg);
}
void ServerWindow::onStartServerClicked()
{
    if (serverThread_) {
        showMessage("Server already running.");
        return;
    }

    serverThread_ = new QThread(this);
    serverApp_ = std::make_shared<ServerApp>();
    serverApp_->moveToThread(serverThread_);

    connect(serverThread_, &QThread::started, [this]() {
        serverApp_->start();
        });

    connect(serverThread_, &QThread::finished, serverApp_.get(), &QObject::deleteLater);
    connect(serverApp_.get(), &ServerApp::logMessage, this, &ServerWindow::appendLogMessage);
    connect(serverApp_.get(), &ServerApp::clientConnected, this, &ServerWindow::onClientConnected);
    connect(serverApp_.get(), &ServerApp::clientDisconnected, this, &ServerWindow::onClientDisconnected);

    // Connect auto-refresh
    connect(serverApp_.get(), &ServerApp::fileUploaded, this, [this](const QString& f) {
        appendLogMessage(QString("[UI] File uploaded: %1").arg(f));
        refreshFileList();
        });

    serverThread_->start();
    showMessage("Server started successfully.");
    ui->statusLabel->setText("Started");

}
