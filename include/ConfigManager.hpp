#ifndef CONFIGMANAGER_HPP
# define CONFIGMANAGER_HPP

#include <fstream>
#include <stdexcept>
#include <iostream>
#include <nlohmann/json.hpp>
#include "Process.hpp"
#include "Utils.hpp"

class Process;
using json = nlohmann::json;
using ConfigChangesMap = std::unordered_map<std::string, std::string>;

class ConfigManager {
public:
       static json parseConfig(const std::string& configFilePath);

       static void checkCommand(const json &newConfig, const Process &process, ConfigChangesMap &changes);
       static void checkInstances(const json &newConfig, const Process &process, ConfigChangesMap &changes);
       static void checkAutoStart(const json &newConfig, const Process &process, ConfigChangesMap &changes);
       static void checkAutoRestart(const json &newConfig, const Process &process, ConfigChangesMap &changes);
       static void checkStartTime(const json &newConfig, const Process &process, ConfigChangesMap &changes);
       static void checkStopTime(const json &newConfig, const Process &process, ConfigChangesMap &changes);
       static void checkRestartAttempts(const json &newConfig, const Process &process, ConfigChangesMap &changes);
       static void checkStopSignal(const json &newConfig, const Process &process, ConfigChangesMap &changes);
       static void checkExpectedExitCodes(const json &newConfig, const Process &process, ConfigChangesMap &changes);
       static void checkWorkingDirectory(const json &newConfig, const Process &process, ConfigChangesMap &changes);
       static void checkUmask(const json &newConfig, const Process &process, ConfigChangesMap &changes);
       static void checkStdoutLog(const json &newConfig, const Process &process, ConfigChangesMap &changes);
       static void checkStderrLog(const json &newConfig, const Process &process, ConfigChangesMap &changes);
       static void checkEnvironmentVariables(const json &newConfig, const Process &process, ConfigChangesMap &changes);

       static std::string serializeVector(const std::vector<int> &vec);
       static std::string serializeEnvVars(const std::map<std::string, std::string> &envVars);
       static std::map<std::string, std::string> deserializeEnvVars(const std::string &str);
};

#endif
