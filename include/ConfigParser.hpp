#ifndef CONFIGPARSER_HPP
# define CONFIGPARSER_HPP

#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
using json = nlohmann::json;

class ConfigParser {

    public:
        explicit ConfigParser(const std::string& configFilePath);
        nlohmann::json getConfig() const;

    private:
        nlohmann::json config;
        void parseConfig(const std::string& configFilePath);
};

#endif
