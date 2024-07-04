#include "TaskMaster.hpp"
#include "Utils.hpp"

std::map<std::string, Process> TaskMaster::processes;

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
        // std::cerr << "Error starting program " << name << ": " << ex.what() << std::endl;
        // process.stop();
        stopAllProcesses();
        exit(1); 
    }
}

void TaskMaster::startInitialProcesses() {
    for (auto& [name, process] : processes) {
        if (process.getStartTime() == 1) {
            try {
                startProcess(name);
                // process.start();
                // std::cout << "Started program " << name << std::endl;
                // usleep(1000000);
                // // Wait for the process to be verified as healthy
                // if (process.isRunning()) {
                //     std::cout << "Program " << name << " is healthy." << std::endl;
                // } else {
                //     std::cout << "Program " << name << " failed to start correctly." << std::endl;
                //     process.stop();
                // }
            } catch (const std::exception& ex) {
                std::cerr << "Error starting program " << name << ": " << ex.what() << std::endl;
                // process.stop();
                stopAllProcesses();
                exit(1); 
            }
        }
    }
}

void TaskMaster::commandLoop() {
    std::string command;
    while (true) {
        std::cout << "taskmaster> ";
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

void TaskMaster::stopAllProcesses() {
    for (auto& [name, process] : processes) {
        process.stop();
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

void TaskMaster::startProcess(const std::string& processName) {
    Process* process = findProcess(processName);
    if (process != nullptr) {
         try {
                process->start();
                // std::cout << "Started program " << processName << std::endl;
                usleep(1000000);
                // Wait for the process to be verified as healthy
                if (process->isRunning()) {
                    std::cout << std::endl;
                    std::cout << "All instances configured for the program ";
                    std::cout << GREEN << processName << RESET;
                    std::cout << " have started successfully." << std::endl;
                    std::cout << std::endl;
                } else {
                    // TODO: Add logic for number of restarts from config
                    std::cout << std::endl;
                    std::cout << "One or more instances configured for the program ";
                    std::cout << GREEN << processName << RESET;
                    std::cout << " failed to start correctly. The program will be stopped. Please check the logs for details." << std::endl;
                    std::cout << std::endl;
                    process->stop();
                }
            } catch (const std::exception& ex) {
                std::cerr << "Error starting program " << processName << ": " << ex.what() << std::endl;
                // todo display message for user
                stopAllProcesses();
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

void TaskMaster::displayStatus() {
    for (const auto& [name, process] : processes) {
        std::cout << process.getName() << ": " << process.getStatus() << std::endl;
    }
}

void TaskMaster::displayUsage() {
    std::cout << "Usage:" << std::endl;
    std::cout << std::endl;
    std::cout << "Commands already implemented:" << std::endl;
    std::cout << "start <program_name>: Start a program by name. (For programs with start_time = 0, not started at taskmaster launch)" << std::endl;
    std::cout << "stop <program_name>: Stop a running program by name." << std::endl;
    std::cout << "status: Show the status of all programs." << std::endl;
    std::cout << "exit: Exit the taskmaster." << std::endl;
    std::cout << std::endl;
    std::cout << "Commands to be implemented:" << std::endl;
    std::cout << "restart <program_name>: Restart a program by name." << std::endl;
    std::cout << "reload <program_name>: Reload the configuration of a program without stopping it." << std::endl;
    std::cout << std::endl;
}
