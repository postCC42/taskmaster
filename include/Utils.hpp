#ifndef UTILS_H
# define UTILS_H

#include <string>
#include <vector>
// #include <sstream>
#include <csignal>
// #include <iostream>
#include <nlohmann/json.hpp>
#include "Logger.hpp"
#include "TaskMaster.hpp"

class Process;


class Utils {
    public:
        static std::vector<std::string> split(const std::string &s, char delimiter);
        static void signalHandler(int sig);

};

#endif