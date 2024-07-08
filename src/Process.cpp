/*
____ Process Management Class ____

    ____ Initialization and Configuration Parsing ____:
    - Handles initialization of process parameters such as command, instances, and environment variables from a JSON configuration.

    Functions:
    - Process(const std::string& name, const json& config): Constructor initializing the process with name and configuration.
    - parseConfig(const json& config): Parses JSON config to set process parameters.
    - setUpEnvironment(): Sets up environment variables based on configured values.

    ____ Process Start and Monitoring ____:
    - Manages the creation and monitoring of child processes based on configured instances.

    Functions:
    - start(): Initiates child processes and starts monitoring.
    - startChildProcesses(): Forks child processes based on configured instances.
    - monitorChildProcesses(): Monitors child processes for exit and status changes.
    - runChildProcess(): Executes the command for each child process, manages directory changes, and redirects outputs.
    - handleParentProcess(pid_t child_pid): Manages parent process responsibilities after forking.

    ____ Process Stop and Synchronization ____:
    - Handles termination of all child processes, including graceful and forced termination methods.

    Functions:
    - stop(): Initiates the process to stop all child processes.
    - checkNoInstancesLeft(): Checks if there are no instances left to stop.
    - stopProcess(pid_t pid, std::vector<pid_t>& pidsToErase): Stops a specific child process gracefully.
    - forceStopProcess(pid_t pid, std::vector<pid_t>& pidsToErase): Forces termination of a specific child process.
    - cleanupStoppedProcesses(std::vector<pid_t>& pidsToErase): Cleans up and removes stopped child processes from tracking.
    - notifyAllStopped(): Notifies when all instances of the program have been successfully stopped.

    ____ Lifecycle and Status Checking ____:
    - Provides methods to check if processes are running and retrieve their current status.

    Functions:
    - isRunning(): Checks if all configured instances of the process are currently running.
    - getStatus(): Retrieves the current status of the process (Running or Stopped).
    - getName(): Retrieves the name of the process.
    - countRunningInstances(): Counts and returns the number of currently running child processes.
*/

#include "Process.hpp"


// ___________________ INIT AND PARSE ___________________
Process::Process(const std::string& name, const json& config) : name(name), instances(0) {
    parseConfig(config);
}

void Process::parseConfig(const json& config) {
    command = config.at("command").get<std::string>();
    instances = config.at("instances").get<int>();
    if (instances < 0) throw std::runtime_error(name + ": Invalid number of instances: " + std::to_string(instances));
    autoStart = config.at("auto_start").get<bool>();
    autoRestart = config.at("auto_restart").get<std::string>();
    if (autoRestart != "always" && autoRestart != "never" && autoRestart != "unexpected") {
        throw std::runtime_error(name + ": Invalid auto restart value: " + autoRestart);
    }
    startTime = config.at("start_time").get<int>();
    if (startTime < 0) throw std::runtime_error(name + ": Invalid start time: " + std::to_string(startTime));
    stopTime = config.at("stop_time").get<int>();
    if (stopTime < 0) throw std::runtime_error(name + ": Invalid stop time: " + std::to_string(stopTime));
    restartAttempts = config.at("restart_attempts").get<int>();
    if (restartAttempts < 0) throw std::runtime_error(name + ": Invalid restart attempts: " + std::to_string(restartAttempts));

    std::string stopSignalStr = config.at("stop_signal").get<std::string>();
    if (signalMap.find(stopSignalStr) == signalMap.end()) {
        throw std::runtime_error(name + ": Invalid stop signal: " + stopSignalStr);
    }
    stopSignal = signalMap.at(stopSignalStr);

    expectedExitCodes = config.at("expected_exit_codes").get<std::vector<int>>();
    workingDirectory = config.at("working_directory").get<std::string>();
    umaskInt = config.at("umask").get<int>();
    stdoutLog = config.at("stdout_log").get<std::string>();
    stderrLog = config.at("stderr_log").get<std::string>();
    for (const auto& envVar : config.at("environment_variables")) {
        std::string envVarStr = envVar.get<std::string>();
        const auto delimiterPos = envVarStr.find('=');
        auto key = envVarStr.substr(0, delimiterPos);
        const auto value = envVarStr.substr(delimiterPos + 1);
        environmentVariables[key] = value;
    }
}

