#include "Logger.hpp"
#include <iostream>

void Logger::init(const std::string& logFile) {
    std::cout << "Initializing logger with file: " << logFile << std::endl;
}

void Logger::info(const std::string& message) {
    std::cout << "[INFO] " << message << std::endl;
}

void Logger::error(const std::string& message) {
    std::cerr << "[ERROR] " << message << std::endl;
}