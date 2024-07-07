#include "../include/Logger.hpp"

Logger* Logger::instance = nullptr;

Logger& Logger::getInstance() {
    if (instance == nullptr) {
        instance = new Logger();
    }
    return *instance;
}

void Logger::initialize(const bool logToFile, const std::string& logFilePath) {
    this->logToFile = logToFile;
    if (logToFile) {
        logFile.open(logFilePath, std::ios::out | std::ios::app);
        if (!logFile.is_open()) {
            throw std::runtime_error("Failed to open log file");
        }
        log("Logging to file: " + logFilePath);
    }
}

Logger::~Logger() {
    if (logToFile) {
        logFile.close();
    }
}

void Logger::log(const std::string& message) {
    std::cout << message << std::endl;
    if (logToFile) {
        logFile << message << std::endl;
    }
}

void Logger::logError(const std::string& message) {
    std::cerr << message << std::endl;
    if (logToFile) {
        logFile << "[ERROR] " << message << std::endl;
    }
}