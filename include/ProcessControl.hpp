#ifndef PROCESSCONTROL_HPP
# define PROCESSCONTROL_HPP

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <csignal>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <algorithm>

using json = nlohmann::json;

class ProcessControl {

    public:
        explicit ProcessControl(const std::string& name, const json& config);
        void start();
        bool isRunning() const;
        std::string getStatus() const;
        std::string getName() const;
        int getStartTime() const { return startTime; }


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
        std::string umaskStr;
        std::string stdoutLog;
        std::string stderrLog;
        std::map<std::string, std::string> environmentVariables; // each key unique and quick access to values
        pid_t pid;

        void parseConfig(const json& config);
        void setUpEnvironment();
    };

#endif