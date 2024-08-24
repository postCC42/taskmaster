#include "../include/Logger.hpp"

std::once_flag Logger::initInstanceFlag;
std::unique_ptr<Logger> Logger::instance;
std::string Logger::logFilePath;

Logger& Logger::getInstance() {
    std::call_once(Logger::initInstanceFlag, []() {
        instance.reset(new Logger);
    });
    return *instance;
}

void Logger::initialize(bool logToFile, const std::string& logFilePath) {
    if (logToFile) {
        Logger::logFilePath = logFilePath;
        logFile.open(logFilePath, std::ios::out | std::ios::app);
        if (!logFile.is_open()) {
            throw std::runtime_error("Failed to open log file");
        }
        log("Logging to file: " + logFilePath);
    }
}

void Logger::reloadConfig(bool logToFile, const std::string& logFilePath) {
    if (logToFile) {
        std::lock_guard<std::mutex> guard(logMutex);
        if (logFile.is_open() && Logger::logFilePath == logFilePath) {
            return;
        }
        if (logFile.is_open() == false || Logger::logFilePath != logFilePath) {
            logFile.close();
        }

        Logger::logFilePath = logFilePath;
        logFile.open(logFilePath, std::ios::out | std::ios::app);
        if (!logFile.is_open()) {
            throw std::runtime_error("Failed to open log file");
        }
    } else if (logFile.is_open()) {
        logFile.close();
    }
}

Logger::~Logger() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

void Logger::log(const std::string& message) {
    std::cout << message << std::endl;
    std::lock_guard<std::mutex> guard(logMutex);
    if (logFile.is_open()) {
        logFile << message << std::endl;
    }
}

void Logger::logError(const std::string& message) {
    std::cerr << message << std::endl;
    std::lock_guard<std::mutex> guard(logMutex);
    if (logFile.is_open()) {
        logFile << "[ERROR] " << message << std::endl;
    }
}

void Logger::logToFile(const std::string& message) {
    std::lock_guard<std::mutex> guard(logMutex);
    if (logFile.is_open()) {
        logFile << message << std::endl;
    }
}