ConfigChangesMap Process::detectChanges(const json& newConfig) {
    ConfigChangesMap changes;

    if (newConfig.at("command").get<std::string>() != command) {
        changes["command"] = newConfig.at("command").get<std::string>();
    }

    if (newConfig.at("instances").get<int>() != instances) {
        int newInstances = newConfig.at("instances").get<int>();
        if (newInstances < 0) throw std::runtime_error(name + ": Invalid number of instances: " + std::to_string(newInstances));
        changes["instances"] = std::to_string(newInstances);
    }

    if (newConfig.at("auto_start").get<bool>() != autoStart) {
        changes["auto_start"] = std::to_string(newConfig.at("auto_start").get<bool>());
    }

    if (newConfig.at("auto_restart").get<std::string>() != autoRestart) {
        std::string newAutoRestart = newConfig.at("auto_restart").get<std::string>();
        if (newAutoRestart != "always" && newAutoRestart != "never" && newAutoRestart != "unexpected") {
            throw std::runtime_error(name + ": Invalid auto restart value: " + newAutoRestart);
        }
        changes["auto_restart"] = newAutoRestart;
    }

    if (newConfig.at("start_time").get<int>() != startTime) {
        int newStartTime = newConfig.at("start_time").get<int>();
        if (newStartTime < 0) throw std::runtime_error(name + ": Invalid start time: " + std::to_string(newStartTime));
        changes["start_time"] = std::to_string(newStartTime);
    }

    if (newConfig.at("stop_time").get<int>() != stopTime) {
        int newStopTime = newConfig.at("stop_time").get<int>();
        if (newStopTime < 0) throw std::runtime_error(name + ": Invalid stop time: " + std::to_string(newStopTime));
        changes["stop_time"] = std::to_string(newStopTime);
    }

    if (newConfig.at("restart_attempts").get<int>() != restartAttempts) {
        int newRestartAttempts = newConfig.at("restart_attempts").get<int>();
        if (newRestartAttempts < 0) throw std::runtime_error(name + ": Invalid restart attempts: " + std::to_string(newRestartAttempts));
        changes["restart_attempts"] = std::to_string(newRestartAttempts);
    }
       std::string newStopSignal = newConfig.at("stop_signal").get<std::string>();

    // Log the previous and new stop signals
    Logger::getInstance().log("Previous stop signal: " + signalToString(stopSignal));
    Logger::getInstance().log("New stop signal: " + newStopSignal);

    // Check if the new stop signal is valid
    if (signalMap.find(newStopSignal) == signalMap.end()) {
        Logger::getInstance().log(name + ": Invalid stop signal: " + newStopSignal);
        throw std::runtime_error(name + ": Invalid stop signal: " + newStopSignal);
    }

    // If the new stop signal is different from the current one, record the change
    if (signalMap.at(newStopSignal) != stopSignal) {
        Logger::getInstance().log("Detected change in stop signal: " + newStopSignal);
        changes["stop_signal"] = newStopSignal;
    }
    if (newConfig.at("expected_exit_codes").get<std::vector<int>>() != expectedExitCodes) {
        changes["expected_exit_codes"] = "changed";
    }

    if (newConfig.at("working_directory").get<std::string>() != workingDirectory) {
        changes["working_directory"] = newConfig.at("working_directory").get<std::string>();
    }

    if (newConfig.at("umask").get<int>() != umaskInt) {
        changes["umask"] = std::to_string(newConfig.at("umask").get<int>());
    }

    if (newConfig.at("stdout_log").get<std::string>() != stdoutLog) {
        changes["stdout_log"] = newConfig.at("stdout_log").get<std::string>();
    }

    if (newConfig.at("stderr_log").get<std::string>() != stderrLog) {
        changes["stderr_log"] = newConfig.at("stderr_log").get<std::string>();
    }

    for (const auto& envVar : newConfig.at("environment_variables")) {
        std::string envVarStr = envVar.get<std::string>();
        const auto delimiterPos = envVarStr.find('=');
        auto key = envVarStr.substr(0, delimiterPos);
        const auto value = envVarStr.substr(delimiterPos + 1);
        if (environmentVariables[key] != value) {
            changes["environment_variables"] = "changed";
            break;
        }
    }

    return changes;
}

