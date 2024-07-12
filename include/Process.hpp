#ifndef PROCESS_HPP
# define PROCESS_HPP

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
#include <algorithm>
#include <thread>
#include "colors.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include "ConfigManager.hpp"
// #include <atomic> // thread safe


using json = nlohmann::json;

using ConfigChangesMap = std::unordered_map<std::string, std::string>;

class Process {

    public:
        explicit Process(const std::string& name, const json& config);
        void start();
        void stop();
        [[nodiscard]] std::string getStatus() const;
        [[nodiscard]] std::string getName() const;
        [[nodiscard]] int getNumberOfInstances() const;
        [[nodiscard]] std::string getCommand() const { return command; }
        [[nodiscard]] int getInstances() const { return instances; }
        [[nodiscard]] bool getAutoStart() const { return autoStart; }
        [[nodiscard]] std::string getAutoRestart() const { return autoRestart; }
        [[nodiscard]] int getStartTime() const { return startTime; }
        [[nodiscard]] int getStopTime() const { return stopTime; }
        [[nodiscard]] int getRestartAttempts() const { return restartAttempts; }
        [[nodiscard]] int getStopSignal() const { return stopSignal; }
        [[nodiscard]] const std::vector<int>& getExpectedExitCodes() const { return expectedExitCodes; }
        [[nodiscard]] std::string getWorkingDirectory() const { return workingDirectory; }
        [[nodiscard]] int getUmaskInt() const { return umaskInt; }
        [[nodiscard]] std::string getStdoutLog() const { return stdoutLog; }
        [[nodiscard]] std::string getStderrLog() const { return stderrLog; }
        [[nodiscard]] const std::map<std::string, std::string>& getEnvironmentVariables() const { return environmentVariables; }
        [[nodiscard]] const std::map<std::string, int>& getSignalMap() const { return signalMap; }
        void reloadConfig(const json& newConfig);
        void stopInstance();

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
        std::vector<int> expectedExitCodes;
        std::string workingDirectory;
        int umaskInt;
        std::string stdoutLog;
        std::string stderrLog;
        std::map<std::string, std::string> environmentVariables; // each key unique and quick access to values
        std::vector<pid_t> child_pids;
        bool monitorThreadRunning = false;
        json newConfigFile;
        bool userStopped;        

        [[nodiscard]] bool isRunning() const;
        void parseConfig(const json& config);
        void setUpEnvironment();
        int getRunningChildCount();
        void startChildProcessAndMonitor();

        void runChildProcess() const;
        void monitorChildProcesses();
        void handleChildExit(pid_t pid, int status);

        bool stopProcess(pid_t pid, std::vector<pid_t>& pidsToErase);
        static void forceStopProcess(pid_t pid, std::vector<pid_t>& pidsToErase);
        void cleanupStoppedProcesses(std::vector<pid_t>& pidsToErase);

        ConfigChangesMap detectChanges(const json& newConfig);
        void applyChanges(const ConfigChangesMap& changes);
        bool changesRequireRestart(const ConfigChangesMap& changes);
        void updateUmask(const std::string &newValue);

        const std::map<std::string, int> signalMap = {
            {"SIGTERM", SIGTERM},
            {"SIGINT", SIGINT},
            {"SIGKILL", SIGKILL},
            {"SIGSTOP", SIGSTOP},
            {"SIGCONT", SIGCONT},
        };
};

#endif