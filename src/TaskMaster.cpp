/*
____ TaskMaster Class ____

    ____ Initialization and Configuration Parsing ____:
    - Handles initialization of process parameters such as command, instances, and environment variables from a JSON configuration.
    - Initializes logging based on configuration settings.

    Functions:
    - TaskMaster(const std::string& configFilePath): Constructor initializing TaskMaster with a configuration file path, displaying usage information, initializing processes, and starting the command loop.
    - ~TaskMaster(): Destructor ensuring all processes are stopped gracefully upon shutdown.
    - initializeLogger(const json& config): Initializes logging based on configuration settings.
    - initializeProcesses(const json& config): Reads and parses configuration data to initialize Process instances for each program defined in the configuration file.
    - startInitialProcesses(): Starts processes configured to begin immediately upon initialization based on their startup time.
    - findProcess(const std::string& processName): Retrieves a pointer to a Process instance by its name for command handling and management.

    ____ Command Handling ____:
    - Manages user commands for process control and status checking.

    Functions:
    - commandLoop(): Enters a loop to continuously accept and process user commands (start, stop, restart, reload, status, exit).
    - handleCommand(const std::string& command): Parses and executes user commands to manage processes.
    - startProcess(const std::string& processName): Initiates the startup of a specified process by name.
    - stopProcess(const std::string& processName): Terminates a running process by name.
    - restartProcess(const std::string& processName): Restarts a process by name.
    - reloadConfig(): Reloads the configuration file, updating or adding processes as necessary.
    - stopAllProcesses(): Stops all managed processes, ensuring all child processes are terminated gracefully.
    - displayStatus(): Retrieves and displays the current running status of all managed processes (Running or Stopped).
    - displayUsage(): Displays usage instructions including implemented and to-be-implemented commands.

    ____ Utility and Signal Handling ____:
    - Provides utility functions and signal handling for process management.

    Functions:
    - signalHandler(int signal): Handles signals such as SIGINT to ensure graceful termination of processes.
    - sendSighupSignalToReload(): Sends a SIGHUP signal to reload the configuration without stopping the program.

*/


#include "TaskMaster.hpp"

std::map<std::string, Process> TaskMaster::processes;
std::string TaskMaster::configFilePath;

// ___________________ INIT AND CONFIG PARSE ___________________
TaskMaster::TaskMaster(const std::string& configFilePath) {
    TaskMaster::configFilePath = configFilePath;
    json config = ConfigManager::parseConfig(configFilePath);
    initializeLogger(config);
    Logger::getInstance().log("TaskMaster created with config file path: " + configFilePath);

    displayUsage();
    initializeProcesses(config);
    commandLoop();
}

TaskMaster::~TaskMaster() {
    stopAllProcesses();
    Logger::getInstance().log("TaskMaster shutting down...");
}

void TaskMaster::initializeLogger(const json& config) {
    const bool loggingEnabled = config.at("logging_enabled").get<bool>();
    const std::string logFilePath = config.at("log_file").get<std::string>();
    Logger::getInstance().initialize(loggingEnabled, logFilePath);
}

void TaskMaster::initializeProcesses(const json& config) {
    try {
        for (const auto& item : config.at("programs").items()) {
            processes.emplace(item.key(), Process(item.key(), item.value()));
            Logger::getInstance().log("Process " + item.key() + " initialized");
        }

        startInitialProcesses();
        signal(SIGINT, Utils::signalHandler);
        signal(SIGQUIT, Utils::signalHandler);
        signal(SIGHUP, Utils::signalHandler);
    } catch (const std::exception& ex) {
        Logger::getInstance().logError("Error initializing processes: " + std::string(ex.what()));
        exit(EXIT_FAILURE);
    }
}

void TaskMaster::startInitialProcesses() {
    for (auto& [name, process] : processes) {
        if (process.getAutoStart() == true) {
            try {
                startProcess(name);
            } catch (const std::exception& ex) {
                Logger::getInstance().logError("Error starting program " + name + ": " + ex.what());
                process.stop();
                exit(1); 
            }
        }
    }
}

Process* TaskMaster::findProcess(const std::string& processName) {
    auto it = processes.find(processName);
    if (it == processes.end()) {
        Logger::getInstance().logError("Process " + processName + " not found");
        return nullptr;
    }
    return &it->second;
}

// ___________________ COMMAND HANDLING ___________________
void TaskMaster::commandLoop() {
    std::string command;
    while (std::cout << GREEN << "taskmaster> " << RESET && std::getline(std::cin, command) && command != "exit") {
        if (std::cin.eof()) {
            break;
        }
        handleCommand(command);
    }
}

