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
    - handleChildExit(pid_t pid, int status): Handles child process exit events and potentially restarts based on configuration.

    ____ Process Stop and Synchronization ____:
    - Handles termination of all child processes, including graceful and forced termination methods.

    Functions:
    - stop(): Initiates the process to stop all child processes.
    - stopInstance(): Stops the most recent instance of the process.
    - stopProcess(pid_t pid, std::vector<pid_t>& pidsToErase): Stops a specific child process gracefully.
    - forceStopProcess(pid_t pid, std::vector<pid_t>& pidsToErase): Forces termination of a specific child process.
    - cleanupStoppedProcesses(std::vector<pid_t>& pidsToErase): Cleans up and removes stopped child processes from tracking.
    - notifyAllStopped(): Notifies when all instances of the program have been successfully stopped.

    ____ Configuration Reload and Dynamic Updates ____:
    - Supports reloading configuration and applying dynamic updates without restart when possible.

    Functions:
    - detectChanges(const json& newConfig): Detects changes between current and new configurations.
    - reloadConfig(const json& newConfig): Reloads configuration and applies changes, optionally restarting as needed.
    - changesRequireRestart(const ConfigChangesMap& changes): Checks if detected changes require a restart.
    - updateDinamicallyWithoutRestarting(const ConfigChangesMap& changes): Applies non-restart-required changes dynamically.
    - updateUmask(std::string newValue): Updates umask dynamically based on configuration changes.

    ____ Lifecycle and Status Checking ____:
    - Provides methods to check if processes are running and retrieve their current status.

    Functions:
    - isRunning(): Checks if all configured instances of the process are currently running.
    - getNumberOfInstances(): Returns the number of currently running child processes.
    - getStatus(): Retrieves the current status of the process (Running or Stopped).
    - getName(): Retrieves the name of the process.
*/


#include "Process.hpp"

Process::Process(Process&& other) noexcept
    : name(std::move(other.name)),
      command(std::move(other.command)),
      instances(other.instances),
      autoStart(other.autoStart),
      autoRestart(std::move(other.autoRestart)),
      startTime(other.startTime),
      stopTime(other.stopTime),
      restartAttempts(other.restartAttempts),
      stopSignal(other.stopSignal),
      expectedExitCodes(std::move(other.expectedExitCodes)),
      workingDirectory(std::move(other.workingDirectory)),
      umaskInt(other.umaskInt),
      stdoutLog(std::move(other.stdoutLog)),
      stderrLog(std::move(other.stderrLog)),
      environmentVariables(std::move(other.environmentVariables)),
      childPids(std::move(other.childPids)),
      newConfigFile(std::move(other.newConfigFile)),
      stopAutoRestart(other.stopAutoRestart.load()) {
}

Process& Process::operator=(Process&& other) noexcept {
    if (this != &other) {
        name = std::move(other.name);
        command = std::move(other.command);
        instances = other.instances;
        autoStart = other.autoStart;
        autoRestart = std::move(other.autoRestart);
        startTime = other.startTime;
        stopTime = other.stopTime;
        restartAttempts = other.restartAttempts;
        stopSignal = other.stopSignal;
        expectedExitCodes = std::move(other.expectedExitCodes);
        workingDirectory = std::move(other.workingDirectory);
        umaskInt = other.umaskInt;
        stdoutLog = std::move(other.stdoutLog);
        stderrLog = std::move(other.stderrLog);
        environmentVariables = std::move(other.environmentVariables);
        childPids = std::move(other.childPids);
        newConfigFile = std::move(other.newConfigFile);
        stopAutoRestart.store(other.stopAutoRestart.load());
    }
    return *this;
}


// ___________________ INIT AND PARSE ___________________
Process::Process(const std::string& name, const json& config) : name(name), instances(0) {
    parseConfig(config);
}

Process::~Process() {
    stopThread();
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
    Logger::getInstance().log("Starting " + name);

    stopAutoRestart.store(true);
    int attempts = 0;
    constexpr int oneSecond = 1000000;
    const int totalSleepTime = startTime * oneSecond;
    const int intervalsNeeded = totalSleepTime / oneSecond;
    do {
        try {
            startChildProcessAndMonitor();
            for (int i = 0; i < intervalsNeeded; ++i) {
                usleep(oneSecond);
                if (isRunning()) {
                    break;
                }
            }

            if (isRunning()) {
                Logger::getInstance().log("Process " + name + " started successfully");
                stopAutoRestart.store(false);
                break;
            }
            Logger::getInstance().logError("Attempt " + std::to_string(attempts + 1) + " failed to start " + name);
        } catch (const std::exception &ex) {
            Logger::getInstance().logError("Error starting program " + name + ": " + ex.what());
            stop();
        }
        if (attempts == restartAttempts) {
            Logger::getInstance().logError("Maximum restart attempts reached for " + name);
            stop();
            break;
        }
        attempts++;
    } while (attempts < restartAttempts);
}

