/*
____ Process Management Class ____

    ____ Overview ____:
    - Manages child processes based on configuration settings.
    - Provides methods to start, stop, and monitor processes.

    ____ Constructor ____:
    - Initializes the process with a name and configuration data.

    ____ Configuration Parsing ____:
    - Parses a JSON configuration to set up process parameters such as command, instances, environment variables, etc.

    ____ Process Lifecycle ____:
    - start(): Initiates child processes according to configured instances and manages their lifecycle.
    - stop(): Terminates all child processes.
    - isRunning(): Checks if any child processes are currently running.
    - getStatus(): Retrieves the current status of the process (Running or Stopped).

    ____ Child Process Management ____:
    - startChildProcesses(): Forks child processes and manages process creation.
    - monitorChildProcesses(): Monitors child processes for exit and handles exit statuses.
    - terminateAllChildProcesses(): Stops all child processes forcefully in case of errors or termination signals.

    ____ Error Handling ____:
    - Provides robust error handling for process creation failures, execution errors, and termination issues.

    ____ Methods ____:
    - parseConfig(const json& config): Parses JSON configuration data to initialize process parameters.
    - setUpEnvironment(): Sets up environment variables for child processes based on configured values.
    - handleForkFailure(int instanceNumber): Handles failures during child process creation.
    - runChildProcess(): Executes the command for each child process, handles directory changes, and redirects outputs.
    - handleParentProcess(pid_t child_pid, int instanceNumber): Manages parent process responsibilities after forking.

    ____ Additional Features ____:
    - Supports configuration validation for critical parameters like number of instances and stop signals.
    - Implements robust handling for process termination signals and exit statuses.

    ____ Dependencies ____:
    - Requires Utils.hpp for utility functions like string splitting (Utils::split).
    - Utilizes POSIX standard functions for process management (fork, execvp, kill, waitpid).
*/

#include "Process.hpp"
#include "Utils.hpp"


Process::Process(const std::string& name, const json& config)
    : name(name), instances(0), running(false) {
    parseConfig(config);
    // Initialize running state for each instance
    instanceRunning.resize(instances, false);
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

    // Resize instanceRunning vector based on instances
    instanceRunning.resize(instances, false);
}

void Process::setUpEnvironment() {
    for (const auto& [key, value] : environmentVariables) {
        setenv(key.c_str(), value.c_str(), 1);
    }
}

void Process::start() {
    if (instances < 1) {
        throw std::runtime_error("Invalid number of instances: " + std::to_string(instances));
    }
    setUpEnvironment();
    child_pids.clear();
    startChildProcesses();
    std::thread monitorThread(&Process::monitorChildProcesses, this);
    monitorThread.detach();
}

void Process::startChildProcesses() {
    for (int i = 0; i < instances; ++i) {
        pid_t child_pid = fork();
        if (child_pid < 0) {
            handleForkFailure(i + 1);
        } else if (child_pid == 0) {
            runChildProcess();
        } else {
            handleParentProcess(child_pid, i + 1);
        }
    }
}

void Process::handleForkFailure(int instanceNumber) {
    std::cerr << "Failed to fork process for instance " << instanceNumber << std::endl;
    throw std::runtime_error("Fork failure for instance " + std::to_string(instanceNumber));
}

void Process::runChildProcess() {
    if (chdir(workingDirectory.c_str()) != 0) {
        perror("Failed to change directory");
        _exit(EXIT_FAILURE);
    }

    if (umaskInt != -1) {
        ::umask(umaskInt);
    }

    // Redirect stdout and stderr
    int stdoutFd = open(stdoutLog.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    int stderrFd = open(stderrLog.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (stdoutFd < 0 || stderrFd < 0 || dup2(stdoutFd, STDOUT_FILENO) < 0 || dup2(stderrFd, STDERR_FILENO) < 0) {
        perror("Failed to redirect stdout/stderr");
        _exit(EXIT_FAILURE);
    }

    std::vector<std::string> args = Utils::split(command, ' ');
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

void Process::handleParentProcess(pid_t child_pid, int instanceNumber) {
    child_pids.push_back(child_pid);
    instanceRunning[instanceNumber - 1] = true;
    std::cout << "Started process " << name << " instance " << instanceNumber << " with PID " << child_pid << std::endl;
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
                handleErrorWaitingForChildProcess();
            }
        }

        usleep(100000);

        bool allRunning = true;
        for (bool running : instanceRunning) {
            if (!running) {
                allRunning = false;
                break;
            }
        }

        if (child_pids.empty() || !allRunning) {
            running = false;
            break;
        }
    }
}


