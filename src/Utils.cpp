#include "Utils.hpp"

std::vector<std::string> Utils::split(const std::string &s, const char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}


void Utils::signalHandler(int sig) {
    if (sig == SIGHUP) {
        TaskMaster::reloadConfigTriggered = true;
        std::cout << "SIGHUP signal received" << std::endl;
    } else {
        TaskMaster::stopTaskmasterTriggered = true;
        std::cout << "SIGINT signal received" << std::endl;
    }
}
