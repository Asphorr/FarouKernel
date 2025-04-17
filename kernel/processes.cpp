#include <algorithm>
#include <csignal>
#include <condition_variable>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

class ProcessManager {
public:
    // Singleton instance access
    static ProcessManager& instance() {
        static ProcessManager instance;
        return instance;
    }

    // Delete copy/move constructors and assignment operators
    ProcessManager(const ProcessManager&) = delete;
    ProcessManager& operator=(const ProcessManager&) = delete;
    ProcessManager(ProcessManager&&) = delete;
    ProcessManager& operator=(ProcessManager&&) = delete;

    // Process management interface
    pid_t addProcess(std::function<void()> func, int priority = 0);
    void addThreadProcess(std::function<void()> func, int priority = 0);
    void waitForAll();
    void terminateAll();
    void pauseProcess(pid_t pid);
    void resumeProcess(pid_t pid);
    void setProcessPriority(pid_t pid, int priority);
    void reapZombies();

    // System configuration and maintenance
    void loadConfiguration(const std::string& configFile);
    void adjustPriorities();
    void scheduleProcesses();
    void loadBalance();
    void recoverProcess(pid_t pid);
    void setupIPC(pid_t pid);

    // Signal handling
    void setSignalHandler(int signal, void (*handler)(int));

private:
    // Nested ProcessInfo structure
    struct ProcessInfo {
        pid_t pid;
        int priority;
        std::thread workerThread;
        bool isThread;
        bool isPaused;
        std::function<void()> task;
    };

    // Private constructor/destructor for singleton
    ProcessManager();
    ~ProcessManager();

    // Internal helper functions
    void handleSyscallError(const char* message);
    void terminateProcess(pid_t pid);
    void logEvent(const std::string& message);
    void logProcessEvent(pid_t pid, const std::string& event);
    bool checkProcessStatus(pid_t pid, int& status);
    bool checkPermissions(pid_t pid, uid_t uid);
    void isolateProcess(pid_t pid);
    void processCLI();

    // Member variables
    std::vector<ProcessInfo> m_processInfos;
    std::mutex m_processMutex;
    std::condition_variable m_cv;
    std::mutex m_logMutex;
};

// ProcessManager Implementation

ProcessManager::ProcessManager() {
    std::srand(std::time(nullptr));  // Seed for simulated resource usage
}

ProcessManager::~ProcessManager() {
    terminateAll();
}

void ProcessManager::handleSyscallError(const char* message) {
    perror(message);
    throw std::runtime_error(message);
}

pid_t ProcessManager::addProcess(std::function<void()> func, int priority) {
    pid_t pid = fork();
    if (pid == -1) {
        handleSyscallError("Failed to create process");
    } else if (pid == 0) {  // Child process
        try {
            func();
            exit(EXIT_SUCCESS);
        } catch (const std::exception& e) {
            std::cerr << "Child process error: " << e.what() << '\n';
            exit(EXIT_FAILURE);
        }
    }

    // Parent process
    std::lock_guard<std::mutex> lock(m_processMutex);
    m_processInfos.push_back({pid, priority, std::thread(), false, false, func});
    return pid;
}

void ProcessManager::addThreadProcess(std::function<void()> func, int priority) {
    std::lock_guard<std::mutex> lock(m_processMutex);
    std::thread t([this, func, priority]() {
        try {
            func();
        } catch (const std::exception& e) {
            logEvent("Thread process failed: " + std::string(e.what()));
        }
    });
    m_processInfos.push_back({0, priority, std::move(t), true, false, func});
}

void ProcessManager::waitForAll() {
    std::unique_lock<std::mutex> lock(m_processMutex);
    m_cv.wait(lock, [this] { return m_processInfos.empty(); });
}

void ProcessManager::terminateAll() {
    std::lock_guard<std::mutex> lock(m_processMutex);
    for (auto& process : m_processInfos) {
        if (!process.isThread) {
            terminateProcess(process.pid);
        } else if (process.workerThread.joinable()) {
            process.workerThread.join();
        }
    }
    m_processInfos.clear();
    m_cv.notify_all();
}

