#include "ConfigParser.hpp"

json ConfigParser::parseConfig(const std::string& configFilePath) {
    json config;
    std::ifstream configFile(configFilePath);
    if (!configFile.is_open()) {
        throw std::runtime_error("Could not open config file: " + configFilePath);
    }

    configFile >> config;
    return config;
}