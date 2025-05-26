#include <concepts>
#include <functional>
#include <map>
#include <stdexcept>
#include <utility>

template <typename T>
concept Executor = requires(T executor) {
    // Check that the executor has a member function named 'execute' that takes a single argument of type 'CommandParser::ParsedCommand'.
    executor.execute({});
};

template <typename T>
concept Parser = requires(T parser) {
    // Check that the parser has a member function named 'parse' that returns a value of type 'CommandParser::ParsedCommand'.
    parser.parse("");
};

struct CommandExecutor {
    template <typename FuncType>
    void registerCommand(const std::string& name, FuncType&& func) {
        commands_.emplace(name, std::forward<FuncType>(func));
    }

    void execute(const CommandParser::ParsedCommand& command) {
        try {
            auto it = commands_.find(command.name());
            if (it == commands_.end()) {
                throw std::out_of_range("Unknown command.");
            }
            it->second(command.arguments());
        } catch (const std::exception& e) {
            std::cerr << "Error executing command '" << command.name() << "': " << e.what() << '\n';
        }
    }

private:
    std::map<std::string, std::function<void(const std::vector<std::string>&)>> commands_;
};

struct CommandParser {
    struct ParsedCommand {
        std::string name;
        std::vector<std::string> arguments;
    };

    ParsedCommand parse(const std::string& input) {
        // Parse the input string and extract the command name and arguments.
        // ...
        return {};
    }
};

int main() {
    CommandExecutor executor;
    CommandParser parser;

    // Register some commands with the executor.
    executor.registerCommand("hello", [](){ std::cout << "Hello, world!\n"; });
    executor.registerCommand("add", [](auto x, auto y){ std::cout << x + y << '\n'; });

    // Read user input and pass it to the parser.
    while (true) {
        std::string input;
        std::getline(std::cin, input);
        if (!input.empty()) {
            break;
        }
        try {
            auto parsedCommand = parser.parse(input);
            executor.execute(parsedCommand);
        } catch (const std::exception& e) {
            std::cerr << "Invalid command.\n";
        }
    }

    return 0;
}
