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

using json = nlohmann::json;

class Process {

    public:
        explicit Process(const std::string& name, const json& config);
        void start();
        void stop();
        [[nodiscard]] std::string getStatus() const;
        [[nodiscard]] std::string getName() const;
        [[nodiscard]] int getStartTime() const { return startTime; }
        [[nodiscard]] int getAutoStart() const { return autoStart; }
        [[nodiscard]] int getRestartAttempts() const { return restartAttempts; }
        [[nodiscard]] bool isRunning() const;

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

        void parseConfig(const json& config);
        void setUpEnvironment();
        int getRunningChildCount();

        void runChildProcess() const;
        void monitorChildProcesses();
        void handleChildExit(pid_t pid, int status);

        bool stopProcess(pid_t pid, std::vector<pid_t>& pidsToErase);
        static void forceStopProcess(pid_t pid, std::vector<pid_t>& pidsToErase);
        void cleanupStoppedProcesses(std::vector<pid_t>& pidsToErase);

        const std::map<std::string, int> signalMap = {
            {"SIGTERM", SIGTERM},
            {"SIGINT", SIGINT},
            {"SIGKILL", SIGKILL},
            {"SIGSTOP", SIGSTOP},
            {"SIGCONT", SIGCONT},
        };
};

#endif