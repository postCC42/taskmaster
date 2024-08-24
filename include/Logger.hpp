#ifndef LOGGER_HPP
# define LOGGER_HPP

#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>

class Logger {
    public:
        ~Logger();
        static Logger& getInstance();
        void cleanup();
        void initialize(bool logToFile, const std::string& logFilePath);

        void log(const std::string& message);
        void logError(const std::string& message);
        void logToFile(const std::string& message);
        void reloadConfig(bool logToFile, const std::string& logFilePath);

        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;
        Logger(Logger&&) = delete;
        Logger& operator=(Logger&&) = delete;

    private:
        Logger() = default;
        std::ofstream logFile;
        std::mutex logMutex;

        static std::once_flag initInstanceFlag;
        static std::unique_ptr<Logger> instance;
        static std::string logFilePath;
};

#endif //LOGGER_HPP
