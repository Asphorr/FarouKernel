#include "console_interface.hpp"
#include "command_parser.hpp"
#include "command_executor.hpp"
#include "error_handler.hpp"

// Define the command line interface
class Console : public ConsoleInterface {
public:
    explicit Console(CommandParser parser, CommandExecutor executor, ErrorHandler handler)
      : _parser{parser}, _executor{executor}, _handler{handler} {}

    virtual ~Console() override = default;

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
    CommandParser _parser;
    CommandExecutor _executor;
    ErrorHandler _handler;

    bool isEof() { return !std::cin.good(); }

    std::string readLine() {
        std::string line;
        getline(std::cin, line);
        return line;
    }

    void executeCommand(const std::string& command) {
        auto parsedCommand = _parser.parse(command);
        _executor.execute(parsedCommand);
    }

    void handleError(const std::exception& e) {
        _handler.handle(e);
    }
};
