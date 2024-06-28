#include "ConfigParser.hpp"
#include "ProcessControl.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

void displayUsage();
void initializeProcesses(const std::string& configFilePath, std::vector<ProcessControl>& processes);
void startInitialProcesses(std::vector<ProcessControl>& processes);
void commandLoop(std::vector<ProcessControl>& processes);
void handleCommand(const std::string& command, std::vector<ProcessControl>& processes);
void startProcess(const std::string& processName, std::vector<ProcessControl>& processes);
void showStatus(const std::vector<ProcessControl>& processes);

int main(const int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    std::string configFilePath = "./config/basic_config.json";

    try {
        std::vector<ProcessControl> processes;
        initializeProcesses(configFilePath, processes);
        startInitialProcesses(processes);
        displayUsage();
        commandLoop(processes);
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void displayUsage() {
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

void initializeProcesses(const std::string& configFilePath, std::vector<ProcessControl>& processes) {
    ConfigParser parser(configFilePath);
    json config = parser.getConfig();

    for (const auto& item : config.at("programs").items()) {
        processes.emplace_back(item.key(), item.value());
    }
}

void startInitialProcesses(std::vector<ProcessControl>& processes) {
    for (auto& process : processes) {
        if (process.getStartTime() == 1) {
            process.start();
        }
    }
}

void commandLoop(std::vector<ProcessControl>& processes) {
    std::string command;
    while (true) {
        std::cout << "taskmaster> ";
        std::getline(std::cin, command);

        if (command == "exit") break;

        handleCommand(command, processes);
    }
}

void handleCommand(const std::string& command, std::vector<ProcessControl>& processes) {
    if (command == "status") {
        showStatus(processes);
    } else if (command.rfind("start", 0) == 0) {
        if (command.length() > 6) {
            std::string processName = command.substr(6);
            startProcess(processName, processes);
        } else {
            std::cout << "Invalid command format. Usage: start <process_name>" << std::endl;
        }
    } else {
        std::cout << "Unknown command" << std::endl;
    }
}

void startProcess(const std::string& processName, std::vector<ProcessControl>& processes) {
    auto it = std::find_if(processes.begin(), processes.end(),
        [&processName](ProcessControl& p) { return p.getName() == processName; });
    if (it != processes.end()) {
        it->start();
    } else {
        std::cout << "Process not found" << std::endl;
    }
}

void showStatus(const std::vector<ProcessControl>& processes) {
    for (const auto& process : processes) {
        std::cout << process.getName() << ": " << process.getStatus() << std::endl;
    }
}
