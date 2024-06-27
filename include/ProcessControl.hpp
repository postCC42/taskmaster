#ifndef PROCESSCONTROL_HPP
# define PROCESSCONTROL_HPP

#include <string>
#include <vector>
#include <map>
#include <sys/types.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class ProcessControl {
public:
    explicit ProcessControl(const std::string& name, const json& config);
    void start();
    void stop();
    void restart();
    std::string getStatus() const;
};

#endif