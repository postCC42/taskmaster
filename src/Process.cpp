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

void Process::start() {
    // todo remove check n. instances when added check conf file func
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
    for (int i = 1; i <= instances; ++i) {
        pid_t child_pid = fork();
        if (child_pid < 0) {
            handleForkFailure(i);
        } else if (child_pid == 0) {
            runChildProcess();
        } else {
            handleParentProcess(child_pid, i);
        }
    }
}

void Process::handleForkFailure(int instanceNumber) {
    std::cerr << "Failed to fork process for instance " << instanceNumber << std::endl;
    for (pid_t pid : child_pids) {
        kill(pid, SIGKILL);
    }
    std::cerr << "Exiting due to fork failure." << std::endl;
    exit(EXIT_FAILURE);
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
    std::cout << "Started process " << name << " instance " << instanceNumber << " with PID " << child_pid << std::endl;
}

void Process::monitorChildProcesses() {

    while (true) {
        int status;
        pid_t pid = waitChildProcess(status);
        if (pid > 0) {
            handleChildExit(pid, status);
        } else if (pid == -1) {
            handleErrorWaitingForChildProcess();
        }

        usleep(100000);

        if (child_pids.empty()) {
            break;
        }
    }
}

pid_t Process::waitChildProcess(int& status) {
    pid_t pid = waitpid(-1, &status, WNOHANG);
    return pid;
}

void Process::handleChildExit(pid_t pid, int status) {
    auto it = std::find(child_pids.begin(), child_pids.end(), pid);
    if (it != child_pids.end()) {
        child_pids.erase(it);
    }

    if (WIFEXITED(status)) {
        handleNormalChildExit(pid, status);
    } else if (WIFSIGNALED(status)) {
        handleSignalTermination(pid, status);
    }
}

void Process::handleNormalChildExit(pid_t pid, int status) {
    // std::cout << "Child process " << pid << " exited with status " << WEXITSTATUS(status) << std::endl;
    if (WEXITSTATUS(status) != 0) {
        std::cerr << "Child process " << pid << " failed. Exiting." << std::endl;
        std::cerr << std::endl;
        std::cerr << "If one of the instances fails to start, our policy is to stop all the processes and exit" << std::endl;
        std::cerr << "Please check the logs at " << stderrLog << ",  and try again" << std::endl;
        std::cerr << std::endl;
        terminateAllChildProcesses();
        exit(EXIT_FAILURE);
    }
}

void Process::handleSignalTermination(pid_t pid, int status) {
    std::cerr << "Child process " << pid << " terminated by signal " << WTERMSIG(status) << std::endl;
    std::cerr << "Exiting due to child process termination by signal." << std::endl;
    terminateAllChildProcesses();
    exit(EXIT_FAILURE);
}


void Process::terminateAllChildProcesses() {
    for (pid_t pid : child_pids) {
        std::cout << "Stopping process " << name << " instance with PID " << pid << std::endl;
        if (kill(pid, stopSignal) != 0) {
            throw std::runtime_error("Failed to stop process with PID " + std::to_string(pid));
        }
    }
    cleanUpRemainingChildProcesses();
    child_pids.clear();
}

void Process::handleErrorWaitingForChildProcess() {
    perror("waitpid");
    exit(EXIT_FAILURE);
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



void Process::stop() {
    if (child_pids.empty()) {
        throw std::runtime_error("No child processes to stop");
    }
    terminateAllChildProcesses();

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
