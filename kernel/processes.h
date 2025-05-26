#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <optional>
#include <mutex>
#include <chrono>
#include <algorithm>
#include <stdexcept>

namespace ProcessManagement {

struct ProcessInfo {
    int pid;
    std::string name;
    bool running;
    unsigned long long cpuTime;
    unsigned long long wallClockTime;
    time_t startTime;
    time_t endTime;
    std::string owner;
    int priority;
    int status;
    int numThreads;
    std::vector<std::string> commandLineArgs;
    std::filesystem::path workingDirectory;
    std::unordered_map<std::string, std::string> environmentVariables;
    int parentProcess;
    std::vector<int> childProcesses;
    unsigned long long memoryUsage;
};

using ProcessMap = std::unordered_map<int, ProcessInfo>;

class ProcessManager {
public:
    std::optional<ProcessInfo> getProcessInfo(int pid) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = processMap.find(pid);
        if (it != processMap.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    std::optional<ProcessMap> getProcesses() {
        std::lock_guard<std::mutex> lock(mutex_);
        return processMap;
    }

    bool addProcess(std::string_view name, const std::vector<std::string>& commandLineArgs) {
        std::lock_guard<std::mutex> lock(mutex_);
        int pid = generateUniquePid();
        ProcessInfo info;
        info.pid = pid;
        info.name = std::string(name);
        info.running = true;
        info.cpuTime = 0;
        info.wallClockTime = 0;
        info.startTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        info.endTime = 0;
        info.owner = getCurrentUser();
        info.priority = 0;
        info.status = 0;
        info.numThreads = 1;
        info.commandLineArgs = commandLineArgs;
        info.workingDirectory = std::filesystem::current_path();
        info.environmentVariables = getCurrentEnvironment();
        info.parentProcess = getCurrentProcessId();
        info.memoryUsage = 0;

        processMap[pid] = std::move(info);
        return true;
    }

    bool removeProcess(int pid) {
        std::lock_guard<std::mutex> lock(mutex_);
        return processMap.erase(pid) > 0;
    }

    bool terminateProcess(int pid) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = processMap.find(pid);
        if (it != processMap.end()) {
            it->second.running = false;
            it->second.endTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            return true;
        }
        return false;
    }

    std::optional<unsigned long long> getCPUTime(int pid) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = processMap.find(pid);
        if (it != processMap.end()) {
            return it->second.cpuTime;
        }
        return std::nullopt;
    }

