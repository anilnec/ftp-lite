**FTP-Lite**

FTP-Lite is a lightweight, cross-platform file transfer application designed for secure file uploads and downloads with server-side metadata management and optional compression support. It is implemented in C++ using Qt for GUI and SQLite for metadata storage.

******Features**
**Client**

       1. Upload and download files to/from the server.
        
       2. Resume interrupted file transfers.
        
       3. Optional gzip compression to save bandwidth.
        
       4. User authentication for file operations.
        
       5. GUI-based interface with username input and file selection.

**Server**

        1. Handles multiple client connections via TCP sockets.
        
        2. Manages file storage and metadata in SQLite.
        
        3. Tracks upload/download history with timestamps and users.
        
        4. Admin GUI to view uploaded files, metadata, and download history.
        
        5. Optional compression/decompression of files.

**Admin Functions**

        1.Refresh file list on server UI.
        
        2.View metadata of uploaded files (uploader, size, timestamps, download count).
        
        3.Track download history for sensitive files.

**Architecture**

The system follows a **Client-Server architecture:**

ClientWindow (GUI) --> ClientApp --> FileTransferEngine --> CompressionHelper
ClientApp <----TCP----> ServerApp <--> FileTransferEngine <--> CompressionHelper
ServerApp --> MetadataManager --> SQLite Database
ServerWindow (Admin GUI) --> ServerApp


**ClientApp**: Core client logic handling server communication.

**FileTransferEngine**: Handles file transfer and optional compression.

**CompressionHelper**: Compress/decompress files using gzip.

**ServerApp**: Manages client connections, processes commands, and interacts with MetadataManager.

**MetadataManager**: Maintains file metadata and download records in SQLite.

**ServerWindow**: GUI for admin to monitor server activity.

**Requirements**

        C++17 compatible compiler (MSVC recommended on Windows).
        
        Qt 6 for GUI.
        
        SQLite3 library for database.
        
        zlib for compression support.
        
        Windows or Linux platform.

**Build Instructions**

        Clone the repository
        
        git clone <repository_url>
        cd ftp_lite_studio
        
        
        Configure CMake
        
        mkdir build
        cd build
        cmake ..


Build the project

Windows: Open generated .sln in Visual Studio and build.

Linux:

make


Run the applications

Start the server first:

./ftp_lite_server


Launch client:

./ftp_lite_client

**Database Schema**
Files Table
CREATE TABLE IF NOT EXISTS files (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    filename TEXT,
    size INTEGER,
    uploader TEXT,
    upload_timestamp TEXT,
    download_count INTEGER DEFAULT 0
);

Downloads Table
CREATE TABLE IF NOT EXISTS downloads (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    file_id INTEGER,
    downloader TEXT,
    timestamp TEXT DEFAULT (datetime('now')),
    FOREIGN KEY (file_id) REFERENCES files(id)
);

**Usage**

Client

Enter username in GUI.

Select file to upload or download.

Check “Compress” option for upload/download to save bandwidth.

Monitor progress in the GUI.

Server

Start the server using ServerWindow GUI.

View uploaded files and their metadata.

Click on a file to see metadata and download history.

Refresh file list or view download history as required.

**Design Considerations**
        Threaded server: Each client connection runs on a separate thread to handle concurrent uploads/downloads.
        
        Metadata safety: SQLite ensures persistent metadata storage.
        
        Compression: Optional gzip reduces bandwidth usage for large files.
        
        Admin control: Admin can track sensitive files and downloads.

**Security**

    1.Usernames are tracked for uploads/downloads.
    
    2.Admin can monitor access to sensitive files.
    
    3.Future improvements may include authentication tokens and encrypted transfers.

**License**

MIT License © 2025
