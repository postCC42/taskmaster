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
#include <signal.h>
#include <algorithm>

using json = nlohmann::json;

class ProcessControl {

    public:
        explicit ProcessControl(const std::string& name, const json& config);
        void start();
        void stop();
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
        int stopSignal;
        std::vector<int> expectedExitCodes; // dynamic allocation, order doesn't matter
        std::string workingDirectory;
        int umaskInt;
        std::string stdoutLog;
        std::string stderrLog;
        std::map<std::string, std::string> environmentVariables; // each key unique and quick access to values

        // TODO: the pid needs to be a collection
        pid_t pid;

        void parseConfig(const json& config);
        void setUpEnvironment();

        // TODO: should be static
        const std::map<std::string, int> signalMap = {
            {"SIGTERM", SIGTERM},
            {"SIGINT", SIGINT},
            // TODO: add more signals
        };
};

#endif