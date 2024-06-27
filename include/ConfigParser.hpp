#ifndef CONFIGPARSER_HPP
# define CONFIGPARSER_HPP

#include <fstream>
#include <stdexcept>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class ConfigParser {

    public:
        explicit ConfigParser(const std::string& configFilePath);
        json getConfig() const;

    private:
        json config;
        void parseConfig(const std::string& configFilePath);
};

#endif
