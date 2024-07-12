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
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        // Write the input to the pipe
        if (write(pipefd[1], "exit", 4) == -1) {
            perror("write");
            exit(EXIT_FAILURE);
        }
        close(pipefd[1]);

        // Redirect stdin to read from the pipe
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
    }
}
