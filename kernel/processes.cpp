#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <sys/prctl.h> // For setting child death signal
#include <sys/resource.h> // For setpriority
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>
#include <map> // Using map for easier lookup by id

// Forward declaration
class ProcessManager;

// --- Helper Structures ---

// State for managed processes
enum class ProcessState { RUNNING, PAUSED, EXITED, TERMINATED };

struct ProcessData {
    pid_t pid;
    int priority;
    ProcessState state;
    // Store original func? Maybe useful for recovery, but adds complexity. Omitting for now.

    ProcessData(pid_t p, int prio) : pid(p), priority(prio), state(ProcessState::RUNNING) {}
};

// State for managed threads
struct ThreadData {
    std::thread workerThread;
    std::thread::id id;
    int priority; // Note: std::thread priority is harder to manage directly than process priority
    std::atomic<bool>& stopToken; // Reference to a shared stop token
    bool joined;

    // Must take stopToken by reference, created outside
    ThreadData(std::thread t, int prio, std::atomic<bool>& token) :
        workerThread(std::move(t)), id(workerThread.get_id()), priority(prio), stopToken(token), joined(false) {}

    // Need custom move constructor because of reference member
    ThreadData(ThreadData&& other) noexcept :
        workerThread(std::move(other.workerThread)),
        id(other.id),
        priority(other.priority),
        stopToken(other.stopToken), // Reference itself is copied
        joined(other.joined)
    {
         other.joined = true; // Prevent double join/detach in moved-from object
    }

    ThreadData& operator=(ThreadData&& other) noexcept {
        if (this != &other) {
            // Clean up own thread if necessary before overwriting
            if (workerThread.joinable() && !joined) {
                 // Decide: detach or try join? Detach is simpler here.
                 workerThread.detach();
            }
            workerThread = std::move(other.workerThread);
            id = other.id;
            priority = other.priority;
            // stopToken reference is tricky in assignment, often better to avoid assignment
            // For simplicity here, assume stopToken reference remains valid or redesign needed
            // If ThreadData were stored differently (e.g. unique_ptr), this is easier
            // Let's assume stopToken is managed externally and reference remains valid.
            joined = other.joined;
            other.joined = true;
        }
        return *this;
    }

    // Delete copy operations
    ThreadData(const ThreadData&) = delete;
    ThreadData& operator=(const ThreadData&) = delete;

    ~ThreadData() {
         // Ensure thread is joined or detached before destruction
         if (workerThread.joinable() && !joined) {
            // Log error? Detach? Join can block destructor. Detaching is often safer here.
            // std::cerr << "Warning: ThreadData for id " << id << " destroyed without joining." << std::endl;
            workerThread.detach();
         }
    }
};


// --- ProcessManager Class ---

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

    // --- Process/Thread Management Interface ---

    // Adds a new process using fork()
    // Returns the PID of the created process. Throws on error.
    pid_t addProcess(std::function<void()> func, int priority = 0);

    // Adds a new thread using std::thread
    // Returns the ID of the created thread. Throws on error.
    std::thread::id addThread(std::function<void(std::atomic<bool>&)> func, int priority = 0);

    // Waits until all managed processes have exited and all managed threads have finished.
    void waitForAll();

    // Attempts to terminate all managed processes and threads gracefully, then forcefully.
    void terminateAll(std::chrono::milliseconds gracePeriod = std::chrono::milliseconds(1000));

    // Pauses a process using SIGSTOP.
    void pauseProcess(pid_t pid);

    // Resumes a paused process using SIGCONT.
    void resumeProcess(pid_t pid);

    // Sets the nice value (priority) of a process. Lower value means higher priority.
    void setProcessPriority(pid_t pid, int niceValue);

    // Signals a specific thread to stop (cooperative cancellation).
    void requestThreadStop(std::thread::id id);

    // --- Signal Handling ---
    // Sets a signal handler for the manager process itself. Use with caution.
    void setSignalHandler(int signal, void (*handler)(int));

