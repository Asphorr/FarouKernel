#include <iostream>
#include <string>
#include <utility>
#include <stdexcept>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <vector>
#include <concepts>

// Kernel Concept for Kernel Class
template <typename T>
concept KernelConcept = requires(T k) {
    { k.getName() } -> std::convertible_to<std::string_view>;
    { k.getVersion() } -> std::convertible_to<std::pair<int, int>>;
    { k.getReleaseDate() } -> std::convertible_to<std::string_view>;
    { k.getBuildTime() } -> std::convertible_to<std::string_view>;
    { k.getAuthor() } -> std::convertible_to<std::string_view>;
    { k.getCopyright() } -> std::convertible_to<std::string_view>;
    { k.getLicense() } -> std::convertible_to<std::string_view>;
    { k.getDescription() } -> std::convertible_to<std::string_view>;
    { k.isRunning() } -> std::same_as<bool>;
    { k.requestStop() } -> std::same_as<void>;
    { k.waitForStop() } -> std::same_as<void>;
    { k.start() } -> std::same_as<void>;
};

// Kernel Class Implementation
class Kernel final {
public:
    Kernel(std::string name, int major, int minor, std::string releaseDate, std::string buildTime,
           std::string author, std::string copyright, std::string license, std::string description)
        : m_name(std::move(name)), m_major(major), m_minor(minor), m_releaseDate(std::move(releaseDate)),
          m_buildTime(std::move(buildTime)), m_author(std::move(author)), m_copyright(std::move(copyright)),
          m_license(std::move(license)), m_description(std::move(description)) {}

    // Getters
    [[nodiscard]] std::string_view getName() const noexcept { return m_name; }
    [[nodiscard]] std::pair<int, int> getVersion() const noexcept { return {m_major, m_minor}; }
    [[nodiscard]] std::string_view getReleaseDate() const noexcept { return m_releaseDate; }
    [[nodiscard]] std::string_view getBuildTime() const noexcept { return m_buildTime; }
    [[nodiscard]] std::string_view getAuthor() const noexcept { return m_author; }
    [[nodiscard]] std::string_view getCopyright() const noexcept { return m_copyright; }
    [[nodiscard]] std::string_view getLicense() const noexcept { return m_license; }
    [[nodiscard]] std::string_view getDescription() const noexcept { return m_description; }

    // Check running status
    [[nodiscard]] bool isRunning() const noexcept { return m_running.load(std::memory_order_acquire); }

    // Request stop
    void requestStop() noexcept {
        m_running.store(false, std::memory_order_release);
        m_cv.notify_all();
    }

    // Wait for stop
    void waitForStop() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this] { return !m_running.load(std::memory_order_acquire); });
    }

    // Start the kernel
    void start() {
        if (isRunning()) {
            throw std::runtime_error("Kernel is already running");
        }

        m_running.store(true, std::memory_order_release);
        m_thread = std::thread(&Kernel::run, this);
    }

    // Join the kernel thread
    void join() {
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    // Add a task to the kernel's task queue
    void addTask(std::function<void()> task) {
        std::lock_guard<std::mutex> lock(m_taskMutex);
        m_tasks.push_back(std::move(task));
        m_taskCv.notify_one();
    }

private:
    void run() {
        while (m_running.load(std::memory_order_acquire)) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(m_taskMutex);
                m_taskCv.wait(lock, [this] { return !m_tasks.empty() || !m_running.load(std::memory_order_acquire); });
                if (!m_running.load(std::memory_order_acquire) && m_tasks.empty()) {
                    break;
                }
                task = std::move(m_tasks.front());
                m_tasks.erase(m_tasks.begin());
            }

            try {
                if (task) {
                    task();
                }
            } catch (const std::exception& e) {
                std::cerr << "Error executing task: " << e.what() << '\n';
            } catch (...) {
                std::cerr << "Unknown error occurred while executing task\n";
            }
        }

        // Notify all threads waiting for the kernel to stop
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cv.notify_all();
    }

    std::string m_name;
    int m_major;
    int m_minor;
    std::string m_releaseDate;
    std::string m_buildTime;
    std::string m_author;
    std::string m_copyright;
    std::string m_license;
    std::string m_description;
    std::atomic<bool> m_running{false};
    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_cv;

    std::mutex m_taskMutex;
    std::condition_variable m_taskCv;
    std::vector<std::function<void()>> m_tasks;
};

// Ensure Kernel conforms to KernelConcept
static_assert(KernelConcept<Kernel>, "Kernel does not conform to KernelConcept");

// Example usage of the kernel class
int main() {
    try {
        Kernel kernel(
            "MyKernel", 1, 0, "2024-06-01", "14:00:00", "Jane Doe",
            "© 2024 Jane Doe", "MIT License", "A simple multitasking kernel");

        // Start the kernel
        kernel.start();

        // Add some tasks
        kernel.addTask([] { std::cout << "Task 1 executed\n"; });
        kernel.addTask([] { std::cout << "Task 2 executed\n"; });
        kernel.addTask([] { std::cout << "Task 3 executed\n"; });

        // Request stop after a delay
        std::thread stopThread([&kernel] {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            kernel.requestStop();
        });

        // Wait for the kernel to finish
        kernel.join();
        stopThread.join();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
    }

    return 0;
}
