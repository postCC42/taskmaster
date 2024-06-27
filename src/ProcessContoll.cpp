#include "ProcessControl.hpp"


ProcessControl::ProcessControl(const std::string& name, const json& config)
    : name(name), pid(-1) {
    parseConfig(config);
}

void ProcessControl::parseConfig(const json& config) {
    command = config.at("command").get<std::string>();
    instances = config.at("instances").get<int>();
    autoStart = config.at("auto_start").get<bool>();
    autoRestart = config.at("auto_restart").get<std::string>();
    startTime = config.at("start_time").get<int>();
    stopTime = config.at("stop_time").get<int>();
    restartAttempts = config.at("restart_attempts").get<int>();
    stopSignal = config.at("stop_signal").get<std::string>();
    expectedExitCodes = config.at("expected_exit_codes").get<std::vector<int>>();
    workingDirectory = config.at("working_directory").get<std::string>();
    umaskStr = config.at("umask").get<std::string>();
    stdoutLog = config.at("stdout_log").get<std::string>();
    stderrLog = config.at("stderr_log").get<std::string>();
    for (const auto& envVar : config.at("environment_variables")) {
        std::string envVarStr = envVar.get<std::string>();
        auto delimiterPos = envVarStr.find('=');
        auto key = envVarStr.substr(0, delimiterPos);
        auto value = envVarStr.substr(delimiterPos + 1);
        environmentVariables[key] = value;
    }
}

void ProcessControl::setUpEnvironment() {
    for (const auto& [key, value] : environmentVariables) {
        setenv(key.c_str(), value.c_str(), 1);
    }
}

void ProcessControl::start() {
    setUpEnvironment();
    // we need to ensure that supervised program runs independently => fork create child process
    pid = fork();
    if (pid < 0) {
        throw std::runtime_error("Failed to fork process");
    } else if (pid == 0) {
        if (chdir(workingDirectory.c_str()) != 0) {
            throw std::runtime_error("Failed to change directory to " + workingDirectory);
        }

        if (umaskStr != "None") {
            ::umask(std::stoi(umaskStr, nullptr, 8));
        }

        // Redirect stdout and stderr
        int stdoutFd = open(stdoutLog.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
        int stderrFd = open(stderrLog.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
        dup2(stdoutFd, STDOUT_FILENO);
        dup2(stderrFd, STDERR_FILENO);

        execl("/bin/sh", "sh", "-c", command.c_str(), nullptr);
        _exit(EXIT_FAILURE);
    } else {
        std::cout << "Started process " << name << " with PID " << pid << std::endl;
    }
}

bool ProcessControl::isRunning() const {
    if (pid <= 0) {
        return false; // No valid process ID
    }

    int status;
    pid_t result = waitpid(pid, &status, WNOHANG);

    if (result == 0) {
        // Process with PID is still running
        return true;
    } else if (result == pid) {
        // Process with PID has terminated
        return false;
    } else if (result == -1) {
        throw std::runtime_error("Error checking process status");
    }

    return false;
}

std::string ProcessControl::getStatus() const {
    return isRunning() ? "Running" : "Stopped";
}

std::string ProcessControl::getName() const {
    return name;
}
