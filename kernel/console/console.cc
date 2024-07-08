#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>

class Console final : public ConsoleInterface {
public:
    // Delegate construction to private constructor
    template <typename ParserT, typename ExecutorT, typename HandlerT>
    static std::unique_ptr<Console> Create(ParserT&& parser, ExecutorT&& executor, HandlerT&& handler) {
        return std::make_unique<Console>(std::forward<ParserT>(parser), std::forward<ExecutorT>(executor), std::forward<HandlerT>(handler));
    }

    // Move constructor
    Console(Console&& other) noexcept = default;

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
    Console(CommandParser parser, CommandExecutor executor, ErrorHandler handler)
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
    std::shared_ptr<CommandParser> _parser;
    std::shared_ptr<CommandExecutor> _executor;
    std::shared_ptr<ErrorHandler> _handler;
};
