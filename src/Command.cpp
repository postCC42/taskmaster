#include "Command.hpp"

Command stringToCommand(const std::string& str) {
    if (str == "status") {
        return Command::Status;
    }
    if (str == "start") {
        return Command::Start;
    }
    if (str == "stop") {
        return Command::Stop;
    }
    return Command::Unknown;
}
