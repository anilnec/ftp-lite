#pragma once
#include <string>

class CompressionHelper {
public:
    // Compress inputPath -> outputPath using gzip
    static bool compressFile(const std::string& inputPath, const std::string& outputPath);

    // Decompress inputPath -> outputPath
    static bool decompressFile(const std::string& inputPath, const std::string& outputPath);
};
    