#ifndef UTILS_H
# define UTILS_H

#include <string>
#include <vector>
#include <sstream>
#include <csignal> // for signal handling
#include <iostream>


class Utils {
    public:
        static std::vector<std::string> split(const std::string &s, char delimiter);
        void sigintHandler(int sig);
};

#endif