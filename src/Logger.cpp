#include "../include/Logger.hpp"

Logger* Logger::instance = nullptr;

Logger& Logger::getInstance() {
    if (instance == nullptr) {
        instance = new Logger();
    }
    return *instance;
}

void Logger::initialize(const bool logToFile, const std::string& logFilePath) {
    if (logToFile) {
        logFile.open(logFilePath, std::ios::out | std::ios::app);
        if (!logFile.is_open()) {
            throw std::runtime_error("Failed to open log file");
        }
        log("Logging to file: " + logFilePath);
    }
}

void Logger::cleanup() {
    if (logFile.is_open()) {
        logFile.close();
    }
    delete instance;
    instance = nullptr;
}

void Logger::log(const std::string& message) {
    std::cout << message << std::endl;
    if (logFile.is_open()) {
        logFile << message << std::endl;
    }
}

void Logger::logError(const std::string& message) {
    std::cerr << message << std::endl;
    if (logFile.is_open()) {
        logFile << "[ERROR] " << message << std::endl;
    }
}

void Logger::logToFile(const std::string& message) {
    if (logFile.is_open()) {
        logFile << message << std::endl;
    }
}
