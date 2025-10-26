#include "MetadataManager.hpp"
#include <iostream>
#include <ctime>
#include "Logger.hpp"
#include <filesystem>

MetadataManager::MetadataManager(const std::string& dbPath) {
    if (sqlite3_open(dbPath.c_str(), &db_) != SQLITE_OK) {
        Logger::info("[DB] Failed to open DB: " + std::string(sqlite3_errmsg(db_)));
        db_ = nullptr;
    }
    else {
        initialize();
    }
}

MetadataManager::~MetadataManager() {
    if (db_) sqlite3_close(db_);
}

void MetadataManager::initialize() {
    const char* createTablesSQL = R"(
        CREATE TABLE IF NOT EXISTS files (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            filename TEXT UNIQUE,
            size INTEGER,
            upload_timestamp TEXT,
            uploader TEXT,
            download_count INTEGER DEFAULT 0
        );

        CREATE TABLE IF NOT EXISTS downloads (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            file_id INTEGER,
            downloader TEXT,
            timestamp TEXT DEFAULT (datetime('now')),
            FOREIGN KEY (file_id) REFERENCES files (id)
        );
    )";

    char* errMsg = nullptr;
    if (sqlite3_exec(db_, createTablesSQL, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        Logger::info(std::string("Error creating tables: ") + errMsg);
        sqlite3_free(errMsg);
    }
    else {
        Logger::info("[DB] Metadata tables ready.");
    }
}

bool MetadataManager::addFileRecord(const std::string& filename, long filesize, const std::string& uploader) {
    std::time_t now = std::time(nullptr);
    std::string timestamp = std::asctime(std::localtime(&now));
    timestamp.pop_back();

    const char* sql = R"(
        INSERT OR REPLACE INTO files (filename, size, upload_timestamp, uploader)
        VALUES (?, ?, ?, ?);
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        Logger::info("DB prepare failed: " + std::string(sqlite3_errmsg(db_)));
        return false;
    }

    sqlite3_bind_text(stmt, 1, filename.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, filesize);
    sqlite3_bind_text(stmt, 3, timestamp.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, uploader.c_str(), -1, SQLITE_TRANSIENT);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    if (success)
        Logger::info("[DB] File record added/updated: " + filename);

    return success;
}

void MetadataManager::updateFileMetadata(const std::string& fileName, const std::string& uploader, long size) {
    sqlite3_stmt* stmt = nullptr;

    const char* sql = R"(
        INSERT INTO files (filename, uploader, size, upload_timestamp, download_count)
        VALUES (?, ?, ?, datetime('now'), 0)
        ON CONFLICT(filename) DO UPDATE SET
            uploader = excluded.uploader,
            size = excluded.size,
            upload_timestamp = excluded.upload_timestamp;
    )";

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        Logger::info("Failed to prepare insert/update statement: " + std::string(sqlite3_errmsg(db_)));
        return;
    }

    sqlite3_bind_text(stmt, 1, fileName.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, uploader.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, size);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        Logger::info("Failed to execute insert/update: " + std::string(sqlite3_errmsg(db_)));
    }
    else {
        Logger::info("[DB] File metadata updated: " + fileName);
    }

    sqlite3_finalize(stmt);
}

