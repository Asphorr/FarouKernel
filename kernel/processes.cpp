#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <thread>

// Define a structure to hold process information
struct ProcessInfo {
    pid_t pid;
    int priority;
    std::thread::native_handle_type threadHandle;
};

std::vector<ProcessInfo> processInfos; // Vector to store process information

// Create a process and execute the given function
pid_t create_process(std::function<void()> func, int priority) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("Failed to create process");
        throw std::runtime_error("Failed to create process");
    } else if (pid == 0) {
        // Child process
        func();
        exit(EXIT_SUCCESS);
    } else {
        // Parent process
        processInfos.push_back({pid, priority, nullptr});
        return pid;
    }
}

// Wait for the specified process to finish
int wait_for_process(pid_t pid) {
    int status;
    waitpid(pid, &status, 0);
    return status;
}

// Add a process to the list to be waited for later
void add_process(std::function<void()> func, int priority) {
    processInfos.push_back({create_process(func, priority), priority, nullptr});
}

// Wait for all processes in the list to finish
void wait_all_processes() {
    for (auto& process : processInfos) {
        wait_for_process(process.pid);
    }
}

// Check the status of a process without blocking
int check_process_status(pid_t pid) {
    int status;
    waitpid(pid, &status, WNOHANG);
    return status;
}

// Terminate a process by sending SIGTERM
void terminate_process(pid_t pid) {
    kill(pid, SIGTERM);
}

// Execute a command using execvp
void execute_command(const char* command) {
    char* args[] = {(char*)command, nullptr};
    if (fork() == 0) {
        execvp(command, args);
        perror("exec failed");
        exit(EXIT_FAILURE);
    }
}

// Set a signal handler for the specified signal
void set_signal_handler(int signal, void (*handler)(int)) {
    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(signal, &sa, nullptr) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}

// Create a new process group with the specified PID
void create_process_group(pid_t pid) {
    setpgid(pid, pid);
}

// Join an existing process group with the specified PID
void join_process_group(pid_t pid) {
    setpgid(getpid(), pid);
}

// Add a process using multithreading
void add_process_mt(std::function<void()> func, int priority) {
    std::thread t(func);
    t.detach();
    processInfos.push_back({0, priority, t.native_handle()});
}

// Overloaded function to add a process with arguments
void add_process(std::function<void(int)> func, int priority, int arg) {
    std::thread t(func, arg);
    t.detach();
    processInfos.push_back({0, priority, t.native_handle()});
}
