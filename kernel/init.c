#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <functional>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <chrono>
#include <exception>

class ProgramRunner {
public:
    explicit ProgramRunner(const std::vector<std::string>& arguments): _arguments{arguments}, _running{false} {}
    
    ~ProgramRunner() {
        stop();
    }
    
    bool run() {
        try {
            auto future = std::async(std::launch::async, [this]{
                _running = true;
                
                // Set up the program environment
                setupEnvironment();
                
                // Start the program
                startProgram(_arguments);
                
                // Wait for the program to finish
                waitForProgramToFinish();
                
                // Clean up and exit
                cleanUpAndExit();
            });
            
            return future.valid();
        } catch (...) {
            return false;
        }
    }
    
    void stop() {
        if (_running) {
            _running = false;
            _cv.notify_all();
        }
    }
private:
    const std::vector<std::string> _arguments;
    volatile bool _running;
    std::mutex _mtx;
    std::condition_variable _cv;
};

int main() {
    std::cout << "Enter command line arguments separated by spaces:" << std::endl;
    std::vector<std::string> arguments;
    std::copy(std::istream_iterator<std::string>(std::cin), std::istream_iterator<std::string>(), std::back_inserter(arguments));
    
    ProgramRunner runner(arguments);
    if (!runner.run()) {
        std::cerr << "Failed to start program" << std::endl;
        return EXIT_FAILURE;
    }
    
    std::cout << "Press enter to stop the program..." << std::endl;
    getchar();
    
    runner.stop();
    return EXIT_SUCCESS;
}
