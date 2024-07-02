#include "TaskMaster.hpp"
#include <iostream>
#include <string>

int main(const int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    const std::string configFilePath = "./config/top_config.json";

    try {
        TaskMaster taskmaster(configFilePath);
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
