#include "Process.hpp"

Process::Process(const std::string& name, const json& config)
    : name(name), pid(-1) {
    parseConfig(config);
}

void Process::parseConfig(const json& config) {
    command = config.at("command").get<std::string>();
    instances = config.at("instances").get<int>();
    autoStart = config.at("auto_start").get<bool>();
    autoRestart = config.at("auto_restart").get<std::string>();
    startTime = config.at("start_time").get<int>();
    stopTime = config.at("stop_time").get<int>();
    restartAttempts = config.at("restart_attempts").get<int>();

    std::string stopSignalStr = config.at("stop_signal").get<std::string>();
    if (signalMap.find(stopSignalStr) == signalMap.end()) {
        throw std::runtime_error("Invalid stop signal: " + stopSignalStr);
    }
    stopSignal = signalMap.at(stopSignalStr);

    expectedExitCodes = config.at("expected_exit_codes").get<std::vector<int>>();
    workingDirectory = config.at("working_directory").get<std::string>();
    umaskInt = config.at("umask").get<int>();
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

void Process::setUpEnvironment() {
    for (const auto& [key, value] : environmentVariables) {
        setenv(key.c_str(), value.c_str(), 1);
    }
}

void Process::start() {
    setUpEnvironment();
    // we need to ensure that supervised program runs independently => fork create child process

    // TODO: should be a loop for multiple instances
    pid = fork();
    if (pid < 0) {
        throw std::runtime_error("Failed to fork process");
    } else if (pid == 0) {
        if (chdir(workingDirectory.c_str()) != 0) {
            throw std::runtime_error("Failed to change directory to " + workingDirectory);
        }

         if (umaskInt != -1) {
            ::umask(umaskInt);
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

void Process::stop() {
    std::cout << "Stopping process " << name << " with PID " << pid << std::endl;
    if (pid <= 0) {
        throw std::runtime_error("No valid process ID");
    }

    if (kill(pid, stopSignal) != 0) {
        throw std::runtime_error("Failed to stop process");
    }

    std::cout << "Stopped process " << name << " with PID " << pid << std::endl;
    pid = -1;
}

bool Process::isRunning() const {
    if (pid <= 0) {
        return false; // No valid process ID
    }

    std::cout << "Checking process status for PID " << pid << std::endl;

    int status;
    pid_t result = waitpid(pid, &status, WNOHANG);

    if (result == 0) {
        return true;
    } else if (result == pid) {
        return false;
    } else if (result == -1) {
        throw std::runtime_error("Error checking process status");
    }

    return false;
}

std::string Process::getStatus() const {
    if (isRunning()) {
        return "Running PID " + std::to_string(pid);
    } else {
        return "Stopped";
    }
}

std::string Process::getName() const {
    return name;
}
