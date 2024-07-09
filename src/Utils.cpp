#include "Utils.hpp"

std::vector<std::string> Utils::split(const std::string &s, const char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}


void Utils::signalHandler(int sig) {
    if (sig == SIGHUP) {
        Logger::getInstance().log("\nSIGHUP signal received. Reloading configuration...");
        TaskMaster::reloadConfig();
    } else {
        Logger::getInstance().log("\nsignal " + std::to_string(sig) + " received. Stopping all processes...");
        TaskMaster::stopAllProcesses();
        Logger::getInstance().log("TaskMaster shutting down...");
        exit(sig);
    }
}


void Utils::checkCommand(const json& newConfig, const Process& process, ConfigChangesMap& changes) {
    if (newConfig.at("command").get<std::string>() != process.getCommand()) {
        changes["command"] = newConfig.at("command").get<std::string>();
    }
}

void Utils::checkInstances(const json& newConfig, const Process& process, ConfigChangesMap& changes) {
    int newInstances = newConfig.at("instances").get<int>();
    if (newInstances != process.getInstances()) {
        if (newInstances < 0) {
            throw std::runtime_error(process.getName() + ": Invalid number of instances: " + std::to_string(newInstances));
        }
        changes["instances"] = std::to_string(newInstances);
    }
}

void Utils::checkAutoStart(const json& newConfig, const Process& process, ConfigChangesMap& changes) {
    if (newConfig.at("auto_start").get<bool>() != process.getAutoStart()) {
        changes["auto_start"] = std::to_string(newConfig.at("auto_start").get<bool>());
    }
}

void Utils::checkAutoRestart(const json& newConfig, const Process& process, ConfigChangesMap& changes) {
    std::string newAutoRestart = newConfig.at("auto_restart").get<std::string>();
    if (newAutoRestart != process.getAutoRestart()) {
        if (newAutoRestart != "always" && newAutoRestart != "never" && newAutoRestart != "unexpected") {
            throw std::runtime_error(process.getName() + ": Invalid auto restart value: " + newAutoRestart);
        }
        changes["auto_restart"] = newAutoRestart;
    }
}

void Utils::checkStartTime(const json& newConfig, const Process& process, ConfigChangesMap& changes) {
    int newStartTime = newConfig.at("start_time").get<int>();
    if (newStartTime != process.getStartTime()) {
        if (newStartTime < 0) {
            throw std::runtime_error(process.getName() + ": Invalid start time: " + std::to_string(newStartTime));
        }
        changes["start_time"] = std::to_string(newStartTime);
    }
}

void Utils::checkStopTime(const json& newConfig, const Process& process, ConfigChangesMap& changes) {
    int newStopTime = newConfig.at("stop_time").get<int>();
    if (newStopTime != process.getStopTime()) {
        if (newStopTime < 0) {
            throw std::runtime_error(process.getName() + ": Invalid stop time: " + std::to_string(newStopTime));
        }
        changes["stop_time"] = std::to_string(newStopTime);
    }
}

void Utils::checkRestartAttempts(const json& newConfig, const Process& process, ConfigChangesMap& changes) {
    int newRestartAttempts = newConfig.at("restart_attempts").get<int>();
    if (newRestartAttempts != process.getRestartAttempts()) {
        if (newRestartAttempts < 0) {
            throw std::runtime_error(process.getName() + ": Invalid restart attempts: " + std::to_string(newRestartAttempts));
        }
        changes["restart_attempts"] = std::to_string(newRestartAttempts);
    }
}

void Utils::checkStopSignal(const json& newConfig, const Process& process, ConfigChangesMap& changes) {
    std::string newStopSignal = newConfig.at("stop_signal").get<std::string>();
    if (process.getSignalMap().find(newStopSignal) == process.getSignalMap().end()) {
        Logger::getInstance().log(process.getName() + ": Invalid stop signal: " + newStopSignal);
        throw std::runtime_error(process.getName() + ": Invalid stop signal: " + newStopSignal);
    }
    if (process.getSignalMap().at(newStopSignal) != process.getStopSignal()) {
        changes["stop_signal"] = newStopSignal;
    }
}

void Utils::checkExpectedExitCodes(const json& newConfig, const Process& process, ConfigChangesMap& changes) {
    if (newConfig.at("expected_exit_codes").get<std::vector<int>>() != process.getExpectedExitCodes()) {
        std::vector<int> expectedExitCodes = process.getExpectedExitCodes();
        expectedExitCodes.clear();
        expectedExitCodes = newConfig.at("expected_exit_codes").get<std::vector<int>>();
        changes["expected_exit_codes"] = serializeVector(expectedExitCodes);
    }
}

void Utils::checkWorkingDirectory(const json& newConfig, const Process& process, ConfigChangesMap& changes) {
    if (newConfig.at("working_directory").get<std::string>() != process.getWorkingDirectory()) {
        changes["working_directory"] = newConfig.at("working_directory").get<std::string>();
    }
}

void Utils::checkUmask(const json& newConfig, const Process& process, ConfigChangesMap& changes) {
    if (newConfig.at("umask").get<int>() != process.getUmaskInt()) {
        changes["umask"] = std::to_string(newConfig.at("umask").get<int>());
    }
}

void Utils::checkStdoutLog(const json& newConfig, const Process& process, ConfigChangesMap& changes) {
    if (newConfig.at("stdout_log").get<std::string>() != process.getStdoutLog()) {
        changes["stdout_log"] = newConfig.at("stdout_log").get<std::string>();
    }
}

void Utils::checkStderrLog(const json& newConfig, const Process& process, ConfigChangesMap& changes) {
    if (newConfig.at("stderr_log").get<std::string>() != process.getStderrLog()) {
        changes["stderr_log"] = newConfig.at("stderr_log").get<std::string>();
    }
}

void Utils::checkEnvironmentVariables(const json& newConfig, const Process& process, ConfigChangesMap& changes) {
    std::map<std::string, std::string> newEnvVars;
    for (const auto& envVar : newConfig.at("environment_variables")) {
        std::string envVarStr = envVar.get<std::string>();
        const auto delimiterPos = envVarStr.find('=');
        auto key = envVarStr.substr(0, delimiterPos);
        const auto value = envVarStr.substr(delimiterPos + 1);
        newEnvVars[key] = value;
    }

    if (newEnvVars != process.getEnvironmentVariables()) {
        changes["environment_variables"] = serializeEnvVars(newEnvVars);
    }
}

std::string Utils::serializeVector(const std::vector<int>& vec) {
    json j = vec;
    return j.dump();
}

std::string Utils::serializeEnvVars(const std::map<std::string, std::string>& envVars) {
    json j = envVars;
    return j.dump();
}

std::map<std::string, std::string> Utils::deserializeEnvVars(const std::string& str) {
    json j = json::parse(str);
    return j.get<std::map<std::string, std::string>>();
}
