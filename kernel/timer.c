#include <chrono>
#include <iostream>
#include <string>
#include <iomanip> // For std::fixed and std::setprecision
#include <thread> // For std::this_thread::sleep_for

using namespace std::literals;

class ScopedTimer {
public:
    explicit ScopedTimer(const std::string& name)
        : name_(name), start_time_(std::chrono::steady_clock::now()), stopped_(false) {
        std::cout << "Timer '" << name_ << "' started." << std::endl;
    }

    ~ScopedTimer() {
        if (!stopped_) {
            stop();
        }
    }

    void stop() {
        if (!stopped_) {
            auto end_time = std::chrono::steady_clock::now();
            auto duration = end_time - start_time_;
            stopped_ = true;

            auto duration_ms = std::chrono::duration<double, std::milli>(duration).count();

            std::cout << "Timer '" << name_ << "' stopped. Duration: "
                      << std::fixed << std::setprecision(3) << duration_ms << " ms." << std::endl;
        }
    }

    // Prevent copying and assignment
    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer& operator=(const ScopedTimer&) = delete;

private:
    std::string name_;
    std::chrono::steady_clock::time_point start_time_;
    bool stopped_;
};

class ManualTimer {
public:
    explicit ManualTimer() : running_(false) {}

    void start() {
        if(!running_){
            start_time_ = std::chrono::steady_clock::now();
            running_ = true;
        } else {
            std::cerr << "Warning: Timer is already running." << std::endl;
        }
    }

    void stop() {
        if (running_) {
            end_time_ = std::chrono::steady_clock::now();
            running_ = false;
        } else {
            std::cerr << "Warning: Timer was not running when stop() was called." << std::endl;
        }
    }

    template <typename DurationType = std::chrono::milliseconds>
    typename DurationType::rep elapsed() const {
        if (running_) {
            return std::chrono::duration_cast<DurationType>(std::chrono::steady_clock::now() - start_time_).count();
        } else if (start_time_ != std::chrono::steady_clock::time_point{} && end_time_ != std::chrono::steady_clock::time_point{}) {
            return std::chrono::duration_cast<DurationType>(end_time_ - start_time_).count();
        }
        return 0;
    }

    double elapsed_seconds() const {
        return elapsed<std::chrono::duration<double>>();
    }

    double elapsed_milliseconds() const {
        return elapsed<std::chrono::duration<double, std::milli>>();
    }

    double elapsed_microseconds() const {
        return elapsed<std::chrono::duration<double, std::micro>>();
    }

    double elapsed_nanoseconds() const {
        return elapsed<std::chrono::duration<double, std::nano>>();
    }

    bool is_running() const {
        return running_;
    }

private:
    std::chrono::steady_clock::time_point start_time_{}, end_time_{};
    bool running_;
};

int main() {
    std::cout << "Scoped Timer Example:\n";
    {
        ScopedTimer scoped("File Processing");
        std::cout << "Simulating file processing...\n";
        std::this_thread::sleep_for(210ms);
    } // Timer stops and prints here

    std::cout << "\nManual Timer Example:\n";
    ManualTimer manual;
    manual.start();
    std::cout << "Manual timer started. Simulating task 1...\n";
    std::this_thread::sleep_for(80ms);
    std::cout << "Elapsed so far: " << std::fixed << std::setprecision(1) << manual.elapsed_milliseconds() << " ms\n";
    std::cout << "Simulating task 2...\n";
    std::this_thread::sleep_for(130ms);
    manual.stop();
    std::cout << "Manual timer stopped.\n";

    std::cout << "Total time measured by manual timer: "
              << std::fixed << std::setprecision(3)
              << manual.elapsed_milliseconds() << " ms ("
              << manual.elapsed_microseconds() << " us, "
              << manual.elapsed_seconds() << " s)" << std::endl;

    manual.start();
    std::cout << "\nRestarting manual timer for another operation...\n";
    std::this_thread::sleep_for(55ms);
    manual.stop();
    std::cout << "Second operation time: " << manual.elapsed_nanoseconds() << " ns" << std::endl;

    return 0;
}
