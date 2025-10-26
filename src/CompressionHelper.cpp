#include "CompressionHelper.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <zlib.h>

bool CompressionHelper::compressFile(const std::string& inputPath, const std::string& outputPath) {
    std::ifstream inFile(inputPath, std::ios::binary);
    if (!inFile.is_open()) {
        std::cerr << "[Compression] Failed to open input file: " << inputPath << std::endl;
        return false;
    }

    gzFile outFile = gzopen(outputPath.c_str(), "wb");
    if (!outFile) {
        std::cerr << "[Compression] Failed to open output file: " << outputPath << std::endl;
        return false;
    }

    std::vector<char> buffer(4096);
    while (!inFile.eof()) {
        inFile.read(buffer.data(), buffer.size());
        std::streamsize readBytes = inFile.gcount();
        if (readBytes > 0) {
            if (gzwrite(outFile, buffer.data(), static_cast<unsigned int>(readBytes)) != readBytes) {
                std::cerr << "[Compression] gzwrite failed." << std::endl;
                gzclose(outFile);
                return false;
            }
        }
    }

    gzclose(outFile);
    inFile.close();
    std::cout << "[Compression] Compressed: " << inputPath << " → " << outputPath << std::endl;
    return true;
}

bool CompressionHelper::decompressFile(const std::string& inputPath, const std::string& outputPath) {
    gzFile inFile = gzopen(inputPath.c_str(), "rb");
    if (!inFile) {
        std::cerr << "[Compression] Failed to open input file: " << inputPath << std::endl;
        return false;
    }

    std::ofstream outFile(outputPath, std::ios::binary);
    if (!outFile.is_open()) {
        std::cerr << "[Compression] Failed to open output file: " << outputPath << std::endl;
        gzclose(inFile);
        return false;
    }

    std::vector<char> buffer(4096);
    int bytesRead;
    while ((bytesRead = gzread(inFile, buffer.data(), static_cast<unsigned int>(buffer.size()))) > 0) {
        outFile.write(buffer.data(), bytesRead);
    }

    gzclose(inFile);
    outFile.close();
    std::cout << "[Compression] Decompressed: " << inputPath << " → " << outputPath << std::endl;
    return true;
}
