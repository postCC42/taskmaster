#ifndef UTILS_H
# define UTILS_H

#include <string>
#include <vector>
#include <filesystem>
#include "TaskMaster.hpp"

class Utils {
    public:
        static std::vector<std::string> split(const std::string &s, char delimiter);
        static void signalHandler(int sig);
        static bool checkFilePermissions(const std::string& filePath);
};

#endif