void TaskMaster::handleCommand(const std::string &command) {
    Logger::getInstance().logToFile("> " + command);
    const std::vector<std::string> words = Utils::split(command, ' ');

    if (words.empty()) return;

    Command cmd = stringToCommand(words[0]);
    switch (cmd) {
        case Command::Status:
            displayStatus();
            break;
        case Command::Start:
            if (words.size() > 1) {
                startProcess(words[1]);
            } else {
                Logger::getInstance().logError("Invalid command format. Usage: start <process_name>");
            }
            break;
        case Command::Stop:
            if (words.size() > 1) {
                stopProcess(words[1]);
            } else {
                Logger::getInstance().logError("Invalid command format. Usage: stop <process_name>");
            }
            break;
        case Command::Restart:
            if (words.size() > 1) {
                restartProcess(words[1]);
            } else {
                Logger::getInstance().logError("Invalid command format. Usage: restart <process_name>");
            }
            break;
        case Command::Reload:
            if (words.size() > 1) {
                Logger::getInstance().logError("Invalid command format. Usage: reload");
            } else {
                sendSighupSignalToReload();
            }
            break;
        default:
            Logger::getInstance().logError("Unknown command: " + command);
            break;
    }
}

void TaskMaster::sendSighupSignalToReload() {
    kill(getpid(), SIGHUP);
}

void TaskMaster::startProcess(const std::string &processName) {
    Process *process = findProcess(processName);
    if (process != nullptr) {
        process->start();
    }
}

void TaskMaster::stopProcess(const std::string& processName) {
    Process* process = findProcess(processName);
    if (process != nullptr) {
        Logger::getInstance().log("Stopping " + processName);
        process->stop();
    }
}

void TaskMaster::restartProcess(const std::string &processName) {
    Process* process = findProcess(processName);
    if (process != nullptr) {
        Logger::getInstance().log("Restarting " + processName);
        process->stop();
        startProcess(processName);
    }
}

void TaskMaster::reloadConfig() {
    json newConfig;
    try {
        newConfig = ConfigManager::parseConfig(configFilePath);
    }
    catch (const std::exception &ex) {
        Logger::getInstance().logError("Error reloading configuration: " + std::string(ex.what()));
    }
    updateExistingProcesses(newConfig);
    addNewProcesses(newConfig);
    removeOldProcesses(newConfig);
}

void TaskMaster::updateExistingProcesses(const json& newConfig) {
    std::set<std::string> updatedProcesses;

    for (const auto& item : newConfig.at("programs").items()) {
        auto it = processes.find(item.key());
        if (it != processes.end()) {
            it->second.reloadConfig(item.value());
            Logger::getInstance().log("Process " + item.key() + " reloaded");

            updateInstances(it->second, item.value().at("instances").get<int>());
        }
        updatedProcesses.insert(item.key());
    }
}

void TaskMaster::addNewProcesses(const json& newConfig) {
    for (const auto& item : newConfig.at("programs").items()) {
        if (processes.find(item.key()) == processes.end()) {
            processes.emplace(item.key(), Process(item.key(), item.value()));
            Logger::getInstance().log("New process " + item.key() + " added and initialized");

            if (processes.at(item.key()).getAutoStart()) {
                processes.at(item.key()).start();
            }
        }
    }
}

void TaskMaster::removeOldProcesses(const json& newConfig) {
    for (auto it = processes.begin(); it != processes.end();) {
        if (newConfig.at("programs").find(it->first) == newConfig.at("programs").end()) {
            it->second.stop();
            Logger::getInstance().log("Process " + it->first + " removed");
            it = processes.erase(it);
        } else {
            ++it;
        }
    }
}

void TaskMaster::updateInstances(Process& process, int newInstances) {
    int currentInstances = process.getNumberOfInstances();

    if (newInstances > currentInstances && process.getAutoStart()) {
        int instancesToStart = newInstances - currentInstances;
        for (int i = 0; i < instancesToStart; ++i) {
            process.start();
        }
    } else if (newInstances < currentInstances) {
        int instancesToStop = currentInstances - newInstances;
        for (int i = 0; i < instancesToStop; ++i) {
            process.stopInstance();
        }
    }
}


void TaskMaster::stopAllProcesses() {
    for (auto& [_, process] : processes) {
        process.stop();
    }
}

void TaskMaster::displayStatus() {
    for (const auto& [name, process] : processes) {
        Logger::getInstance().log("Process " + name + ": " + process.getStatus());
    }
}

void TaskMaster::displayUsage() {
    Logger::getInstance().log("Usage:");
    Logger::getInstance().log("");
    Logger::getInstance().log("Commands:");
    Logger::getInstance().log("start <program_name>: Start a program by name. (For programs with start_time = 0, not started at taskmaster launch)");
    Logger::getInstance().log("stop <program_name>: Stop a running program by name.");
    Logger::getInstance().log("restart <program_name>: Restart a program by name.");
    Logger::getInstance().log("reload: Reload the configuration without stopping the program.");
    Logger::getInstance().log("status: Show the status of all programs.");
    Logger::getInstance().log("exit: Exit the taskmaster.");
    Logger::getInstance().log("");
}
