/*
____ TaskMaster Class ____

    ____ Initialization and Configuration Parsing ____:
    - Handles initialization of process parameters such as command, instances, and environment variables from a JSON configuration.

    Functions:
    - TaskMaster(const std::string& configFilePath): Constructor initializing TaskMaster with a configuration file path, displaying usage information, initializing processes, and starting the command loop.
    - initializeProcesses(): Reads and parses configuration data to initialize Process instances for each program defined in the configuration file.
    - startInitialProcesses(): Starts processes configured to begin immediately upon initialization based on their startup time.
    - findProcess(const std::string& processName): Retrieves a pointer to a Process instance by its name for command handling and management.

    ____ Command Handling ____:
    - Manages user commands for process control and status checking.

    Functions:
    - commandLoop(): Enters a loop to continuously accept and process user commands (start, stop, status, exit).
    - handleCommand(const std::string& command): Parses and executes user commands to manage processes (start <process_name>, stop <process_name>, status, exit).
    - startProcess(const std::string& processName): Initiates the startup of a specified process by name, handles startup errors, and displays status messages.
    - stopProcess(const std::string& processName): Terminates a running process by name, ensuring proper cleanup and management of child processes.
    - stopAllProcesses(): Stops all managed processes, ensuring all child processes are terminated gracefully.
    - displayStatus(): Retrieves and displays the current running status of all managed processes (Running or Stopped).
    - stringToCommand(const std::string& commandStr): Converts a string command to its corresponding enum Command type for command processing.
    - displayUsage(): Displays usage instructions including implemented and to-be-implemented commands.

    ____ Utility and Signal Handling ____:
    - Provides utility functions and signal handling for process management.

    Functions:
    - signalHandler(int signal): Handles signals such as SIGINT to ensure graceful termination of processes.

*/

#include "TaskMaster.hpp"
#include "Utils.hpp"

std::map<std::string, Process> TaskMaster::processes;

// ___________________ INIT AND CONFIG PARSE ___________________


TaskMaster::TaskMaster(const std::string& configFilePath) : configFilePath(configFilePath), configParser(configFilePath) {
    std::cout << "TaskMaster created with config file path: " << configFilePath << std::endl;
    displayUsage();
    initializeProcesses();
    commandLoop();
}

void TaskMaster::initializeProcesses() {
    json config = configParser.getConfig();

    try {
        for (const auto& item : config.at("programs").items()) {
            processes.emplace(item.key(), Process(item.key(), item.value()));
            std::cout << "Process " << item.key() << " initialized" << std::endl;
        }

        startInitialProcesses();
        signal(SIGINT, Utils::sigintHandler);
    } catch (const std::exception& ex) {
        // todo handle restart attempt and logs to user
        // std::cerr << "Error starting program " << name << ": " << ex.what() << std::endl;
        stopAllProcesses();
        exit(1); 
    }
}

void TaskMaster::startInitialProcesses() {
    for (auto& [name, process] : processes) {
        if (process.getStartTime() == 1) {
            try {
                startProcess(name);
            } catch (const std::exception& ex) {
                std::cerr << "Error starting program " << name << ": " << ex.what() << std::endl;
                process.stop();
                exit(1); 
            }
        }
    }
}

Process* TaskMaster::findProcess(const std::string& processName) {
    auto it = processes.find(processName);
    if (it != processes.end()) {
        return &it->second;
    } else {
        std::cout << "Process not found: " << processName << std::endl;
        return nullptr;
    }
}

// ___________________ COMMAND HANDLING ___________________


void TaskMaster::commandLoop() {
    std::string command;
    while (true) {
        std::cout << YELLOW << "taskmaster> " << RESET;
        std::getline(std::cin, command);
        if (std::cin.eof()) {
            break;
        }

        if (command == "exit") {
            stopAllProcesses();
            break;
        }
        handleCommand(command);
    }
}

void TaskMaster::handleCommand(const std::string &command) {
    std::vector<std::string> words = Utils::split(command, ' ');

    if (words.empty()) {
        std::cout << "Invalid command." << std::endl;
        return;
    }

    Command cmd = stringToCommand(words[0]);
    switch (cmd) {
        case Command::Status:
            displayStatus();
            break;
        case Command::Start:
            if (words.size() > 1) {
                startProcess(words[1]);
                
            } else {
                std::cout << "Invalid command format. Usage: start <process_name>" << std::endl;
            }
            break;
        case Command::Stop:
            if (words.size() > 1) {
                stopProcess(words[1]);
            } else {
                std::cout << "Invalid command format. Usage: stop <process_name>" << std::endl;
            }
            break;
        default:
            std::cout << "Unknown command: " << command << std::endl;
            break;
    }
}



void TaskMaster::startProcess(const std::string& processName) {
    Process* process = findProcess(processName);
    if (process != nullptr) {
         try {
                process->start();
                usleep(1000000);
                if (process->isRunning()) {
                    std::cout << "All instances configured for the program ";
                    std::cout << GREEN << processName << RESET;
                    std::cout << " have started successfully." << std::endl;
                } else {
                    // TODO: Add logic for number of restarts from config
                    std::cout << "One or more instances configured for the program ";
                    std::cout << RED << processName << RESET;
                    std::cout << " failed to start correctly. The program will be stopped. Please check the logs for details." << std::endl;
                    process->stop();
                }
            } catch (const std::exception& ex) {
                std::cerr << "Error starting program " << processName << ": " << ex.what() << std::endl;
                // todo display message for user
                process.stop();
                exit(1); 
            }
    }
}


void TaskMaster::stopProcess(const std::string& processName) {
    Process* process = findProcess(processName);
    if (process != nullptr) {
        process->stop();
    }
}


void TaskMaster::stopAllProcesses() {
    for (auto& [name, process] : processes) {
        process.stop();
    }
}

void TaskMaster::displayStatus() {
    for (const auto& [name, process] : processes) {
        if (process.isRunning()){
            std::cout << GREEN << process.getName() << RESET;
            std::cout << ": " << process.getStatus() << std::endl;
        } else {
            std::cout << RED << process.getName() << RESET;
            std::cout << ": " << process.getStatus() << std::endl;
        }
    }
}

// ___________________ USAGE ___________________

void TaskMaster::displayUsage() {
    std::cout << "Usage:" << std::endl;
    std::cout << std::endl;
    std::cout << "Commands already implemented:" << std::endl;
    std::cout << YELLOW << "start <program_name>: " << RESET; 
    std::cout << "Start a program by name. (For programs with start_time = 0, not started at taskmaster launch)" << std::endl;
    std::cout << YELLOW <<  "stop <program_name>:" << RESET;
    std::cout << "Stop a running program by name." << std::endl;
    std::cout << YELLOW <<  "status: " << RESET;
    std::cout << "Show the status of all programs." << std::endl;
    std::cout << YELLOW <<  "exit: " << RESET;
    std::cout << "Exit the taskmaster." << std::endl;
    std::cout << std::endl;
    std::cout << "Commands to be implemented:" << std::endl;
    std::cout << YELLOW << "restart <program_name>: "<< RESET;
    std::cout << "Restart a program by name." << std::endl;
    std::cout << YELLOW << "reload <program_name>: " << RESET;
    std::cout << "Reload the configuration of a program without stopping it." << std::endl;
    std::cout << std::endl;
}
