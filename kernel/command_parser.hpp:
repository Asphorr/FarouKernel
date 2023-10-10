#pragma once

#include <string>
#include <regex>

class CommandParser {
public:
    struct ParsedCommand {
        std::string name;
        std::vector<std::string> args;
    };

    explicit CommandParser(const std::regex& pattern) : _pattern{pattern} {}

    ParsedCommand parse(const std::string& command) {
        std::smatch match;
        if (std::regex_match(command, match, _pattern)) {
            return {match[1].str(), splitArgs(match[2])};
        } else {
            throw std::invalid_argument{"Invalid command syntax."};
        }
    }

private:
    static std::vector<std::string> splitArgs(const std::string& argString) {
        std::istringstream iss{argString};
        std::vector<std::string> args;
        std::copy(std::istream_iterator<std::string>(iss),
                 std::istream_iterator<std::string>(),
                 std::back_inserter(args));
        return args;
    }

    std::regex _pattern;
};
