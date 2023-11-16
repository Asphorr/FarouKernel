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
#include <future>

std::vector<std::pair<pid_t, int>> process_pids; // Each process is a pair of PID and priority

pid_t create_process(std::function<void()> func, int priority) {
 pid_t pid = fork();
 if (pid == 0) {
  // Child process
  func();
  exit(0);
 } else if (pid < 0) {
  // Error occurred
  perror("Failed to create process");
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
  char* args[] = {(char*)command, NULL};
  if (fork() == 0) {
      execvp(command, args);
      perror("exec failed");
      exit(EXIT_FAILURE);
  }
}

void set_signal_handler(int signal, void (*handler)(int)) {
  struct sigaction sa;
  sa.sa_handler = handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  if (sigaction(signal, &sa, NULL) == -1) {
      perror("sigaction");
      exit(EXIT_FAILURE);
  }
}

void create_process_group(pid_t pid) {
 setpgid(pid, pid);
}

void join_process_group(pid_t pid) {
 setpgid(getpid(), pid);
}

// New function using multithreading
void add_process_mt(std::function<void()> func, int priority) {
 std::thread t(func);
 t.detach();
 process_pids.push_back(std::make_pair(t.native_handle(), priority));
}

// New function using function overloading
void add_process(std::function<void(int)> func, int priority, int arg) {
 std::thread t(func, arg);
 t.detach();
 process_pids.push_back(std::make_pair(t.native_handle(), priority));
}
