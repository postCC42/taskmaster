#ifndef CONFIGPARSER_HPP
# define CONFIGPARSER_HPP

#include <fstream>
#include <stdexcept>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class ConfigParser {
public:
       static json parseConfig(const std::string& configFilePath);
};

#endif
