#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// Forward declarations
struct ParsedCommand;
class CommandParser;
class CommandExecutor;
class ErrorHandler;

using namespace std::literals;

int main() {
    // Create objects
    auto parser = std::make_unique<CommandParser>();
    auto executor = std::make_unique<CommandExecutor>();
    auto handler = std::make_unique<ErrorHandler>();

    // Configure console
    Console console(std::move(parser), std::move(executor), std::move(handler));

    // Start console loop
    console.run();

    return 0;
}

// Class definitions
class Console {
public:
    // Constructor
    Console(std::unique_ptr<CommandParser> parser, std::unique_ptr<CommandExecutor> executor, std::unique_ptr<ErrorHandler> handler)
      : _parser{std::move(parser)}, _executor{std::move(executor)}, _handler{std::move(handler)} {}

    // Copy constructor
    Console(const Console& other) = delete;

    // Move constructor
    Console(Console&& other) noexcept = default;

    // Copy assignment operator
    Console& operator=(const Console& other) = delete;

    // Move assignment operator
    Console& operator=(Console&& other) noexcept = default;

    // Destructor
    ~Console() override = default;

    // Run method
    void run() override {
        while (!isEof()) {
            try {
                auto command = readLine();
                executeCommand(command);
            } catch (const std::exception& e) {
                handleError(e);
            }
        }
    }

private:
    // Private constructor
    Console(std::unique_ptr<CommandParser> parser, std::unique_ptr<CommandExecutor> executor, std::unique_ptr<ErrorHandler> handler)
      : _parser{std::move(parser)}, _executor{std::move(executor)}, _handler{std::move(handler)} {}

    // Check if EOF has been reached
    [[nodiscard]] bool isEof() const {
        return !std::cin.good();
    }

    // Read a line from standard input
    [[noreturn]] std::string readLine() {
        std::string line;
        getline(std::cin, line);
        return line;
    }

    // Parse and execute a command
    void executeCommand(const std::string& command) {
        auto parsedCommand = _parser->parse(command);
        _executor->execute(parsedCommand);
    }

    // Handle an exception thrown during execution
    void handleError(const std::exception& e) {
        _handler->handle(e);
    }

    // Member variables
    std::unique_ptr<CommandParser> _parser;
    std::unique_ptr<CommandExecutor> _executor;
    std::unique_ptr<ErrorHandler> _handler;
};

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

class CommandExecutor {
public:
    virtual void execute(const CommandParser::ParsedCommand& command) = 0;
};

class ErrorHandler {
public:
    virtual void handle(const std::exception& error) = 0;
};
