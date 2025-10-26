#pragma once
#include <string>
#include <vector>
#include <tuple>
#include <sqlite3.h>

struct FileMetadata {
    std::string fileName;
    long fileSize = 0;
    std::string uploadTimestamp;
    std::string uploader;
    int downloadCount = 0;
    std::string storagePath;
};

class MetadataManager {
public:
    explicit MetadataManager(const std::string& dbPath);
    ~MetadataManager();

    void initialize();

    // CRUD / update
    //void insertOrUpdateFile(const std::string& fileName, size_t fileSize);
    bool addFileRecord(const std::string& filename, long filesize, const std::string& uploader);
    void updateFileMetadata(const std::string& fileName, const std::string& uploader, long size);
    //void incrementDownloadCount(const std::string& fileName, const std::string& user = "unknown");
    bool updateDownloadRecord(const std::string& filename, const std::string& downloader);

    // Metadata retrieval
    //std::tuple<long, std::string, std::string, int> getFileMetadata(const std::string& filename);
    std::vector<std::tuple<std::string, std::string>> getDownloaders(const std::string& filename);

    std::vector<std::string> getAllFileNames();
    FileMetadata getFileMetadataRecord(const std::string& filename);
      
private:
    sqlite3* db_ = nullptr;
};
