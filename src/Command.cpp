#include "Command.hpp"

Command stringToCommand(const std::string& str) {
    if (str == "status") {
        return Command::Status;
    } else if (str == "start") {
        return Command::Start;
    } else if (str == "stop") {
        return Command::Stop;
    } else {
        return Command::Unknown;
    }
}