#include "../include/TaskMaster.hpp"

TaskMaster::TaskMaster(const std::string& configFilePath) : configFilePath(configFilePath), configParser(configFilePath) {
    std::cout << "TaskMaster created with config file path: " << configFilePath << std::endl;
    displayUsage();
    initializeProcesses();
    commandLoop();
}

void TaskMaster::initializeProcesses() {
    json config = configParser.getConfig();
    std::cout << "initializeProcesses" << std::endl;

    for (const auto& item : config.at("programs").items()) {
        processes.emplace(item.key(), ProcessControl(item.key(), item.value()));
        std::cout << "Process " << item.key() << " initialized" << std::endl;
    }

    startInitialProcesses();
}

void TaskMaster::startInitialProcesses() {
    for (auto&[name, process] : processes) {
        // TODO: should be autostart
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
        if (std::cin.eof()) {
            break;
        }

        if (command == "exit") break;
        handleCommand(command);
    }
}

void TaskMaster::handleCommand(const std::string &command) {
    std::cout << "Handling command: " << command << std::endl;
    // TODO: create a CLI class for command handling
    if (command == "status") {
        displayStatus();
    } else if (command.rfind("start", -1) == 0) {
        if (command.length() > 6) {
            // TODO: should be a split on space character
            const std::string processName = command.substr(6);
            startProcess(processName);
        } else {
            std::cout << "Invalid command format. Usage: start <process_name>" << std::endl;
        }
    } else if (command.rfind("stop", -1) == 0) {
        if (command.length() > 5) {
            // TODO: should be a split on space character
            const std::string processName = command.substr(5);
            stopProcess(processName);
        } else {
            std::cout << "Invalid command format. Usage: stop <process_name>" << std::endl;
        }
    } else {
        std::cout << "Unknown command: " << command << std::endl;
    }
}

ProcessControl* TaskMaster::findProcess(const std::string& processName) {
    auto it = processes.find(processName);
    if (it != processes.end()) {
        return &it->second;
    } else {
        std::cout << "Process not found: " << processName << std::endl;
        return nullptr;
    }
}

void TaskMaster::startProcess(const std::string& processName) {
    ProcessControl* process = findProcess(processName);
    if (process != nullptr) {
        process->start();
    }
}

void TaskMaster::stopProcess(const std::string& processName) {
    ProcessControl* process = findProcess(processName);
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
    std::cout << "status: Show the status of all programs." << std::endl;
    std::cout << "exit: Exit the taskmaster." << std::endl;
    std::cout << std::endl;
    std::cout << "Commands to be implemented:" << std::endl;
    std::cout << "stop <program_name>: Stop a running program by name." << std::endl;
    std::cout << "restart <program_name>: Restart a program by name." << std::endl;
    std::cout << "reload <program_name>: Reload the configuration of a program without stopping it." << std::endl;
    std::cout << std::endl;
}
