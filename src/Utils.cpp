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
        Logger::getInstance().log("\nSIGHUP signal received. Reloading configuration...");
        TaskMaster::reloadConfig();
    } else {
        Logger::getInstance().log("\nsignal " + std::to_string(sig) + " received. Stopping all processes...");
        TaskMaster::stopAllProcesses();
        Logger::getInstance().log("TaskMaster shutting down...");
        exit(sig);
    }
}