void Process::startChildProcessAndMonitor() {
    for (int i = getRunningChildCount(); i < instances; ++i) {
        pid_t child_pid = fork();
        if (child_pid < 0) {
            throw std::runtime_error("Fork failure for instance " + std::to_string(i));
        }
        if (child_pid == 0) {
            setUpEnvironment();
            runChildProcess();
        } else {
            Logger::getInstance().log(
                name + " instance " + std::to_string(i) + " started with PID " + std::to_string(child_pid) + ".");
            {
                std::lock_guard<std::mutex> guard(childPidsMutex);
                childPids.push_back(child_pid);
            }
        }
    }

    // TODO: test if this works when reloading
    if (monitorThreadRunning.load() == false) {
        std::thread monitorThread(&Process::monitorChildProcesses, this);
        monitorThread.detach();
        monitorThreadRunning.store(true);
    }
}

int Process::getRunningChildCount() {
    int runningChildCount = 0;
    for (const pid_t& pid : childPids) {
        if (kill(pid, 0) == 0) {
            runningChildCount++;
        } else if (errno != ESRCH) {
            Logger::getInstance().logError("Error checking process status for PID " + std::to_string(pid) + ": " + std::strerror(errno));
        }
    }
    return runningChildCount;
}

void Process::runChildProcess() const {
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
    while (!stopRequested.load()) {
        for (auto it = childPids.begin(); it != childPids.end();) {
            int status;
            pid_t pid = *it;
            if (waitpid(pid, &status, WNOHANG) > 0) {
                handleChildExit(pid, status);
                safeEraseFromChildPids(it);
            } else if (pid == -1) {
                // TODO: how to manage this ?
                Logger::getInstance().logError("waitpid error: " + std::string(strerror(errno)));
            } else {
                ++it;
            }
        }
        if (safeChildPidsIsEmpty()) {
            break;
        }
        usleep(100000);
    }
    monitorThreadRunning.store(false);
}

void Process::handleChildExit(pid_t pid, int status) {
    int exitStatus;
    if (WIFEXITED(status)) {
        exitStatus = WEXITSTATUS(status);
        Logger::getInstance().log(
            "Child process " + std::to_string(pid) + " exited with status " + std::to_string(exitStatus));
    } else if (WIFSIGNALED(status)) {
        exitStatus = WTERMSIG(status);
        Logger::getInstance().logError(
            "Child process " + std::to_string(pid) + " terminated by signal " + std::to_string(WTERMSIG(status)));
    } else {
        exitStatus = -1;
        Logger::getInstance().logError("Child process " + std::to_string(pid) + " exited with unknown status");
    }

    if (stopAutoRestart.load() == true) return;

    if (autoRestart == "always") {
        Logger::getInstance().log("Restarting child process " + std::to_string(pid) + " as per configuration.");
        this->start();
    } else if (autoRestart == "unexpected") {
        if (std::find(expectedExitCodes.begin(), expectedExitCodes.end(), exitStatus) == expectedExitCodes.end()) {
            Logger::getInstance().logError(
                "Child process " + std::to_string(pid) + " exited with unexpected status " +
                std::to_string(exitStatus) + ". Considering restart.");
            this->start();
        }
    }
}

// ___________________ STOP AND SYNCH ___________________
void Process::stop() {
    std::cout << "Running child count: " << getRunningChildCount() << std::endl;
    if (getRunningChildCount() == 0) {
        return;
    }

    stopAutoRestart.store(false);
    stopThread();

    std::vector<pid_t> pidsToErase;

    while (!safeChildPidsIsEmpty()) {
        pidsToErase.clear();

        for (const pid_t pid : safeGetChildPidsCopy()) {
            if (pid <= 0) continue;

            const bool stopped = stopProcess(pid, pidsToErase);
            if (!stopped) {
                forceStopProcess(pid, pidsToErase);
            }
        }

        cleanupStoppedProcesses(pidsToErase);
    }
    stopAutoRestart.store(false);
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
        // usleep(100000);
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

    std::lock_guard<std::mutex> guard(childPidsMutex);
    childPids.erase(std::remove_if(childPids.begin(), childPids.end(), shouldBeErased), childPids.end());
}

