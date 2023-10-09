// This header file contains declarations for functions related to process management

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

namespace ProcessManagement {

struct ProcessInfo {
    int pid;                   // Process ID
    std::string name;          // Name of the process
    bool running;              // Whether the process is currently running or not
    unsigned long long cpuTime; // Total CPU time spent by the process
    unsigned long long wallClockTime; // Wall clock time elapsed since the process started
};

typedef std::unordered_map<int, ProcessInfo> ProcessMap;

void getProcesses(ProcessMap &processes);
bool addProcess(const char *name, const char *commandLineArgs[]);
bool removeProcess(int pid);
bool terminateProcess(int pid);
unsigned long long getCPUTime(int pid);
unsigned long long getWallClockTime(int pid);

} // namespace ProcessManagement
