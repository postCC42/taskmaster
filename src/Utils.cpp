#include "Utils.hpp"
#include "TaskMaster.hpp"

std::vector<std::string> Utils::split(const std::string &s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}


void Utils::sigintHandler(int sig) {
    std::cout << "\nSIGINT received. Stopping all processes...\n";
    TaskMaster::stopAllProcesses();
    exit(sig); 
}