std::string Process::signalToString(int signal) {
    for (const auto& entry : signalMap) {
        if (entry.second == signal) {
            return entry.first;
        }
    }
    return "Unknown signal";
}

void Process::applyChanges(const ConfigChangesMap& changes) {
    for (const auto& change : changes) {
        const std::string& key = change.first;
        const std::string& value = change.second;

        if (key == "command") {
            command = value;
        } else if (key == "instances") {
            instances = std::stoi(value);
        } else if (key == "auto_start") {
            autoStart = (value == "1");
        } else if (key == "auto_restart") {
            autoRestart = value;
        } else if (key == "start_time") {
            startTime = std::stoi(value);
        } else if (key == "stop_time") {
            stopTime = std::stoi(value);
        } else if (key == "restart_attempts") {
            restartAttempts = std::stoi(value);
        } else if (key == "stop_signal") {
            stopSignal = signalMap.at(value);
        } else if (key == "expected_exit_codes") {
            // Assuming expected_exit_codes are provided as a JSON array of integers
            try {
                expectedExitCodes.clear();
                json exitCodesJson = json::parse(value);
                for (const auto& code : exitCodesJson) {
                    expectedExitCodes.push_back(code.get<int>());
                }
            } catch (const std::exception& e) {
                throw std::runtime_error(name + ": Error parsing expected_exit_codes: " + std::string(e.what()));
            }
        } else if (key == "working_directory") {
            workingDirectory = value;
        } else if (key == "umask") {
            umaskInt = std::stoi(value);
        } else if (key == "stdout_log") {
            stdoutLog = value;
        } else if (key == "stderr_log") {
            stderrLog = value;
        } else if (key == "environment_variables") {
            environmentVariables.clear();
            try {
                json envVarsJson = json::parse(value);
                for (const auto& envVar : envVarsJson) {
                    std::string envVarStr = envVar.get<std::string>();
                    const auto delimiterPos = envVarStr.find('=');
                    auto key = envVarStr.substr(0, delimiterPos);
                    const auto val = envVarStr.substr(delimiterPos + 1);
                    environmentVariables[key] = val;
                }
            } catch (const std::exception& e) {
                throw std::runtime_error(name + ": Error parsing environment_variables: " + std::string(e.what()));
            }
        }
    }
}


void Process::reloadConfig(const json& newConfig) {
    Logger::getInstance().log("Reloading config for process: " + name);

    // Detect and apply configuration changes
    ConfigChangesMap changes = detectChanges(newConfig);
    for (const auto& change : changes) {
        Logger::getInstance().log("Detected change: " + change.first + " -> " + change.second);
    }

    if (!changes.empty()) {
        applyChanges(changes);
        if (changesRequireRestart(changes)) {
            Logger::getInstance().log("Some changes require a restart for process: " + name);
            userStopped = true;
            stop();
            start();
        } else {
            // sendSighup();
            updateDinamicallyWithoutRestarting(changes);
        }
    } else {
        Logger::getInstance().log("No changes detected " + name);
    }
}

bool Process::changesRequireRestart(const ConfigChangesMap& changes) {
    static const std::vector<std::string> restartKeys = {"command", "instances", "auto_start",  "working_directory"};

    bool requiresRestart = false;

    for (const auto& key : restartKeys) {
        if (changes.find(key) != changes.end()) {
            Logger::getInstance().log("Detected change in key: " + key);
            requiresRestart = true;
        }
    }

    return requiresRestart;
}

void Process::updateDinamicallyWithoutRestarting(const ConfigChangesMap& changes) {
    for (const auto& change : changes) {
        if (change.first == "stdout_log") {
            updateStdoutLog();
        } else if (change.first == "stderr_log") {
            updateStderrLog();
        } else if (change.first == "umask") {
            updateUmask(change.second);
        }
    }

    Logger::getInstance().log("Dinamically updated configuration successfully.");
}

void Process::updateStdoutLog() {
    std::cout << "Updating stdout_log to: " << stdoutLog << std::endl;
    fflush(stdout);
    fclose(stdout);
    
    FILE* new_stdout = freopen(stdoutLog.c_str(), "a", stdout);
    if (new_stdout == nullptr) {
        perror("Failed to redirect stdout");
        _exit(EXIT_FAILURE);
    } else {
        stdout = new_stdout;
    }
     fflush(stdout);
}

