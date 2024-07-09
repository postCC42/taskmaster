#ifndef UTILS_H
# define UTILS_H

#include <string>
#include <vector>
#include <sstream>
#include <csignal>
#include <iostream>
#include <nlohmann/json.hpp>
#include "TaskMaster.hpp"

class Process;

using ConfigChangesMap = std::unordered_map<std::string, std::string>;
using json = nlohmann::json; 

class Utils {
    public:
        static std::vector<std::string> split(const std::string &s, char delimiter);
        static void signalHandler(int sig);

        static void checkCommand(const json& newConfig, const Process & process, ConfigChangesMap& changes);
        static void checkInstances(const json& newConfig, const Process & process, ConfigChangesMap& changes);
        static void checkAutoStart(const json& newConfig, const Process & process, ConfigChangesMap& changes);
        static void checkAutoRestart(const json& newConfig, const Process & process, ConfigChangesMap& changes);
        static void checkStartTime(const json& newConfig, const Process & process, ConfigChangesMap& changes);
        static void checkStopTime(const json& newConfig, const Process & process, ConfigChangesMap& changes);
        static void checkRestartAttempts(const json& newConfig, const Process & process, ConfigChangesMap& changes);
        static void checkStopSignal(const json& newConfig, const Process & process, ConfigChangesMap& changes);
        static void checkExpectedExitCodes(const json& newConfig, const Process & process, ConfigChangesMap& changes);
        static void checkWorkingDirectory(const json& newConfig, const Process & process, ConfigChangesMap& changes);
        static void checkUmask(const json& newConfig, const Process & process, ConfigChangesMap& changes);
        static void checkStdoutLog(const json& newConfig, const Process & process, ConfigChangesMap& changes);
        static void checkStderrLog(const json& newConfig, const Process & process, ConfigChangesMap& changes);
        static void checkEnvironmentVariables(const json& newConfig, const Process & process, ConfigChangesMap& changes);
        static std::string serializeVector(const std::vector<int>& vec);
        static std::string serializeEnvVars(const std::map<std::string, std::string>& envVars);   
        static std::map<std::string, std::string> deserializeEnvVars(const std::string& str);
};

#endif