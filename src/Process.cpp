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
#include "Utils.hpp"


// ___________________ INIT AND PARSE ___________________
Process::Process(const std::string& name, const json& config) : name(name), instances(0) {
    parseConfig(config);
}

// void Process::parseConfig(const json& config) {
//     command = config.at("command").get<std::string>();
//     instances = config.at("instances").get<int>();
//     if (instances < 0) throw std::runtime_error("Invalid number of instances: " + std::to_string(instances));
//     autoStart = config.at("auto_start").get<bool>();
//     autoRestart = config.at("auto_restart").get<std::string>();
//     if (autoRestart != "always" && autoRestart != "never" && autoRestart != "unexpected") {
//         throw std::runtime_error("Invalid auto restart value: " + autoRestart);
//     }
//     startTime = config.at("start_time").get<int>();
//     if (startTime < 0) throw std::runtime_error("Invalid start time: " + std::to_string(startTime));
//     stopTime = config.at("stop_time").get<int>();
//     if (stopTime < 0) throw std::runtime_error("Invalid stop time: " + std::to_string(stopTime));
//     restartAttempts = config.at("restart_attempts").get<int>();
//     if (restartAttempts < 0) throw std::runtime_error("Invalid restart attempts: " + std::to_string(restartAttempts));
//
//     std::string stopSignalStr = config.at("stop_signal").get<std::string>();
//     if (signalMap.find(stopSignalStr) == signalMap.end()) {
//         throw std::runtime_error("Invalid stop signal: " + stopSignalStr);
//     }
//     stopSignal = signalMap.at(stopSignalStr);
//
//     expectedExitCodes = config.at("expected_exit_codes").get<std::vector<int>>();
//     workingDirectory = config.at("working_directory").get<std::string>();
//     umaskInt = config.at("umask").get<int>();
//     stdoutLog = config.at("stdout_log").get<std::string>();
//     stderrLog = config.at("stderr_log").get<std::string>();
//     for (const auto& envVar : config.at("environment_variables")) {
//         std::string envVarStr = envVar.get<std::string>();
//         const auto delimiterPos = envVarStr.find('=');
//         auto key = envVarStr.substr(0, delimiterPos);
//         const auto value = envVarStr.substr(delimiterPos + 1);
//         environmentVariables[key] = value;
//     }
// }
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
        // TODO: can we use execvp instead of setenv?
        setenv(key.c_str(), value.c_str(), 1);
    }
}

// ___________________ START AND MONITOR ___________________
void Process::start() {
    // TODO: test
    if (instances < 1) {
        throw std::runtime_error("Invalid number of instances: " + std::to_string(instances));
    }
    setUpEnvironment();
    child_pids.clear();

    for (int i = 0; i < instances; ++i) {
        pid_t child_pid = fork();
        if (child_pid < 0) {
            throw std::runtime_error("Fork failure for instance " + std::to_string(i));
        }
        if (child_pid == 0) {
            runChildProcess();
        } else {
            child_pids.push_back(child_pid);
        }
    }

    std::thread monitorThread(&Process::monitorChildProcesses, this);
    monitorThread.detach();
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
    while (true) {
        int status;

        pid_t pid = waitpid(-1, &status, WNOHANG);
        if (pid > 0) {
            handleChildExit(pid);
        } else if (pid == -1) {
            if (errno == ECHILD) {
                // No more child processes to wait for
                break;
            } else {
                perror("waitpid");
            }
        }

        usleep(100000);

        if (child_pids.empty()) {
            break;
        }
    }
}


void Process::handleChildExit(pid_t pid) {
    int status;

    auto it = std::find(child_pids.begin(), child_pids.end(), pid);
    if (it != child_pids.end()) {
        child_pids.erase(it);
    }

    if (WIFEXITED(status)) {
        // std::cout << "Child process " << pid << " exited with status " << WEXITSTATUS(status) << std::endl;
    } else if (WIFSIGNALED(status)) {
        // std::cerr << "Child process " << pid << " terminated by signal " << WTERMSIG(status) << std::endl;
    } else {
        // std::cerr << "Child process " << pid << " exited with unknown status" << std::endl;
    }
}

// ___________________ STOP AND SYNCH ___________________
void Process::stop() {
    if (checkNoInstancesLeft()) {
        return;
    }

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

    notifyAllStopped();
}

bool Process::checkNoInstancesLeft() const {
    if (child_pids.empty()) {
        std::cout << "Checking... No instances of " << GREEN << name << RESET << " left to stop." << std::endl;
        return true;
    }
    return false;
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
        usleep(100000);  // Wait before retrying
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

void Process::notifyAllStopped() const {
    std::cout << "All instances of " << GREEN << name << RESET << " have been successfully stopped." << std::endl;
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