bool MetadataManager::updateDownloadRecord(const std::string& filename, const std::string& downloader) {
    // Increment download count
    const char* sql1 = "UPDATE files SET download_count = download_count + 1 WHERE filename = ?;";
    sqlite3_stmt* stmt1;

    if (sqlite3_prepare_v2(db_, sql1, -1, &stmt1, nullptr) != SQLITE_OK) {
        Logger::info("Prepare failed (update count): " + std::string(sqlite3_errmsg(db_)));
        return false;
    }

    sqlite3_bind_text(stmt1, 1, filename.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt1);
    sqlite3_finalize(stmt1);

    // Insert download record
    const char* sql2 = R"(
        INSERT INTO downloads (file_id, downloader)
        SELECT id, ? FROM files WHERE filename = ?;
    )";

    sqlite3_stmt* stmt2;
    if (sqlite3_prepare_v2(db_, sql2, -1, &stmt2, nullptr) != SQLITE_OK) {
        Logger::info("Prepare failed (insert download): " + std::string(sqlite3_errmsg(db_)));
        return false;
    }

    sqlite3_bind_text(stmt2, 1, downloader.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt2, 2, filename.c_str(), -1, SQLITE_TRANSIENT);
    bool success = (sqlite3_step(stmt2) == SQLITE_DONE);
    sqlite3_finalize(stmt2);

    if (success)
        Logger::info("[DB] Recorded download of " + filename + " by " + downloader);

    return success;
}
/*
std::tuple<long, std::string, std::string, int> MetadataManager::getFileMetadata(const std::string& filename) {
    const char* sql = R"(
        SELECT size, upload_timestamp, uploader, download_count
        FROM files WHERE filename = ?;
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return { 0, "", "", 0 };

    sqlite3_bind_text(stmt, 1, filename.c_str(), -1, SQLITE_TRANSIENT);

    long size = 0; std::string ts, uploader; int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        size = sqlite3_column_int64(stmt, 0);
        ts = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        uploader = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        count = sqlite3_column_int(stmt, 3);
    }

    sqlite3_finalize(stmt);
    return { size, ts, uploader, count };
}
*/
std::vector<std::tuple<std::string, std::string>> MetadataManager::getDownloaders(const std::string& filename) {
    std::vector<std::tuple<std::string, std::string>> result;
    const char* sql = R"(
        SELECT d.downloader, d.timestamp
        FROM downloads d
        JOIN files f ON f.id = d.file_id
        WHERE f.filename = ?
        ORDER BY d.timestamp DESC;
    )";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return result;

    sqlite3_bind_text(stmt, 1, filename.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string user = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::string time = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        result.emplace_back(user, time);
    }

    sqlite3_finalize(stmt);
    return result;
}
std::vector<std::string> MetadataManager::getAllFileNames() {
    std::vector<std::string> names;
    if (!db_) return names;

    const char* sql = "SELECT filename FROM files ORDER BY upload_timestamp DESC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_free((char*)sqlite3_errmsg(db_));
        return names;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* text = sqlite3_column_text(stmt, 0);
        if (text) names.emplace_back(reinterpret_cast<const char*>(text));
    }
    sqlite3_finalize(stmt);
    return names;
}/*
FileMetadata MetadataManager::getFileMetadataRecord(const std::string& filename) {
    FileMetadata meta;
    Logger::info("first line.");
    if (!db_) return meta;
    Logger::info("first line.");
    const char* sql = R"(
        SELECT filename, filesize, upload_timestamp, uploader, download_count
        FROM files WHERE filename = ? LIMIT 1;
    )";
    Logger::info("first line.");
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_free((char*)sqlite3_errmsg(db_));
        return meta;
    }
    Logger::info("first line.");
    sqlite3_bind_text(stmt, 1, filename.c_str(), -1, SQLITE_TRANSIENT);
    Logger::info("first line.");
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* fn = sqlite3_column_text(stmt, 0);
        meta.fileName = fn ? reinterpret_cast<const char*>(fn) : "";
        meta.fileSize = sqlite3_column_int64(stmt, 1);
        const unsigned char* ts = sqlite3_column_text(stmt, 2);
        meta.uploadTimestamp = ts ? reinterpret_cast<const char*>(ts) : "";
        const unsigned char* upl = sqlite3_column_text(stmt, 3);
        meta.uploader = upl ? reinterpret_cast<const char*>(upl) : "";
        meta.downloadCount = sqlite3_column_int(stmt, 4);
        // storagePath optional: build from known server storage path if needed
    }
    Logger::info("first line.");
    sqlite3_finalize(stmt);
    return meta;
}*/
FileMetadata MetadataManager::getFileMetadataRecord(const std::string& filename) {
    FileMetadata meta;
    if (!db_) return meta;

    const char* sql = R"(
        SELECT filename, size, upload_timestamp, uploader, download_count
        FROM files WHERE filename = ? LIMIT 1;
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "[DB] Failed to prepare statement: " << sqlite3_errmsg(db_) << std::endl;
        return meta;
    }

    rc = sqlite3_bind_text(stmt, 1, filename.c_str(), -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        std::cerr << "[DB] Failed to bind filename: " << sqlite3_errmsg(db_) << std::endl;
        sqlite3_finalize(stmt);
        return meta;
    }

    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        const unsigned char* fn = sqlite3_column_text(stmt, 0);
        meta.fileName = fn ? reinterpret_cast<const char*>(fn) : "";
        meta.fileSize = sqlite3_column_int64(stmt, 1);
        const unsigned char* ts = sqlite3_column_text(stmt, 2);
        meta.uploadTimestamp = ts ? reinterpret_cast<const char*>(ts) : "";
        const unsigned char* upl = sqlite3_column_text(stmt, 3);
        meta.uploader = upl ? reinterpret_cast<const char*>(upl) : "";
        meta.downloadCount = sqlite3_column_int(stmt, 4);
    }
    else if (rc != SQLITE_DONE) {
        std::cerr << "[DB] Failed to step statement: " << sqlite3_errmsg(db_) << std::endl;
    }

    sqlite3_finalize(stmt);
    return meta;
}