private:
    // Private constructor/destructor for singleton
    ProcessManager();
    ~ProcessManager();

    // --- Internal Helper Functions ---
    void handleSyscallError(const std::string& message, bool fatal = true);
    void logEvent(const std::string& message);
    void logProcessEvent(pid_t pid, const std::string& event);
    void logThreadEvent(std::thread::id id, const std::string& event);

    // The loop executed by the reaper thread to handle SIGCHLD
    void reaperLoop();

    // Internal termination helpers
    void terminateProcessInternal(pid_t pid, int signal);
    void joinThreadInternal(std::thread::id id); // Finds and joins a thread

    // Signal handling setup
    void setupSignalHandling();
    static void staticSignalHandler(int signal); // Static handler to trampoline to instance

    // --- Member Variables ---
    std::map<pid_t, ProcessData> m_processes; // Map PID to ProcessData
    std::map<std::thread::id, ThreadData> m_threads; // Map thread::id to ThreadData
    std::map<std::thread::id, std::atomic<bool>> m_threadStopTokens; // Separate map for stop tokens

    std::mutex m_mutex; // Protects access to m_processes, m_threads, m_threadStopTokens, and counts
    std::condition_variable m_cvFinished; // Signalled when counts reach zero

    std::atomic<size_t> m_activeProcessCount{0};
    std::atomic<size_t> m_activeThreadCount{0};

    std::thread m_reaperThread;
    std::atomic<bool> m_stopReaper{false};
    int m_reaperPipe[2] = {-1, -1}; // Pipe to wake up reaper thread reliably

    std::mutex m_logMutex;
    std::ofstream m_logFile;

    // Used for the static signal handler trampoline
    static ProcessManager* g_instance; // Global pointer to the singleton instance for signal handler
    static std::mutex g_instanceMutex; // Protects g_instance creation/access (though singleton pattern handles this)
};

// Initialize static member for signal handler
ProcessManager* ProcessManager::g_instance = nullptr;
std::mutex ProcessManager::g_instanceMutex;

// --- ProcessManager Implementation ---

ProcessManager::ProcessManager() {
    // Setup logging
    m_logFile.open("process_manager.log", std::ios::app);
    if (!m_logFile.is_open()) {
        std::cerr << "Warning: Failed to open log file 'process_manager.log'." << std::endl;
    }
    logEvent("ProcessManager initializing...");

    // Setup singleton instance pointer for signal handler (thread-safe)
    {
        std::lock_guard<std::mutex> lock(g_instanceMutex);
        if (g_instance == nullptr) {
            g_instance = this;
        } else {
            // This shouldn't happen with the Meyers singleton pattern, but safety first
            handleSyscallError("ProcessManager singleton re-initialization attempt", true);
        }
    }

    setupSignalHandling();

    // Create a pipe for waking up the reaper thread
    if (pipe(m_reaperPipe) == -1) {
        handleSyscallError("Failed to create reaper pipe", true);
    }

    // Start the reaper thread
    m_reaperThread = std::thread(&ProcessManager::reaperLoop, this);

    logEvent("ProcessManager initialized successfully.");
}

ProcessManager::~ProcessManager() {
    logEvent("ProcessManager shutting down...");

    // Attempt graceful termination
    terminateAll(std::chrono::milliseconds(500)); // Shorter grace period in destructor

    // Signal reaper thread to stop
    m_stopReaper.store(true);

    // Wake up the reaper thread if it's blocked in sigwaitinfo or select
    char dummy = 'x';
    if (write(m_reaperPipe[1], &dummy, 1) == -1 && errno != EPIPE) {
       // Log warning, but proceed. EPIPE is ok if reader closed already.
       logEvent("Warning: Could not write to reaper pipe during shutdown.");
    }


    // Join the reaper thread
    if (m_reaperThread.joinable()) {
        logEvent("Joining reaper thread...");
        m_reaperThread.join();
        logEvent("Reaper thread joined.");
    } else {
        logEvent("Warning: Reaper thread was not joinable.");
    }

     // Close the pipe file descriptors
    if (m_reaperPipe[0] != -1) close(m_reaperPipe[0]);
    if (m_reaperPipe[1] != -1) close(m_reaperPipe[1]);


    logEvent("ProcessManager shut down complete.");
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
}

void ProcessManager::handleSyscallError(const std::string& message, bool fatal) {
    std::string errorMsg = message + ": " + strerror(errno);
    logEvent("Error: " + errorMsg); // Log it first
    if (fatal) {
        // In a real manager, might attempt cleanup before exiting
        std::cerr << "Fatal Error: " << errorMsg << std::endl;
        // No throw in destructor path, could std::terminate
        // Outside destructor, throw is okay.
         if (!std::current_exception()) { // Avoid throwing if already unwinding
            throw std::runtime_error(errorMsg);
        } else {
            std::cerr << "Terminating due to fatal error during potential cleanup/exception handling." << std::endl;
            std::terminate();
        }
    } else {
        std::cerr << "Warning: " << errorMsg << std::endl;
    }
}