void Process::stopInstance() {
     if (!safeChildPidsIsEmpty()) {
        pid_t lastPid = childPids.back();
        std::vector<pid_t> pidsToErase;

        if (!stopProcess(lastPid, pidsToErase)) {
            forceStopProcess(lastPid, pidsToErase);
        }

        cleanupStoppedProcesses(pidsToErase);

        Logger::getInstance().log("Stopped and removed " + name + " process with PID: " + std::to_string(lastPid));
    } else {
        Logger::getInstance().log("No child processes to stop and remove.");
    }
}

// ___________________ RELOAD ___________________

void Process::reloadConfig(const json& newConfig) {
    Logger::getInstance().log("Reloading config for process: " + name);

    ConfigChangesMap changes = detectChanges(newConfig);
    for (const auto& change : changes) {
        Logger::getInstance().log("Detected change: " + change.first + " -> " + change.second);
    }

    if (!changes.empty()) {
        applyChanges(changes);
        if (changesRequireRestart(changes)) {
            Logger::getInstance().log("Some changes require a restart for process: " + name);
            stopAutoRestart = true;
            stop();
            if (autoStart) {
                start();
            }
        }
    } else {
        Logger::getInstance().log("No changes detected for " + name);
    }
}

bool Process::changesRequireRestart(const ConfigChangesMap& changes) {
    static const std::vector<std::string> restartKeys = {
        "command", "auto_start", "auto_restart", "working_directory", "stdout_log", "stderr_log",
        "environment_variables", "start_time", "stop_time", "restart_attempts", "umask"
    };

    for (const auto& key : restartKeys) {
        if (changes.find(key) != changes.end()) {
            return true;
        }
    }
    return false;
}

void Process::updateUmask(const std::string &newValue) {
    std::cout << "Updating umask to: " << newValue << std::endl;
    if (umaskInt != -1) {
        ::umask(umaskInt);
    }
}

ConfigChangesMap Process::detectChanges(const json& newConfig) {
    ConfigChangesMap changes;

    ConfigManager::checkCommand(newConfig, *this, changes);
    ConfigManager::checkInstances(newConfig, *this, changes);
    ConfigManager::checkAutoStart(newConfig, *this, changes);
    ConfigManager::checkAutoRestart(newConfig, *this, changes);
    ConfigManager::checkStartTime(newConfig, *this, changes);
    ConfigManager::checkStopTime(newConfig, *this, changes);
    ConfigManager::checkRestartAttempts(newConfig, *this, changes);
    ConfigManager::checkStopSignal(newConfig, *this, changes);
    ConfigManager::checkExpectedExitCodes(newConfig, *this, changes);
    ConfigManager::checkWorkingDirectory(newConfig, *this, changes);
    ConfigManager::checkUmask(newConfig, *this, changes);
    ConfigManager::checkStdoutLog(newConfig, *this, changes);
    ConfigManager::checkStderrLog(newConfig, *this, changes);
    ConfigManager::checkEnvironmentVariables(newConfig, *this, changes);

    return changes;
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
            // TODO: test
            // Already handled
        } else if (key == "working_directory") {
            workingDirectory = value;
        } else if (key == "umask") {
            umaskInt = std::stoi(value);
        } else if (key == "stdout_log") {
            stdoutLog = value;
        } else if (key == "stderr_log") {
            stderrLog = value;
        } else if (key == "environment_variables") {
            environmentVariables = ConfigManager::deserializeEnvVars(value);
        }
    }
}

// ___________________ CHECK LIFECYCLE ___________________
bool Process::isRunning() {
    return getNumberOfInstances() == instances;
}

int Process::getNumberOfInstances() {
    std::lock_guard<std::mutex> guard(childPidsMutex);
    return static_cast<int>(childPids.size());
}

std::string Process::getStatus() const {
    return std::to_string(childPids.size()) + " out of " + std::to_string(instances) + " instances running";
}

std::string Process::getName() const {
    return name;
}

// ___________________ MUTEX ___________________
void Process::safeEraseFromChildPids(std::vector<pid_t>::iterator it) {
    std::lock_guard<std::mutex> guard(childPidsMutex);
    childPids.erase(it);
}

bool Process::safeChildPidsIsEmpty() {
    std::lock_guard<std::mutex> guard(childPidsMutex);
    return childPids.empty();
}

std::vector<pid_t> Process::safeGetChildPidsCopy() {
    std::lock_guard<std::mutex> guard(childPidsMutex);
    std::vector<pid_t> copy = childPids;
    return copy;
}

void Process::stopThread() {
    stopRequested.store(true);
    while (monitorThreadRunning.load() != false) {
        usleep(100000);
    }
    if (thread.joinable()) {
        thread.join();
    }
}
