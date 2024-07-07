#ifndef LOGGER_HPP
# define LOGGER_HPP

#include <fstream>
#include <iostream>

class Logger {
    public:
        static Logger& getInstance();
        ~Logger();
        void initialize(const bool logToFile, const std::string& logFilePath);

        void log(const std::string& message);
        void logError(const std::string& message);
        void logToFile(const std::string& message);

    private:
        static Logger* instance;
        std::ofstream logFile;
};

#endif //LOGGER_HPP
