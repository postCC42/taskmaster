#include "ConfigParser.hpp"

int main(const int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    std::string configFilePath = "./config/basic_config.json";

    try {
        ConfigParser parser(configFilePath);
        json config = parser.getConfig();

        // std::cout << "===== Parsed Configuration Recap =====" << std::endl;

        // for (auto it = config.begin(); it != config.end(); ++it) {
        //     std::cout << "- " << it.key() << ": " << it.value() << std::endl;
        // }

        // std::cout << "======================================" << std::endl;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