    std::optional<unsigned long long> getWallClockTime(int pid) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = processMap.find(pid);
        if (it != processMap.end()) {
            return it->second.wallClockTime;
        }
        return std::nullopt;
    }

    std::optional<int> getParentProcess(int pid) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = processMap.find(pid);
        if (it != processMap.end()) {
            return it->second.parentProcess;
        }
        return std::nullopt;
    }

    std::optional<std::vector<int>> getChildProcesses(int pid) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = processMap.find(pid);
        if (it != processMap.end()) {
            return it->second.childProcesses;
        }
        return std::nullopt;
    }

    std::optional<unsigned long long> getMemoryUsage(int pid) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = processMap.find(pid);
        if (it != processMap.end()) {
            return it->second.memoryUsage;
        }
        return std::nullopt;
    }

    std::optional<int> getPriority(int pid) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = processMap.find(pid);
        if (it != processMap.end()) {
            return it->second.priority;
        }
        return std::nullopt;
    }

    std::optional<int> getStatus(int pid) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = processMap.find(pid);
        if (it != processMap.end()) {
            return it->second.status;
        }
        return std::nullopt;
    }

    std::optional<int> getNumThreads(int pid) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = processMap.find(pid);
        if (it != processMap.end()) {
            return it->second.numThreads;
        }
        return std::nullopt;
    }

    std::optional<std::vector<std::string>> getCommandLineArgs(int pid) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = processMap.find(pid);
        if (it != processMap.end()) {
            return it->second.commandLineArgs;
        }
        return std::nullopt;
    }

    std::optional<std::filesystem::path> getWorkingDirectory(int pid) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = processMap.find(pid);
        if (it != processMap.end()) {
            return it->second.workingDirectory;
        }
        return std::nullopt;
    }

    std::optional<std::unordered_map<std::string, std::string>> getEnvironmentVariables(int pid) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = processMap.find(pid);
        if (it != processMap.end()) {
            return it->second.environmentVariables;
        }
        return std::nullopt;
    }

    void setPriority(int pid, int priority) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = processMap.find(pid);
        if (it != processMap.end()) {
            it->second.priority = priority;
        }
    }

    void setStatus(int pid, int status) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = processMap.find(pid);
        if (it != processMap.end()) {
            it->second.status = status;
        }
    }

    void setNumThreads(int pid, int numThreads) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = processMap.find(pid);
        if (it != processMap.end()) {
            it->second.numThreads = numThreads;
        }
    }

    void setParentProcess(int pid, int parentProcess) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = processMap.find(pid);
        if (it != processMap.end()) {
            it->second.parentProcess = parentProcess;
        }
    }

    void setChildProcesses(int pid, const std::vector<int>& childProcesses) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = processMap.find(pid);
        if (it != processMap.end()) {
            it->second.childProcesses = childProcesses;
        }
    }

    void setMemoryUsage(int pid, unsigned long long memoryUsage) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = processMap.find(pid);
        if (it != processMap.end()) {
            it->second.memoryUsage = memoryUsage;
        }
    }

    bool suspendProcess(int pid) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = processMap.find(pid);
        if (it != processMap.end() && it->second.running) {
            it->second.running = false;
            return true;
        }
        return false;
    }

    bool resumeProcess(int pid) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = processMap.find(pid);
        if (it != processMap.end() && !it->second.running) {
            it->second.running = true;
            return true;
        }
        return false;
    }

    std::optional<std::string> getProcessOwner(int pid) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = processMap.find(pid);
        if (it != processMap.end()) {
            return it->second.owner;
        }
        return std::nullopt;
    }

    void setProcessOwner(int pid, const std::string& owner) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = processMap.find(pid);
        if (it != processMap.end()) {
            it->second.owner = owner;
        }
    }

    std::optional<time_t> getProcessStartTime(int pid) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = processMap.find(pid);
        if (it != processMap.end()) {
            return it->second.startTime;
        }
        return std::nullopt;
    }

    void setProcessStartTime(int pid, time_t startTime) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = processMap.find(pid);
        if (it != processMap.end()) {
            it->second.startTime = startTime;
        }
    }

    std::optional<time_t> getProcessEndTime(int pid) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = processMap.find(pid);
        if (it != processMap.end()) {
            return it->second.endTime;
        }
        return std::nullopt;
    }

    void setProcessEndTime(int pid, time_t endTime) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = processMap.find(pid);
        if (it != processMap.end()) {
            it->second.endTime = endTime;
        }
    }

    void setProcessRunning(int pid, bool running) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = processMap.find(pid);
        if (it != processMap.end()) {
            it->second.running = running;
        }
    }

    bool isProcessRunning(int pid) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = processMap.find(pid);
        if (it != processMap.end()) {
            return it->second.running;
        }
        return false;
    }

private:
    ProcessMap processMap;
    std::mutex mutex_;

    int generateUniquePid() {
        static int nextPid = 1;
        return nextPid++;
    }

    std::string getCurrentUser() {
        // This is a placeholder. In a real implementation, you would use
        // platform-specific APIs to get the current user.
        return "current_user";
    }

    int getCurrentProcessId() {
        // This is a placeholder. In a real implementation, you would use
        // platform-specific APIs to get the current process ID.
        return 0;
    }

    std::unordered_map<std::string, std::string> getCurrentEnvironment() {
        // This is a placeholder. In a real implementation, you would use
        // platform-specific APIs to get the current environment variables.
        return {{"PATH", "/usr/bin:/bin"}};
    }
};

} // namespace ProcessManagement
