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
    if (str == "restart") {
        return Command::Restart;
    }
    if (str == "reload") {
        return Command::Reload;
    }
    return Command::Unknown;
}