void Process::updateStderrLog() {
    fflush(stderr);
    fflush(stderr);
    std::cout << "Updating stderr_log to: " << stderrLog << std::endl;
    FILE* new_stderr = freopen(stderrLog.c_str(), "a", stderr);
    if (new_stderr == nullptr) {
        perror("Failed to redirect stderr");
        _exit(EXIT_FAILURE);
    } else {
        stderr = new_stderr;
    }
    fflush(stderr);
}

void Process::updateUmask(std::string newValue) {
    std::cout << "Updating umask to: " << newValue << std::endl;
    if (umaskInt != -1) {
        ::umask(umaskInt);
    }
}


// void Process::sighupHandler(int sig) {
//     Logger::getInstance().log("Received SIGHUP signal for process: " + name);
// }

void Process::sendSighup() {
    Logger::getInstance().log("Sending SIGHUP signal to process: " + name);

    for (const auto& pid : child_pids) {
        if (kill(pid, SIGHUP) != 0) {
            Logger::getInstance().logError("Error sending SIGHUP to PID " + std::to_string(pid) + ": " + std::strerror(errno));
        }
    }
}


void Process::setUpEnvironment() {
    for (const auto& [key, value] : environmentVariables) {
        setenv(key.c_str(), value.c_str(), 1);
    }
}

// ___________________ START AND MONITOR ___________________
void Process::start() {
    if (instances < 1) {
        throw std::runtime_error("Invalid number of instances: " + std::to_string(instances));
    }

    // Check if any child process is already running
    int runningChildCount = getRunningChildCount();

    for (int i = runningChildCount; i < instances; ++i) {
        pid_t child_pid = fork();
        if (child_pid < 0) {
            throw std::runtime_error("Fork failure for instance " + std::to_string(i));
        }
        if (child_pid == 0) {
            setUpEnvironment();
            runChildProcess();
        } else {
            Logger::getInstance().log(name + " instance " + std::to_string(i) + " started with PID " + std::to_string(child_pid) + ".");
            child_pids.push_back(child_pid);
        }
    }

    if (monitorThreadRunning == false) {
        std::thread monitorThread(&Process::monitorChildProcesses, this);
        monitorThread.detach();
        monitorThreadRunning = true;
    }
}

int Process::getRunningChildCount() {
    int runningChildCount = 0;
    for (const pid_t& pid : child_pids) {
        if (kill(pid, 0) == 0) {
            runningChildCount++;
        } else if (errno != ESRCH) {
            Logger::getInstance().logError("Error checking process status for PID " + std::to_string(pid) + ": " + std::strerror(errno));
        }
    }
    return runningChildCount;
}

void Process::runChildProcess() const {
    // signal(SIGHUP, sighupHandler);
    if (chdir(workingDirectory.c_str()) != 0) {
        perror("Failed to change directory");
        _exit(EXIT_FAILURE);
    }

    if (umaskInt != -1) {
        ::umask(umaskInt);
    }

    // Redirect stdout and stderr
    if (freopen(stdoutLog.c_str(), "a", stdout) == nullptr || freopen(stderrLog.c_str(), "a", stderr) == nullptr) {
        perror("Failed to redirect stdout/stderr");
        _exit(EXIT_FAILURE);
    }

    const std::vector<std::string> args = Utils::split(command, ' ');
    std::vector<char*> argv(args.size() + 1);
    for (size_t j = 0; j < args.size(); ++j) {
        argv[j] = const_cast<char*>(args[j].c_str());
    }
    argv[args.size()] = nullptr;

    if (execvp(argv[0], argv.data()) == -1) {
        perror("execvp");
        _exit(EXIT_FAILURE);
    }
}

void Process::monitorChildProcesses() {
    while (true) {
        int status;

        pid_t pid = waitpid(-1, &status, WNOHANG);
        if (pid > 0) {
            handleChildExit(pid, status);
        } else if (pid == -1) {
            if (errno == ECHILD) {
                Logger::getInstance().log("No more child processes to monitor.");
                break;
            }
            Logger::getInstance().logError("waitpid error: " + std::string(strerror(errno)));
        }
        if (child_pids.empty()) {
            break;
        }
        usleep(100000);
    }
    monitorThreadRunning = false;
}

