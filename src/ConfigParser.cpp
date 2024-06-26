#include "ConfigParser.hpp"
#include <fstream>
#include <stdexcept>

ConfigParser::ConfigParser(const std::string &configFilePath) {
    parseConfig(configFilePath);
}

void ConfigParser::parseConfig(const std::string& configFilePath) {
    std::ifstream configFile(configFilePath);
    if (!configFile.is_open()) {
        throw std::runtime_error("Could not open config file: " + configFilePath);
    }
    configFile >> config;
}

nlohmann::json ConfigParser::getConfig() const {
    return config;
}