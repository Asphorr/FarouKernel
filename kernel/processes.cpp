#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <csignal>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <condition_variable>

// Process Information Structure
struct ProcessInfo {
    pid_t pid;
    int priority;
    std::thread workerThread;
    bool isThread;
};

// Process Manager Class
class ProcessManager {
public:
    // Singleton pattern for global access
    static ProcessManager& instance() {
        static ProcessManager instance;
        return instance;
    }

    // Add a new process
    pid_t addProcess(std::function<void()> func, int priority = 0) {
        std::lock_guard<std::mutex> lock(m_processMutex);  // Lock the mutex before forking
        pid_t pid = fork();
        if (pid == -1) {
            handleSyscallError("Failed to create process");
        } else if (pid == 0) {
            // Child process
            func();
            exit(EXIT_SUCCESS);
        }

        // Parent process: add process info to the list
        m_processInfos.push_back({pid, priority, std::thread(), false});
        return pid;
    }

    // Add a new multithreaded process
    void addThreadProcess(std::function<void()> func, int priority = 0) {
        std::thread t([this, func, priority] {
            func();
            onThreadFinish();  // Notify when the thread finishes
        });
        std::lock_guard<std::mutex> lock(m_processMutex);
        m_processInfos.push_back({0, priority, std::move(t), true});
    }

    // Wait for all processes and threads to finish
    void waitForAll() {
        std::unique_lock<std::mutex> lock(m_processMutex);
        m_cv.wait(lock, [this] { return std::all_of(m_processInfos.begin(), m_processInfos.end(), [this](const ProcessInfo& p) {
            if (p.isThread) {
                if (p.workerThread.joinable()) {
                    p.workerThread.join();
                }
                return true;
            } else {
                return checkProcessStatus(p.pid) != -1;
            }
        }); });
    }

    // Terminate all processes
    void terminateAll() {
        std::lock_guard<std::mutex> lock(m_processMutex);
        for (auto& process : m_processInfos) {
            if (!process.isThread) {
                terminateProcess(process.pid);
            } else if (process.workerThread.joinable()) {
                process.workerThread.join();
            }
        }
        m_processInfos.clear();
    }

    // Set signal handler
    void setSignalHandler(int signal, void (*handler)(int)) {
        struct sigaction sa;
        sa.sa_handler = handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        if (sigaction(signal, &sa, nullptr) == -1) {
            handleSyscallError("Failed to set signal handler");
        }
    }

    // Create a process group
    void createProcessGroup(pid_t pid) {
        if (setpgid(pid, pid) == -1) {
            handleSyscallError("Failed to create process group");
        }
    }

    // Join an existing process group
    void joinProcessGroup(pid_t pid) {
        if (setpgid(getpid(), pid) == -1) {
            handleSyscallError("Failed to join process group");
        }
    }

private:
    // Constructor
    ProcessManager() = default;

    // Destructor
    ~ProcessManager() {
        terminateAll();
    }

    // Disable copy/move constructors and assignment
    ProcessManager(const ProcessManager&) = delete;
    ProcessManager& operator=(const ProcessManager&) = delete;
    ProcessManager(ProcessManager&&) = delete;
    ProcessManager& operator=(ProcessManager&&) = delete;

    // Utility function to handle errors in syscalls
    void handleSyscallError(const char* message) {
        perror(message);
        throw std::runtime_error(message);
    }

    // Terminate a process by sending SIGTERM
    void terminateProcess(pid_t pid) {
        if (kill(pid, SIGTERM) == -1) {
            handleSyscallError("Failed to terminate process");
        }
    }

    // Wait for a specific process to finish
    int waitForProcess(pid_t pid) {
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            handleSyscallError("Failed to wait for process");
        }
        return status;
    }

    // Check the status of a process without blocking
    int checkProcessStatus(pid_t pid) {
        int status;
        if (waitpid(pid, &status, WNOHANG) == -1) {
            handleSyscallError("Failed to check process status");
        }
        return status;
    }

    // Notify when a thread finishes
    void onThreadFinish() {
        std::lock_guard<std::mutex> lock(m_processMutex);
        m_processInfos.erase(std::remove_if(m_processInfos.begin(), m_processInfos.end(),
                                             [](const ProcessInfo& p) { return p.isThread && !p.workerThread.joinable(); }),
                             m_processInfos.end());
        m_cv.notify_all();
    }

    std::vector<ProcessInfo> m_processInfos;
    std::mutex m_processMutex;
    std::condition_variable m_cv;
};

// Example usage of ProcessManager
int main() {
    try {
        // Example process creation
        ProcessManager& pm = ProcessManager::instance();

        // Add a process
        pm.addProcess([] {
            std::cout << "Process 1 running\n";
            sleep(2); // Simulate work
            std::cout << "Process 1 finished\n";
        });

        // Add another process with priority
        pm.addProcess([] {
            std::cout << "Process 2 running\n";
            sleep(3); // Simulate work
            std::cout << "Process 2 finished\n";
        });

        // Add a multithreaded process
        pm.addThreadProcess([] {
            std::cout << "Thread Process running\n";
            std::this_thread::sleep_for(std::chrono::seconds(1)); // Simulate work
            std::cout << "Thread Process finished\n";
        });

        // Wait for all processes and threads to finish
        pm.waitForAll();
        std::cout << "All processes and threads completed\n";
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
