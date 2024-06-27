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

    private:
    std::string name;
    std::string command;
    int instances;
    bool autoStart;
    std::string autoRestart;
    int startTime;
    int stopTime;
    int restartAttempts;
    std::string stopSignal;
    std::vector<int> expectedExitCodes; // dynamic allocation, order doesn't matter
    std::string workingDirectory;
    std::string umask;
    std::string stdoutLog;
    std::string stderrLog;
    std::map<std::string, std::string> environmentVariables; // each key unique and quick access to values
    pid_t pid;

    void parseConfig(const nlohmann::json& config);
    void setUpEnvironment();

#endif