#ifndef UTILS_H
# define UTILS_H

#include <string>
#include <vector>
#include <sstream>

class Utils {
    public:
        static std::vector<std::string> split(const std::string &s, char delimiter);
};

#endif