void Process::handleChildExit(pid_t pid, int status) {
    auto it = std::find(child_pids.begin(), child_pids.end(), pid);
    if (it != child_pids.end()) {
        child_pids.erase(it);
        int exitStatus;
        if (WIFEXITED(status)) {
            exitStatus = WEXITSTATUS(status);
            Logger::getInstance().log("Child process " + std::to_string(pid) + " exited with status " + std::to_string(exitStatus));
        } else if (WIFSIGNALED(status)) {
            exitStatus = WTERMSIG(status);
            Logger::getInstance().logError("Child process " + std::to_string(pid) + " terminated by signal " + std::to_string(WTERMSIG(status)));
        } else {
            exitStatus = -1;
            Logger::getInstance().logError("Child process " + std::to_string(pid) + " exited with unknown status");
        }

        Logger::getInstance().log("Expected exit codes: " + vectorToString(expectedExitCodes));

        if (autoRestart == "always" && !userStopped) {
            Logger::getInstance().log("Restarting child process " + std::to_string(pid) + " as per configuration.");
            this->start();
        } else if (autoRestart == "unexpected") {
            if (!userStopped && std::find(expectedExitCodes.begin(), expectedExitCodes.end(), exitStatus) == expectedExitCodes.end()) {
                Logger::getInstance().logError(
                    "Child process " + std::to_string(pid) + " exited with unexpected status " +
                    std::to_string(exitStatus) + ". Considering restart.");
                this->start();
            }
        }
    }
}

// todo remove vectorToString after testing
std::string Process::vectorToString(const std::vector<int>& vec) const {
    std::stringstream ss;
    ss << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        ss << vec[i];
        if (i < vec.size() - 1) {
            ss << ", ";
        }
    }
    ss << "]";
    return ss.str();
}

// ___________________ STOP AND SYNCH ___________________
void Process::stop() {
    if (getRunningChildCount() == 0) {
        return;
    }

    userStopped = true;

    std::vector<pid_t> pidsToErase;

    while (!child_pids.empty()) {
        pidsToErase.clear();

        for (const pid_t pid : child_pids) {
            if (pid <= 0) continue;

            const bool stopped = stopProcess(pid, pidsToErase);
            if (!stopped) {
                forceStopProcess(pid, pidsToErase);
            }
        }

        cleanupStoppedProcesses(pidsToErase);
    }
    userStopped = false; 
    Logger::getInstance().log("All instances of " + name + " have been successfully stopped.");
}

bool Process::stopProcess(pid_t pid, std::vector<pid_t>& pidsToErase) {
    int status;

    for (int attempt = 0; attempt < stopTime; ++attempt) {
        if (kill(pid, stopSignal) == 0) {
            if (waitpid(pid, &status, WNOHANG) > 0) {
                if (WIFEXITED(status) || WIFSIGNALED(status)) {
                    pidsToErase.push_back(pid);
                    return true;
                }
            }
        } else {
            if (errno == ESRCH) {
                pidsToErase.push_back(pid);
                return true;
            }
        }
        usleep(100000);
    }

    return false;
}

void Process::forceStopProcess(pid_t pid, std::vector<pid_t>& pidsToErase) {
    int status;

    std::cerr << "Unable to stop process with PID " << pid << " gracefully, forcing termination" << std::endl;
    if (kill(pid, SIGKILL) == 0) {
        while (waitpid(pid, &status, WNOHANG) == 0) {
            usleep(100000);  // Wait for the process to exit
        }
        pidsToErase.push_back(pid);
    } else if (errno == ESRCH) {
        pidsToErase.push_back(pid);
    }
}

void Process::cleanupStoppedProcesses(std::vector<pid_t>& pidsToErase) {
    auto shouldBeErased = [&pidsToErase](const pid_t pid) {
        return std::find(pidsToErase.begin(), pidsToErase.end(), pid) != pidsToErase.end();
    };

    child_pids.erase(std::remove_if(child_pids.begin(), child_pids.end(), shouldBeErased), child_pids.end());
}

// ___________________ CHECK LIFECYCLE ___________________
bool Process::isRunning() const {
    return static_cast<int>(child_pids.size()) == instances;
}

std::string Process::getStatus() const {
    return std::to_string(child_pids.size()) + " out of " + std::to_string(instances) + " instances running";
}

std::string Process::getName() const {
    return name;
}