#include "TaskMaster.hpp"
#include <iostream>
#include <string>



int main(const int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config_file_path>" << std::endl;
        return EXIT_FAILURE;
    }

    // Needs to be started as root
    if (geteuid() != 0) {
        std::cerr << "This program must be run as root" << std::endl;
        return EXIT_FAILURE;
    }

    try {
        TaskMaster taskmaster(argv[1]);
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}