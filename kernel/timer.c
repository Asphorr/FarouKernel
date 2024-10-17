#include <chrono>
#include <iostream>
#include <string>

using namespace std::literals;

class Timer {
public:
    explicit Timer(const std::string &name): name_(name) {}
    
    void start() {
        startTime_ = std::chrono::steady_clock::now();
    }
    
    void stop() {
        endTime_ = std::chrono::steady_clock::now();
        
        auto duration = endTime_ - startTime_;
        std::cout << "Timer '" << name_ << "' took " << duration.count() << " seconds." << std::endl;
    }
    
private:
    std::string name_;
    std::chrono::steady_clock::time_point startTime_, endTime_;
};

int main() {
    Timer t("My Timer");
    t.start();
    // Do something here...
    t.stop();
    return 0;
}
