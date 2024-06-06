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
#include <chrono>
#include <map>

// Process Information Structure
struct ProcessInfo {
    pid_t pid;
    int priority;
    std::thread workerThread;
    bool isThread;
    bool isPaused;  // New field to track if the process is paused
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
        std::lock_guard<std::mutex> lock(m_processMutex);  // Lock before forking
        pid_t pid = fork();
        if (pid == -1) {
            handleSyscallError("Failed to create process");
        } else if (pid == 0) {
            // Child process
            func();
            exit(EXIT_SUCCESS);
        }
        // Parent process: add process info to the list
        m_processInfos.push_back({pid, priority, std::thread(), false, false});
        setPriority(pid, priority);  // Set priority
        return pid;
    }

    // Add a new multithreaded process
    void addThreadProcess(std::function<void()> func, int priority = 0) {
        std::thread t([this, func, priority] {
            func();
            onThreadFinish();  // Notify on completion
        });
        std::lock_guard<std::mutex> lock(m_processMutex);
        m_processInfos.push_back({0, priority, std::move(t), true, false});
    }

    // Wait for all processes and threads to finish
    void waitForAll() {
        std::unique_lock<std::mutex> lock(m_processMutex);
        m_cv.wait(lock, [this] {
            return std::all_of(m_processInfos.begin(), m_processInfos.end(), [this](const ProcessInfo& p) {
                if (p.isThread) {
                    if (p.workerThread.joinable()) {
                        p.workerThread.join();
                    }
                    return true;
                } else {
                    return checkProcessStatus(p.pid) != -1;
                }
            });
        });
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

    // Handle signal with additional flags for flexibility
    void handleSignal(int signal, void (*handler)(int), int flags = 0) {
        struct sigaction sa;
        sa.sa_handler = handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = flags;
        if (sigaction(signal, &sa, nullptr) == -1) {
            handleSyscallError("Failed to handle signal");
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

    // Pause a process
    void pauseProcess(pid_t pid) {
        std::lock_guard<std::mutex> lock(m_processMutex);
        if (kill(pid, SIGSTOP) == -1) {
            handleSyscallError("Failed to pause process");
        } else {
            updateProcessStatus(pid, true);
        }
    }

    // Resume a paused process
    void resumeProcess(pid_t pid) {
        std::lock_guard<std::mutex> lock(m_processMutex);
        if (kill(pid, SIGCONT) == -1) {
            handleSyscallError("Failed to resume process");
        } else {
            updateProcessStatus(pid, false);
        }
    }

    // Reap zombie processes
    void reapZombies() {
        std::lock_guard<std::mutex> lock(m_processMutex);
        for (auto it = m_processInfos.begin(); it != m_processInfos.end();) {
            if (!it->isThread && checkProcessStatus(it->pid) == -1) {
                it = m_processInfos.erase(it);
            } else {
                ++it;
            }
        }
    }

    // Set process priority
    void setPriority(pid_t pid, int priority) {
        if (setpriority(PRIO_PROCESS, pid, priority) == -1) {
            handleSyscallError("Failed to set process priority");
        }
    }

    // Get process information
    std::vector<ProcessInfo> getProcessInfo() {
        std::lock_guard<std::mutex> lock(m_processMutex);
        return m_processInfos;
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

    // Notify when a thread finishes execution
    void onThreadFinish() {
        std::lock_guard<std::mutex> lock(m_processMutex);
        m_processInfos.erase(std::remove_if(m_processInfos.begin(), m_processInfos.end(),
                                             [](const ProcessInfo& p) { return p.isThread && !p.workerThread.joinable(); }),
                             m_processInfos.end());
        m_cv.notify_all();
    }

    // Update the status of a process (paused or not)
    void updateProcessStatus(pid_t pid, bool isPaused) {
        for (auto& process : m_processInfos) {
            if (process.pid == pid) {
                process.isPaused = isPaused;
                break;
            }
        }
    }

    std::vector<ProcessInfo> m_processInfos;
    std::mutex m_processMutex;
    std::condition_variable m_cv;
};

// Example usage of ProcessManager
int main()
{
    try {
        ProcessManager& pm = ProcessManager::instance();

        // Add a process
        pm.addProcess([] {
            std::cout << "Process 1 running\n";
            sleep(2);
            std::cout << "Process 1 finished\n";
        });

        // Add another process with priority
        pm.addProcess([] {
            std::cout << "Process 2 running\n";
            sleep(3);
            std::cout << "Process 2 finished\n";
        });

        // Add a multithreaded process
        pm.addThreadProcess([] {
            std::cout << "Thread Process running\n";
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "Thread Process finished\n";
        });

        // Pause and resume a process
        auto pids = pm.getProcessInfo();
        if (!pids.empty()) {
            pm.pauseProcess(pids[0].pid);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            pm.resumeProcess(pids[0].pid);
        }

        // Wait for all processes and threads to finish
        pm.waitForAll();
        std::cout << "All processes and threads completed\n";
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
