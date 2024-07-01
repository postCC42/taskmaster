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
    std::cout << "stop <program_name>: Stop a running program by name." << std::endl;
    std::cout << "status: Show the status of all programs." << std::endl;
    std::cout << "exit: Exit the taskmaster." << std::endl;
    std::cout << std::endl;
    std::cout << "Commands to be implemented:" << std::endl;
    std::cout << "restart <program_name>: Restart a program by name." << std::endl;
    std::cout << "reload <program_name>: Reload the configuration of a program without stopping it." << std::endl;
    std::cout << std::endl;
}
