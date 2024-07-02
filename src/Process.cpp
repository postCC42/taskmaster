#include "Process.hpp"
#include "Utils.hpp"

Process::Process(const std::string& name, const json& config)
    : name(name) {
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

// void Process::start() {
//     setUpEnvironment();
//     // we need to ensure that supervised program runs independently => fork create child process

//     // TODO: should be a loop for multiple instances
//     pid = fork();
//     if (pid < 0) {
//         throw std::runtime_error("Failed to fork process");
//     } else if (pid == 0) {
//         if (chdir(workingDirectory.c_str()) != 0) {
//             throw std::runtime_error("Failed to change directory to " + workingDirectory);
//         }

//          if (umaskInt != -1) {
//             ::umask(umaskInt);
//         }

//         // Redirect stdout and stderr
//         int stdoutFd = open(stdoutLog.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
//         int stderrFd = open(stderrLog.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
//         dup2(stdoutFd, STDOUT_FILENO);
//         dup2(stderrFd, STDERR_FILENO);

//         std::vector<std::string> args = Utils::split(command, ' ');
//         const char* argv[args.size() + 1];
//         for (size_t i = 0; i < args.size(); ++i) {
//             argv[i] = args[i].c_str();
//         }
//         argv[args.size()] = nullptr;

//         execvp(argv[0], const_cast<char* const*>(argv));

//         _exit(EXIT_FAILURE);
//     } else {
//         std::cout << "Started process " << name << " with PID " << pid << std::endl;
//     }
// }


void Process::start() {
    // todo remove check n. instances when added check conf file func
    if (instances < 1) {
        throw std::runtime_error("Invalid number of instances: " + std::to_string(instances));
    }
    setUpEnvironment();
    child_pids.clear(); 

    for (int i = 1; i <= instances; ++i) {
        pid_t child_pid = fork();
        if (child_pid < 0) {
            std::cerr << "Failed to fork process for instance " << i << std::endl;
            for (pid_t pid : child_pids) {
                kill(pid, SIGKILL);
            }
            std::cerr << "Exiting due to fork failure." << std::endl;
            exit(EXIT_FAILURE);
        } else if (child_pid == 0) {
            // Child process
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
        } else {
            // Parent process
            child_pids.push_back(child_pid);
            std::cout << "Started process " << name << " instance " << i << " with PID " << child_pid << std::endl;
        }
    }

    monitorChildProcesses();
}

void Process::monitorChildProcesses() {
    // Parent waits for child processes to terminate
    bool failed_process = false;
    
    while (!child_pids.empty()) {
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG);
        if (pid != 0 && pid != -1) {
            // stop();
            // Child process has terminated
            auto it = std::find(child_pids.begin(), child_pids.end(), pid);
            if (it != child_pids.end()) {
                child_pids.erase(it);
            }
            if (WIFEXITED(status)) {
                std::cout << "Child process " << pid << " exited with status " << WEXITSTATUS(status) << std::endl;
                if (WEXITSTATUS(status) != 0) {
                    std::cerr << "Child process " << pid << " failed. Exiting." << std::endl;
                    failed_process = true;
                    for (pid_t p : child_pids) {
                        kill(p, SIGKILL);
                    }
                    break;
                }
            } else if (WIFSIGNALED(status)) {
                std::cerr << "Child process " << pid << " terminated by signal " << WTERMSIG(status) << std::endl;
                std::cerr << "Exiting due to child process termination by signal." << std::endl;
                failed_process = true;
                for (pid_t p : child_pids) {
                    kill(p, SIGKILL);
                }
                break;
            }
        } else if (pid == -1) {
            // Error waiting for child process
            perror("waitpid");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            break;
        }
    }

    std::cout << "failed_process =  " << failed_process << std::endl;
    if (failed_process) {
        // Cleanup remaining child processes
        while (!child_pids.empty()) {
            int status;
            pid_t pid = waitpid(-1, &status, 0);
            if (pid > 0) {
                auto it = std::find(child_pids.begin(), child_pids.end(), pid);
                if (it != child_pids.end()) {
                    child_pids.erase(it);
                }
                if (WIFEXITED(status)) {
                    std::cout << "Child process " << pid << " exited with status " << WEXITSTATUS(status) << std::endl;
                } else if (WIFSIGNALED(status)) {
                    std::cerr << "Child process " << pid << " terminated by signal " << WTERMSIG(status) << std::endl;
                }
            } else if (pid == -1) {
                perror("waitpid");
                exit(EXIT_FAILURE);
            }
        }
        exit(EXIT_FAILURE);
    }

    std::cout << "All child processes completed successfully." << std::endl;
    // exit(EXIT_SUCCESS);
}


void Process::stop() {
    if (child_pids.empty()) {
        throw std::runtime_error("No child processes to stop");
    }
    for (pid_t pid : child_pids) {
        // check pid <= 0 : je ne suis pas sur de comprendre. il ne semble pas necessaire, vu que pid = 0 est le process child qu'on veut tuer et pid < 0 est un erreur deja traité
        // if (pid <= 0) {
        //     throw std::runtime_error("No valid process ID");
        // }
        std::cout << "Stopping process " << name << " instance with PID " << pid << std::endl;
        if (kill(pid, stopSignal) != 0) {
            throw std::runtime_error("Failed to stop process with PID " + std::to_string(pid));
        }
    }

    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, 0)) > 0) {
        auto it = std::find(child_pids.begin(), child_pids.end(), pid);
        if (it != child_pids.end()) {
            child_pids.erase(it);
        }
        if (WIFEXITED(status)) {
            // std::cout << "Child process " << pid << " exited with status " << WEXITSTATUS(status) << std::endl;
        } else if (WIFSIGNALED(status)) {
            // std::cerr << "Child process " << pid << " terminated by signal " << WTERMSIG(status) << std::endl;
        }
    }

    if (pid == -1 && errno != ECHILD) {
        perror("waitpid");
        exit(EXIT_FAILURE);
    }

    std::cout << "All processes instances of " << name << " stopped." << std::endl;
    child_pids.clear(); 
    // pid = -1;
}

bool Process::isRunning() const {
    if (child_pids.empty()) {
        return false;
    }

    for (pid_t pid : child_pids) {
        int status;
        pid_t result = waitpid(pid, &status, WNOHANG);
        if (result == 0) {
            // Child process is still running
            return true;
        } else if (result == pid) {
            // Child process has exited
            return false;
        } else if (result == -1) {
            throw std::runtime_error("Error checking process status");
        }
    }

    return false;
}

std::string Process::getStatus() const {
    if (isRunning()) {
        return "Running";
    } else {
        return "Stopped";
    }
}

std::string Process::getName() const {
    return name;
}