void ProcessManager::logEvent(const std::string& message) {
    std::lock_guard<std::mutex> lock(m_logMutex);
    if (m_logFile.is_open()) {
        // Add timestamp (optional)
        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);
        m_logFile << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S") << " | " << message << std::endl;
    }
    // Optionally also log to cerr for immediate visibility
    // std::cerr << "LOG: " << message << std::endl;
}

void ProcessManager::logProcessEvent(pid_t pid, const std::string& event) {
    logEvent("Process [" + std::to_string(pid) + "]: " + event);
}

void ProcessManager::logThreadEvent(std::thread::id id, const std::string& event) {
    std::stringstream ss;
    ss << id;
    logEvent("Thread [" + ss.str() + "]: " + event);
}


void ProcessManager::setupSignalHandling() {
    // Block SIGCHLD in the main thread and any threads it creates *before* they are created.
    // The reaper thread will specifically wait for it.
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    if (pthread_sigmask(SIG_BLOCK, &mask, nullptr) != 0) {
        handleSyscallError("Failed to block SIGCHLD", true);
    }

    // Setup a handler for other signals if needed (e.g., SIGTERM for graceful shutdown)
    setSignalHandler(SIGTERM, ProcessManager::staticSignalHandler);
    setSignalHandler(SIGINT, ProcessManager::staticSignalHandler);
    // Ignore SIGPIPE commonly
    setSignalHandler(SIGPIPE, SIG_IGN);
}

// Static handler that calls the instance's handler (if needed) or performs simple actions
void ProcessManager::staticSignalHandler(int signal) {
     // WARNING: Async-signal-safety is crucial here. Avoid complex logic, locks, heap allocation, most functions.
     // A common safe pattern is to set a flag or write to a self-pipe.
     // For simplicity here, just log (write is usually safe) and potentially trigger shutdown.

    // Simple signal handling example: trigger shutdown on SIGTERM/SIGINT
    if (signal == SIGTERM || signal == SIGINT) {
        // Writing to stderr might not be strictly safe, but common for simple cases.
        // A safer approach uses write() to a pre-opened FD or the self-pipe.
        const char* msg = "Received termination signal, initiating shutdown...\n";
        write(STDERR_FILENO, msg, strlen(msg)); // write is async-signal-safe

        // Trigger shutdown - This is NOT async-signal-safe if terminateAll locks etc.
        // A better way: set an atomic flag and have a main loop check it, or signal a condition var.
        // Or, use the self-pipe trick to wake a dedicated handling thread/loop.
        // For this example, we *won't* call terminateAll directly from here due to safety issues.
        // Rely on external cleanup or main loop checking.
        // If g_instance is needed:
        // std::lock_guard<std::mutex> lock(g_instanceMutex); // LOCKING IS UNSAFE HERE
        // if (g_instance) {
        //    g_instance->logEvent("Received signal: " + std::to_string(signal)); // Logging might use locks - UNSAFE
        //    // g_instance->terminateAll(); // DEFINITELY UNSAFE
        // }

         // A slightly safer approach for simple exit:
         _exit(EXIT_FAILURE); // _exit is async-signal-safe
    }
}


