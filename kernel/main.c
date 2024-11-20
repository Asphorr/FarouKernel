#include <iostream>
#include <string>
#include <utility>
#include <stdexcept>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <queue>
#include <functional>
#include <concepts>

// Kernel Concept defining required interface
template <typename T>
concept KernelInterface = requires(T k) {
    { k.getName() } -> std::convertible_to<std::string_view>;
    { k.getVersion() } -> std::convertible_to<std::pair<int, int>>;
    { k.getReleaseDate() } -> std::convertible_to<std::string_view>;
    { k.getBuildTime() } -> std::convertible_to<std::string_view>;
    { k.getAuthor() } -> std::convertible_to<std::string_view>;
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
           std::string author, std::string license, std::string description)
        : name_(std::move(name)), majorVersion_(major), minorVersion_(minor),
          releaseDate_(std::move(releaseDate)), buildTime_(std::move(buildTime)),
          author_(std::move(author)), license_(std::move(license)), 
          description_(std::move(description)) {}

    // Getters
    [[nodiscard]] std::string_view getName() const noexcept { return name_; }
    [[nodiscard]] std::pair<int, int> getVersion() const noexcept { return {majorVersion_, minorVersion_}; }
    [[nodiscard]] std::string_view getReleaseDate() const noexcept { return releaseDate_; }
    [[nodiscard]] std::string_view getBuildTime() const noexcept { return buildTime_; }
    [[nodiscard]] std::string_view getAuthor() const noexcept { return author_; }
    [[nodiscard]] std::string_view getLicense() const noexcept { return license_; }
    [[nodiscard]] std::string_view getDescription() const noexcept { return description_; }

    // Running state
    [[nodiscard]] bool isRunning() const noexcept { return running_.load(std::memory_order_acquire); }

    // Stop request handling
    void requestStop() noexcept {
        running_.store(false, std::memory_order_release);
        taskCv_.notify_all(); // Wake up anyone waiting
    }

    void waitForStop() {
        std::unique_lock<std::mutex> lock(stateMutex_);
        condition_.wait(lock, [this] { return !running_.load(std::memory_order_acquire); });
    }

    void start() {
        if (isRunning()) {
            throw std::runtime_error("Kernel is already running");
        }
        running_.store(true, std::memory_order_release);
        workerThread_ = std::thread(&Kernel::processTasks, this);
    }

    void join() {
        if (workerThread_.joinable()) {
            workerThread_.join();
        }
    }

    void addTask(std::function<void()> task) {
        std::lock_guard<std::mutex> lock(taskMutex_);
        taskQueue_.emplace(std::move(task));
        taskCv_.notify_one();
    }

private:
    void processTasks() {
        while (running_.load(std::memory_order_acquire)) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(taskMutex_);
                taskCv_.wait(lock, [this] { return !taskQueue_.empty() || !running_.load(std::memory_order_acquire); });
                if (!running_.load(std::memory_order_acquire) && taskQueue_.empty()) {
                    break;
                }
                task = std::move(taskQueue_.front());
                taskQueue_.pop();
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

        std::lock_guard<std::mutex> lock(stateMutex_);
        condition_.notify_all();
    }

    std::string name_;
    int majorVersion_;
    int minorVersion_;
    std::string releaseDate_;
    std::string buildTime_;
    std::string author_;
    std::string license_;
    std::string description_;
    std::atomic<bool> running_{false};
    std::thread workerThread_;
    std::mutex stateMutex_;
    std::condition_variable condition_;

    std::mutex taskMutex_;
    std::condition_variable taskCv_;
    std::queue<std::function<void()>> taskQueue_;
};

// Ensure Kernel conforms to KernelInterface
static_assert(KernelInterface<Kernel>, "Kernel does not conform to KernelInterface");

// Example usage of the kernel class
int main() {
    try {
        Kernel kernel(
            "MyKernel", 1, 0, "2024-06-01", "14:00:00", "Jane Doe",
            "MIT License", "A simple multitasking kernel");

        // Start the kernel
        kernel.start();

        // Add some tasks
        kernel.addTask([] { std::cout << "Executing Task 1\n"; });
        kernel.addTask([] { std::cout << "Executing Task 2\n"; });
        kernel.addTask([] { std::cout << "Executing Task 3\n"; });

        // Request stop after a delay using a separate thread
        std::thread stopThread([&kernel] {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            kernel.requestStop();
        });

        // Ensure proper termination
        kernel.join();
        stopThread.join();
    } catch (const std::exception& e) {
        std::cerr << "Runtime Error: " << e.what() << '\n';
    }

    return 0;
}