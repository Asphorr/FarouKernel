#pragma once

#include <map>
#include <functional>

class CommandExecutor {
public:
    template <typename FuncType>
    void registerCommand(const std::string& name, FuncType func) {
        _commands[name] = std::bind(func, this, std::placeholders::_1);
    }

    void execute(const CommandParser::ParsedCommand& command) {
        auto it = _commands.find(command.name);
        if (it != _commands.end()) {
            it->second(command.args);
        } else {
            throw std::runtime_error{"Unknown command."};
        }
    }

private:
    std::map<std::string, std::function<void(const std::vector<std::string>&)>> _commands;
};
