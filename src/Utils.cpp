#include "Utils.hpp"
#include "TaskMaster.hpp"

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
    std::cout << "\nsignal " << sig << " received. Stopping all processes...\n";
    TaskMaster::stopAllProcesses();
    exit(sig);
}