#include "../include/TaskMaster.hpp"

TaskMaster::TaskMaster(const std::string& configFilePath) : configFilePath(configFilePath), configParser(configFilePath) {
    std::cout << "TaskMaster created with config file path: " << configFilePath << std::endl;
    displayUsage();
    commandLoop();
}

void TaskMaster::initializeProcesses() {
    json config = configParser.getConfig();

    for (const auto& item : config.at("programs").items()) {
        processes.emplace_back(item.key(), item.value());
    }

    startInitialProcesses();
}
void TaskMaster::startInitialProcesses() {
    for (auto& process : processes) {
        if (process.getStartTime() == 1) {
            process.start();
        }
    }
}

void TaskMaster::commandLoop() {
    std::string command;
    while (true) {
        std::cout << "taskmaster> ";
        std::getline(std::cin, command);

        if (command == "exit") break;
        handleCommand(command);
    }
}

void TaskMaster::handleCommand(const std::string &command) {
    if (command == "status") {
        displayStatus();
    } else if (command.rfind("start", -1) == 0) {
        if (command.length() > 5) {
            std::string processName = command.substr(5);
            startProcess(processName);
        } else {
            std::cout << "Invalid command format. Usage: start <process_name>" << std::endl;
        }
    } else {
        std::cout << "Unknown command: " << command << std::endl;
    }
}

void TaskMaster::startProcess(const std::string& processName) {
    auto it = std::find_if(processes.begin(), processes.end(),
        [&processName](ProcessControl& p) { return p.getName() == processName; });
    if (it != processes.end()) {
        it->start();
    } else {
        std::cout << "Process not found" << std::endl;
    }
}

void TaskMaster::displayStatus() {
    for (const auto& process : processes) {
        std::cout << process.getName() << ": " << process.getStatus() << std::endl;
    }
}

void TaskMaster::displayUsage() {
    std::cout << "Usage:" << std::endl;
    std::cout << std::endl;
    std::cout << "Commands already implemented:" << std::endl;
    std::cout << "start <program_name>: Start a program by name. (For programs with start_time = 0, not started at taskmaster launch)" << std::endl;
    std::cout << "status: Show the status of all programs." << std::endl;
    std::cout << "exit: Exit the taskmaster." << std::endl;
    std::cout << std::endl;
    std::cout << "Commands to be implemented:" << std::endl;
    std::cout << "stop <program_name>: Stop a running program by name." << std::endl;
    std::cout << "restart <program_name>: Restart a program by name." << std::endl;
    std::cout << "reload <program_name>: Reload the configuration of a program without stopping it." << std::endl;
    std::cout << std::endl;
}
