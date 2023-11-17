#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <optional>

namespace ProcessManagement {

struct ProcessInfo {
 int pid;              // Process ID
 std::string name;      // Name of the process
 bool running;          // Whether the process is currently running or not
 unsigned long long cpuTime; // Total CPU time spent by the process
 unsigned long long wallClockTime; // Wall clock time elapsed since the process started
 time_t startTime;      // Start time of the process
 time_t endTime;        // End time of the process
 std::string owner;     // User who owns the process
 int priority;          // Priority of the process
 int status;            // Status of the process
 int numThreads;        // Number of threads of the process
 std::vector<std::string> commandLineArgs; // Command line arguments of the process
 std::filesystem::path workingDirectory; // Working directory of the process
 std::unordered_map<std::string, std::string> environmentVariables; // Environment variables of the process
 int parentProcess;     // Parent process of the process
 std::vector<int> childProcesses; // Child processes of the process
 unsigned long long memoryUsage; // Memory usage of the process
};

using ProcessMap = std::unordered_map<int, ProcessInfo>;

std::optional<ProcessMap> getProcesses();
bool addProcess(std::string_view name, const std::vector<std::string>& commandLineArgs);
bool removeProcess(int pid);
bool terminateProcess(int pid);
std::optional<unsigned long long> getCPUTime(int pid);
std::optional<unsigned long long> getWallClockTime(int pid);
std::optional<int> getParentProcess(int pid);
std::optional<std::vector<int>> getChildProcesses(int pid);
std::optional<unsigned long long> getMemoryUsage(int pid);
std::optional<int> getPriority(int pid);
std::optional<int> getStatus(int pid);
std::optional<int> getNumThreads(int pid);
std::optional<std::vector<std::string>> getCommandLineArgs(int pid);
std::optional<std::filesystem::path> getWorkingDirectory(int pid);
std::optional<std::unordered_map<std::string, std::string>> getEnvironmentVariables(int pid);
void setPriority(int pid, int priority);
void setStatus(int pid, int status);
void setNumThreads(int pid, int numThreads);
void setParentProcess(int pid, int parentProcess);
void setChildProcesses(int pid, const std::vector<int>& childProcesses);
void setMemoryUsage(int pid, unsigned long long memoryUsage);

} // namespace ProcessManagement
