#include "ConfigParser.hpp"
#include "ProcessControl.hpp"

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

int main(const int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    std::string configFilePath = "./config/basic_config.json";

    try {
        ConfigParser parser(configFilePath);
        json config = parser.getConfig();

        std::vector<ProcessControl> processes;
        for (const auto& item : config.at("programs").items()) {
                processes.emplace_back(item.key(), item.value());
            }

        for (auto& process : processes) {
            if (process.getStartTime() == 1) {
                process.start();
            }
        }

        displayUsage();


        // Control shell to manage processes
        std::string command;
        while (true) {
            std::cout << "taskmaster> ";
            std::getline(std::cin, command);

            if (command == "exit") break;

            if (command == "status") {
                for (auto& process : processes) {
                    std::cout << process.getName() << ": " << process.getStatus() << std::endl;
                }
            } else if (command.rfind("start", 0) == 0) {
                if (command.length() > 6) {
                    std::string processName = command.substr(6);
                    auto it = std::find_if(processes.begin(), processes.end(),
                        [&processName](ProcessControl& p) { return p.getName() == processName; });
                    if (it != processes.end()) {
                        it->start();
                    } else {
                        std::cout << "Process not found" << std::endl;
                    }
                } else {
                    std::cout << "Invalid command format. Usage: start <process_name>" << std::endl;
                }
            } else {
                std::cout << "Unknown command" << std::endl;
            }
        }
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
