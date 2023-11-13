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

std::vector<std::pair<pid_t, int>> process_pids; // Each process is a pair of PID and priority

pid_t create_process(std::function<void()> func, int priority) {
 pid_t pid = fork();
 if (pid == 0) {
    // Child process
    func();
    exit(0);
 } else if (pid < 0) {
    // Error occurred
    throw std::runtime_error("Failed to create process");
 } else {
    // Parent process
    process_pids.push_back(std::make_pair(pid, priority));
    return pid;
 }
}

int wait_for_process(pid_t pid) {
 int status;
 waitpid(pid, &status, 0);
 return status;
}

void add_process(std::function<void()> func, int priority) {
 process_pids.push_back(create_process(func, priority));
}

void wait_all_processes() {
 for (auto& pid : process_pids) {
   wait_for_process(pid.first);
 }
}

int check_process_status(pid_t pid) {
 int status;
 waitpid(pid, &status, WNOHANG);
 return status;
}

void terminate_process(pid_t pid) {
 kill(pid, SIGTERM);
}

void execute_command(const char* command) {
 system(command);
}

void set_signal_handler(int signal, void (*handler)(int)) {
 signal(signal, handler);
}

void create_process_group(pid_t pid) {
 setpgid(pid, pid);
}

void join_process_group(pid_t pid) {
 setpgid(getpid(), pid);
}
