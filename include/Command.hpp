#ifndef COMMAND_H
# define COMMAND_H

#include <string>

enum class Command {
    Status,
    Start,
    Stop,
    Unknown
};

Command stringToCommand(const std::string& str);
#endif