void Process::handleChildExit(pid_t pid) {
    int status;
    // Wait for the child process to change state and get its exit status
    std::cout << "waitpid in handlechild" << std::endl;
    // if (waitpid(pid, &status, 0) == -1) {
    //     std::cout << "process with pid " << pid << "terminated unexpectedly. Please check logs" << std::endl;
    //     auto it = std::find(child_pids.begin(), child_pids.end(), pid);
    //     if (it != child_pids.end()) {
    //         child_pids.erase(it); // Erase the element pointed to by 'it'
    //     }
    //     perror("waitpid");
    //     return;
    // }

    auto it = std::find(child_pids.begin(), child_pids.end(), pid);
    if (it != child_pids.end()) {
        int index = std::distance(child_pids.begin(), it);
        instanceRunning[index] = false;
        child_pids.erase(it);
    }

    // Log the exit status of the child process
    if (WIFEXITED(status)) {
        std::cout << "Child process " << pid << " exited with status " << WEXITSTATUS(status) << std::endl;
    } else if (WIFSIGNALED(status)) {
        std::cerr << "Child process " << pid << " terminated by signal " << WTERMSIG(status) << std::endl;
    } else {
        std::cerr << "Child process " << pid << " exited with unknown status" << std::endl;
    }
}

// void Process::handleChildExit(pid_t pid) {
//     auto it = std::find(child_pids.begin(), child_pids.end(), pid);
//     if (it != child_pids.end()) {
//         int index = std::distance(child_pids.begin(), it);
//         instanceRunning[index] = false;
//         child_pids.erase(it);
//     }
// }

void Process::handleErrorWaitingForChildProcess() {
    perror("waitpid");
}

// void Process::stop() {
//     // Continue stopping processes until child_pids is empty
//     while (!child_pids.empty()) {
//         // Print the PIDs being stopped
//         std::cout << "PIDs in program being stopped:" << std::endl;
//         for (pid_t pid : child_pids) {
//             std::cout << pid << " ";
//         }
//         std::cout << std::endl;

//         std::vector<pid_t> remainingPids;

//         // Iterate over child_pids to stop processes
//         for (pid_t pid : child_pids) {
//             std::cout << "Stopping process " << name << " instance with PID " << pid << std::endl;

//             if (pid <= 0) {
//                 continue;  // Skip invalid PIDs
//             }

//             int status;
//             if (kill(pid, stopSignal) != 0) {
//                 std::cerr << "Error: Failed to stop process with PID " << pid << std::endl;
//                 remainingPids.push_back(pid);
//                 continue;
//             }

//             // Wait for the process to exit
//             while (true) {
//                 pid_t result = waitpid(pid, &status, WNOHANG);
//                 if (result > 0) {
//                     if (WIFEXITED(status) || WIFSIGNALED(status)) {
//                         std::cout << "Child process " << pid << " exited with status " << WEXITSTATUS(status) << std::endl;
//                         break;
//                     }
//                 } else if (result == 0) {
//                     usleep(100000);  // Wait for a short period
//                 } else if (result == -1) {
//                     if (errno == ECHILD) {
//                         std::cout << "No more child processes to wait for" << std::endl;
//                         break;
//                     } else {
//                         std::cerr << "Error waiting for process " << pid << " to exit: " << strerror(errno) << std::endl;
//                         break;
//                     }
//                 }
//             }
//         }

