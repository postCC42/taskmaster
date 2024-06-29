#include "TaskMaster.hpp"
#include <iostream>
#include <string>

int main(const int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    const std::string configFilePath = "./config/basic_config.json";

    try {
        TaskMaster taskmaster(configFilePath);
        taskmaster.initializeProcesses();
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