void ProcessManager::reaperLoop() {
    logEvent("Reaper thread started.");
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);

    // File descriptor set for select/pselect
    fd_set read_fds;
    int pipe_read_fd = m_reaperPipe[0];


    while (!m_stopReaper.load()) {
        FD_ZERO(&read_fds);
        FD_SET(pipe_read_fd, &read_fds);
        int max_fd = pipe_read_fd;

        // Use pselect to wait for SIGCHLD OR data on the pipe, without modifying the process signal mask
        // We need to prepare the signal mask *not* to block SIGCHLD temporarily for pselect
        sigset_t original_mask;
        pthread_sigmask(SIG_SETMASK, nullptr, &original_mask); // Get current mask
        sigset_t wait_mask = original_mask;
        sigdelset(&wait_mask, SIGCHLD); // Allow SIGCHLD during pselect

        int result = pselect(max_fd + 1, &read_fds, nullptr, nullptr, nullptr, &wait_mask); // No timeout, wait indefinitely

        if (m_stopReaper.load()) { // Check stop flag immediately after waking up
             break;
        }

        if (result == -1) {
            if (errno == EINTR) {
                // Interrupted by a signal (like SIGCHLD, or another handled one)
                // We primarily care if SIGCHLD was the cause. Check below.
            } else {
                 handleSyscallError("pselect error in reaper loop", false); // Log non-fatal error
                 std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Prevent busy-loop on persistent error
                 continue;
            }
        }

        // Check if woken by the pipe (shutdown signal)
        if (FD_ISSET(pipe_read_fd, &read_fds)) {
             char buf[16];
             // Drain the pipe
             while(read(pipe_read_fd, buf, sizeof(buf)) > 0);
             if (m_stopReaper.load()) { // Re-check stop flag after reading pipe
                 break;
             }
        }


        // Regardless of pselect result (EINTR or signal), check for zombies using WNOHANG
        while (true) {
            int status;
            pid_t pid = waitpid(-1, &status, WNOHANG); // Check for ANY child

            if (pid > 0) {
                // Found a zombie
                std::lock_guard<std::mutex> lock(m_mutex);
                auto it = m_processes.find(pid);
                if (it != m_processes.end()) {
                    std::string exit_reason;
                    if (WIFEXITED(status)) {
                        exit_reason = "exited normally with status " + std::to_string(WEXITSTATUS(status));
                        it->second.state = ProcessState::EXITED;
                    } else if (WIFSIGNALED(status)) {
                        exit_reason = "terminated by signal " + std::to_string(WTERMSIG(status));
                        it->second.state = ProcessState::TERMINATED;
                    } else {
                         exit_reason = "stopped or continued (unexpected)"; // Should be handled by pause/resume
                    }

                    logProcessEvent(pid, "Reaped. Status: " + exit_reason);

                    // Decrement count and remove from map
                    m_processes.erase(it);
                    size_t prev_count = m_activeProcessCount.fetch_sub(1);

                    // Notify if counts reached zero *after* decrementing
                    if (prev_count == 1 && m_activeThreadCount.load() == 0) {
                        m_cvFinished.notify_all();
                    }
                } else {
                    // Reaped a process not managed by us? Or already removed? Log warning.
                    logEvent("Warning: Reaped unknown or already removed process PID: " + std::to_string(pid));
                }
            } else if (pid == 0) {
                // No more zombies waiting
                break;
            } else {
                // Error
                if (errno == ECHILD) {
                    // No children exist (or none we can wait for)
                    break;
                } else if (errno == EINTR) {
                    // Interrupted, retry waitpid
                    continue;
                } else {
                    handleSyscallError("waitpid error in reaper loop", false);
                    break; // Avoid busy loop on error
                }
            }
        } // End while(true) for waitpid loop
    } // End while(!m_stopReaper)

    logEvent("Reaper thread finished.");
}


// --- Public Interface Implementation ---

pid_t ProcessManager::addProcess(std::function<void()> func, int priority) {
    pid_t pid = fork();

    if (pid == -1) {
        handleSyscallError("Failed to fork process"); // Throws
        return -1; // Should not be reached
    } else if (pid == 0) {
        // --- Child Process ---
        // 1. Reset signal handlers/mask if necessary (especially SIGCHLD)
        struct sigaction sa_default;
        sa_default.sa_handler = SIG_DFL;
        sigemptyset(&sa_default.sa_mask);
        sa_default.sa_flags = 0;
        sigaction(SIGCHLD, &sa_default, nullptr);
        // sigaction(SIGTERM, &sa_default, nullptr); // Inherit parent's handlers? Or default?
        // sigaction(SIGINT, &sa_default, nullptr);

        sigset_t mask;
        sigemptyset(&mask);
        // sigaddset(&mask, SIGCHLD); // Unblock SIGCHLD if parent blocked it system-wide
        pthread_sigmask(SIG_SETMASK, &mask, nullptr); // Set empty mask or unblock specific signals

        // 2. Set priority (nice value) if requested
        if (priority != 0) {
            if (setpriority(PRIO_PROCESS, 0, priority) == -1) { // 0 means current process
                perror("Child: Failed to set priority"); // Non-fatal error in child
            }
        }

        // 3. Ask kernel to send a signal (e.g. SIGCHLD) to parent when child exits
        //    This is often the default, but prctl can make it explicit or change the signal.
        // prctl(PR_SET_PDEATHSIG, SIGCHLD); // Optional: ensure SIGCHLD is sent

        // 4. Execute the user function
        try {
            func();
            _exit(EXIT_SUCCESS); // Use _exit in child after fork
        } catch (const std::exception& e) {
            std::cerr << "Child process [" << getpid() << "] error: " << e.what() << std::endl;
            _exit(EXIT_FAILURE);
        } catch (...) {
  
