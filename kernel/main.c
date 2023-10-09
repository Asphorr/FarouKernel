#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <memory>
#include <cassert>

using namespace std::literals;

class Kernel {
public:
    explicit Kernel(const std::string& name, int major, int minor, const std::string& releaseDate, const std::string& buildTime, const std::string& author, const std::string& copyright, const std::string& license, const std::string& description) :
        m_name{name}, m_major{major}, m_minor{minor}, m_releaseDate{releaseDate}, m_buildTime{buildTime}, m_author{author}, m_copyright{copyright}, m_license{license}, m_description{description} {}

    ~Kernel() noexcept {
        stop();
    }

    void start() {
        assert(!m_running && !m_stopped);
        m_running = true;
        m_stopRequested = false;
        m_mainThread = std::make_unique<std::thread>(&Kernel::run, this);
    }

    void stop() {
        assert(m_running || m_stopped);
        m_stopRequested = true;
        m_cv.notify_all();
        m_mainThread->join();
        m_mainThread.reset();
        m_running = false;
        m_stopped = true;
    }

private:
    void run() {
        while (!m_stopRequested) {
            auto tp = std::chrono::system_clock::now().time_since_epoch();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp).count();
            std::cout << "[" << ms << "ms] Hello from kernel!" << std::endl;
            std::this_thread::sleep_for(500ms);
        }
    }

    bool m_running = false;
    bool m_stopped = false;
    bool m_stopRequested = false;
    std::unique_ptr<std::thread> m_mainThread;
};

int main() {
    Kernel kernel{"My First Kernel", 1, 0, "2023-09-28", "14:30:00", "Mikhail", "Copyright (C) 2023 Mikhail", "MIT License", "A simple kernel for learning purposes"};
    kernel.start();
    std::cin.get();
    kernel.stop();
    return 0;
}
