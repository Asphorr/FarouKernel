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

struct ProcessInfo {
    pid_t pid;
    int priority;
    std::thread workerThread;
    bool isThread;
};

std::vector<ProcessInfo> processInfos;
std::mutex processMutex;  // Mutex to protect access to processInfos

// Utility function to handle errors in syscalls
void handle_syscall_error(const char* message) {
    perror(message);
    throw std::runtime_error(message);
}

// Create a process to execute the given function
pid_t create_process(std::function<void()> func, int priority) {
    pid_t pid = fork();

    if (pid == -1) {
        handle_syscall_error("Failed to create process");
    } else if (pid == 0) {
        // Child process
        func();
        exit(EXIT_SUCCESS);
    }

    // Parent process: add process info to the list
    std::lock_guard<std::mutex> lock(processMutex);
    processInfos.push_back({pid, priority, std::thread(), false});
    return pid;
}

// Wait for the specified process to finish
int wait_for_process(pid_t pid) {
    int status;
    if (waitpid(pid, &status, 0) == -1) {
        handle_syscall_error("Failed to wait for process");
    }
    return status;
}

// Add a process to the list to be waited for later
void add_process(std::function<void()> func, int priority) {
    create_process(func, priority);
}

// Wait for all processes and threads in the list to finish
void wait_all_processes() {
    std::lock_guard<std::mutex> lock(processMutex);
    for (auto& process : processInfos) {
        if (!process.isThread) {
            wait_for_process(process.pid);
        } else if (process.workerThread.joinable()) {
            process.workerThread.join();
        }
    }
    processInfos.clear();
}

// Check the status of a process without blocking
int check_process_status(pid_t pid) {
    int status;
    if (waitpid(pid, &status, WNOHANG) == -1) {
        handle_syscall_error("Failed to check process status");
    }
    return status;
}

// Terminate a process by sending SIGTERM
void terminate_process(pid_t pid) {
    if (kill(pid, SIGTERM) == -1) {
        handle_syscall_error("Failed to terminate process");
    }
}

// Function to set up signal handling
void set_signal_handler(int signal, void (*handler)(int)) {
    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(signal, &sa, nullptr) == -1) {
        handle_syscall_error("Failed to set signal handler");
    }
}

// Create a new process group
void create_process_group(pid_t pid) {
    if (setpgid(pid, pid) == -1) {
        handle_syscall_error("Failed to create process group");
    }
}

// Join an existing process group
void join_process_group(pid_t pid) {
    if (setpgid(getpid(), pid) == -1) {
        handle_syscall_error("Failed to join process group");
    }
}

// Function to handle multithreading
void add_process_mt(std::function<void()> func, int priority) {
    std::lock_guard<std::mutex> lock(processMutex);
    std::thread t(func);
    processInfos.push_back({0, priority, std::move(t), true});
}

int main() {
    try {
        add_process([]() {
            std::cout << "Process running\n";
            sleep(2);  // Simulate work
        }, 1);

        wait_all_processes();
        std::cout << "All processes completed\n";
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
