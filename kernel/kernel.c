#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;
using namespace std::literals;

class KernelModule : public fs::module {
public:
    explicit KernelModule(std::string name) : _name(std::move(name)), _files(), _threads() {}
    
    ~KernelModule() override {
        stopAllThreads();
    }
    
    bool initialize() override {
        try {
            _files.emplace_back("kernel", fs::mode::read | fs::mode::write);
            _files.emplace_back("modules", fs::mode::read | fs::mode::write);
            
            auto& kernelFile = _files.front().get();
            kernelFile.setPosition(0);
            kernelFile.write("Hello, world!"sv);
            
            auto& modulesFile = _files.at(1).get();
            modulesFile.setPosition(0);
            modulesFile.write("This is a test."sv);
            
            return true;
        } catch (...) {
            return false;
        }
    }
    
    void terminate() override {
        stopAllThreads();
        
        _files.clear();
    }
    
    void run() override {
        while (_running) {
            switchToNextThread();
        }
    }
    
    void addThread(std::unique_ptr<fs::thread>&& thread) {
        _threads.push_back(std::move(thread));
    }
    
    void removeThread(fs::thread* thread) {
        _threads.erase(std::remove(_threads.begin(), _threads.end(), thread), _threads.end());
    }
    
    void stopAllThreads() {
        for (auto& thread : _threads) {
            thread->stop();
        }
    }
    
    void switchToNextThread() {
        if (_currentThreadIndex >= _threads.size()) {
            _currentThreadIndex = 0;
        }
        
        auto nextThread = _threads[_currentThreadIndex].get();
        ++_currentThreadIndex;
        
        nextThread->run();
    }
    
private:
    std::string _name;
    std::vector<std::unique_ptr<fs::file>> _files;
    std::vector<std::unique_ptr<fs::thread>> _threads;
    size_t _currentThreadIndex = 0;
};

int main() {
    KernelModule kernelModule("My Kernel Module");
    kernelModule.initialize();
    kernelModule.addThread(std::make_unique<fs::thread>([](){
        std::cout << "Hello, world!";
    }));
    kernelModule.run();
    kernelModule.terminate();
    return 0;
}