void ProcessManager::pauseProcess(pid_t pid) {
    std::lock_guard<std::mutex> lock(m_processMutex);
    for (auto& process : m_processInfos) {
        if (process.pid == pid && !process.isPaused) {
            if (kill(pid, SIGSTOP) == -1) {
                handleSyscallError("Failed to pause process");
            }
            process.isPaused = true;
            break;
        }
    }
}

void ProcessManager::resumeProcess(pid_t pid) {
    std::lock_guard<std::mutex> lock(m_processMutex);
    for (auto& process : m_processInfos) {
        if (process.pid == pid && process.isPaused) {
            if (kill(pid, SIGCONT) == -1) {
                handleSyscallError("Failed to resume process");
            }
            process.isPaused = false;
            break;
        }
    }
}

void ProcessManager::setProcessPriority(pid_t pid, int priority) {
    std::lock_guard<std::mutex> lock(m_processMutex);
    for (auto& process : m_processInfos) {
        if (process.pid == pid) {
            process.priority = priority;
            if (setpriority(PRIO_PROCESS, pid, priority) == -1) {
                handleSyscallError("Failed to set process priority");
            }
            break;
        }
    }
}

void ProcessManager::reapZombies() {
    std::lock_guard<std::mutex> lock(m_processMutex);
    bool wasEmpty = m_processInfos.empty();
    
    auto it = m_processInfos.begin();
    while (it != m_processInfos.end()) {
        if (!it->isThread) {
            int status;
            if (waitpid(it->pid, &status, WNOHANG) > 0) {
                logProcessEvent(it->pid, "Process exited with status: " + std::to_string(WEXITSTATUS(status)));
                it = m_processInfos.erase(it);
                continue;
            }
        }
        ++it;
    }

    if (!wasEmpty && m_processInfos.empty()) {
        m_cv.notify_all();
    }
}

void ProcessManager::logEvent(const std::string& message) {
    std::lock_guard<std::mutex> lock(m_logMutex);
    std::ofstream logFile("process_manager.log", std::ios::app);
    if (logFile.is_open()) {
        logFile << message << std::endl;
    }
}

void ProcessManager::logProcessEvent(pid_t pid, const std::string& event) {
    logEvent("Process " + std::to_string(pid) + ": " + event);
}

void ProcessManager::adjustPriorities() {
    std::lock_guard<std::mutex> lock(m_processMutex);
    for (auto& process : m_processInfos) {
        if (process.isThread || process.isPaused) continue;

        // Example priority adjustment logic
        if (process.priority < 10) {
            setProcessPriority(process.pid, process.priority + 1);
        }
    }
}

void ProcessManager::scheduleProcesses() {
    std::lock_guard<std::mutex> lock(m_processMutex);
    std::sort(m_processInfos.begin(), m_processInfos.end(),
        [](const ProcessInfo& a, const ProcessInfo& b) {
            return a.priority > b.priority;
        });
}

void ProcessManager::setSignalHandler(int signal, void (*handler)(int)) {
    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(signal, &sa, nullptr) == -1) {
        handleSyscallError("Failed to set signal handler");
    }
}

void ProcessManager::terminateProcess(pid_t pid) {
    if (kill(pid, SIGTERM) == -1) {
        handleSyscallError("Failed to terminate process");
    }
}

// Example usage
int main() {
    try {
        ProcessManager& pm = ProcessManager::instance();

        // Example process creation
        pm.addProcess([] {
            std::cout << "Process 1 running\n";
            sleep(2);
            std::cout << "Process 1 finished\n";
        });

        pm.addProcess([] {
            std::cout << "Process 2 running\n";
            sleep(3);
            std::cout << "Process 2 finished\n";
        });

        pm.addThreadProcess([] {
            std::cout << "Thread process running\n";
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "Thread process finished\n";
        });

        pm.waitForAll();
        std::cout << "All processes completed\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