//         // Update child_pids with only those PIDs that were not successfully stopped
//         child_pids = remainingPids;
//     }

//     std::cout << "All processes stopped." << std::endl;
// }

void Process::stop() {
    std::vector<pid_t> pidsToErase;

    // Continue stopping processes until child_pids is empty
    while (!child_pids.empty()) {
        pidsToErase.clear();

        // Print the PIDs being stopped
        std::cout << "PIDs in program being stopped:" << std::endl;
        for (pid_t pid : child_pids) {
            std::cout << pid << " ";
        }
        std::cout << std::endl;

        // Iterate over child_pids to stop processes
        for (pid_t pid : child_pids) {
            std::cout << "Stopping process " << name << " instance with PID " << pid << std::endl;

            if (pid <= 0) {
                continue;  // Skip invalid PIDs
            }

            int status;
            if (kill(pid, stopSignal) != 0) {
                std::cerr << "Error: Failed to stop process with PID " << pid << std::endl;
                continue;
            }

            // Wait for the process to exit
            while (true) {
                pid_t result = waitpid(pid, &status, WNOHANG);
                if (result > 0) {
                    if (WIFEXITED(status) || WIFSIGNALED(status)) {
                        std::cout << "Child process " << pid << " exited with status " << WEXITSTATUS(status) << std::endl;
                        pidsToErase.push_back(pid);
                        break;
                    }
                } else if (result == 0) {
                    usleep(100000);  // Wait for a short period
                } else if (result == -1) {
                    if (errno == ECHILD) {
                        std::cout << "No more child processes to wait for" << std::endl;
                        break;
                    } else {
                        std::cerr << "Error waiting for process " << pid << " to exit: " << strerror(errno) << std::endl;
                        break;
                    }
                }
            }
        }

        // Remove the stopped PIDs from child_pids
        for (pid_t pid : pidsToErase) {
            auto it = std::find(child_pids.begin(), child_pids.end(), pid);
            if (it != child_pids.end()) {
                child_pids.erase(it);
            }
        }
    }

    std::cout << "All processes stopped." << std::endl;
}





void Process::terminateAllChildProcesses() {
    for (pid_t pid : child_pids) {
        std::cout << "Stopping process " << name << " instance with PID " << pid << std::endl;
        if (kill(pid, stopSignal) != 0) {
            throw std::runtime_error("Failed to stop process with PID " + std::to_string(pid));
        }
    }
    child_pids.clear();
    // cleanUpRemainingChildProcesses();
}

void Process::cleanUpRemainingChildProcesses() {
    while (!child_pids.empty()) {
        int status;
        pid_t pid = waitpid(-1, &status, 0);
        if (pid > 0) {
            auto it = std::find(child_pids.begin(), child_pids.end(), pid);
            if (it != child_pids.end()) {
                child_pids.erase(it);
            }
            if (WIFEXITED(status)) {
                // std::cout << "Child process " << pid << " exited with status " << WEXITSTATUS(status) << std::endl;
            } else if (WIFSIGNALED(status)) {
                // std::cerr << "Child process " << pid << " terminated by signal " << WTERMSIG(status) << std::endl;
            }
        } else if (pid == -1) {
            perror("waitpid");
            exit(EXIT_FAILURE);
        }
    }
}

bool Process::isRunning() const {
    int runningCount = 0;    
    for (pid_t pid : child_pids) {
        if (pid)
            runningCount += 1;
    }
    return (runningCount == instances);
}

std::string Process::getStatus() const {
    int runningCount = 0;    
    for (pid_t pid : child_pids) {
        if (pid)
            runningCount += 1;
    }
    // int runningCount = std::count(instanceRunning.begin(), instanceRunning.end(), true);
    return std::to_string(runningCount) + " out of " + std::to_string(instances) + " instances running";
}

std::string Process::getName() const {
    return name;
}