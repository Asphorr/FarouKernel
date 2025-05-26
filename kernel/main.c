#include <algorithm>
#include <atomic>
#include <barrier>
#include <deque>
#include <exception>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <random>
#include <shared_mutex>
#include <stop_token>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

class ThreadPoolException : public std::runtime_error {
public:
    std::vector<std::exception_ptr> exceptions;

    explicit ThreadPoolException(std::vector<std::exception_ptr> e)
        : std::runtime_error("ThreadPool encountered exceptions"), exceptions(std::move(e)) {}
};

template <typename T>
class LockFreeStealQueue {
private:
    struct alignas(64) StealQueue {
        std::deque<T> tasks;
        mutable std::mutex mtx;
    };

    std::vector<StealQueue> queues;
    std::atomic<size_t> index{0};
    static thread_local size_t local_index;

public:
    explicit LockFreeStealQueue(size_t num_queues) : queues(num_queues) {}

    template <typename Task>
    void push(Task&& task) {
        size_t i = local_index;
        if (i == 0) i = index++ % queues.size();
        {
            std::lock_guard lock(queues[i].mtx);
            queues[i].tasks.emplace_back(std::forward<Task>(task));
        }
    }

    bool try_pop(T& task) {
        size_t i = local_index;
        if (i == 0) return false;

        {
            std::lock_guard lock(queues[i].mtx);
            if (!queues[i].tasks.empty()) {
                task = std::move(queues[i].tasks.back());
                queues[i].tasks.pop_back();
                return true;
            }
        }
        
        for (size_t j = 0; j < queues.size(); ++j) {
            if (j == i) continue;
            std::lock_guard lock(queues[j].mtx);
            if (!queues[j].tasks.empty()) {
                task = std::move(queues[j].tasks.front());
                queues[j].tasks.pop_front();
                return true;
            }
        }
        return false;
    }

    bool try_pop_bulk(std::vector<T>& result, size_t max_tasks = 10) {
        size_t i = local_index;
        if (i == 0) return false;

        {
            std::lock_guard lock(queues[i].mtx);
            auto& tasks = queues[i].tasks;
            size_t n = std::min(max_tasks, tasks.size());
            result.reserve(n);
            for (size_t j = 0; j < n; ++j) {
                result.emplace_back(std::move(tasks.back()));
                tasks.pop_back();
            }
            return !result.empty();
        }
    }

    size_t size() const {
        size_t total = 0;
        for (const auto& q : queues) {
            std::lock_guard lock(q.mtx);
            total += q.tasks.size();
        }
        return total;
    }
};

thread_local size_t LockFreeStealQueue<void()>::local_index = 0;

class DynamicThreadPool {
public:
    explicit DynamicThreadPool(
        size_t min_threads = std::thread::hardware_concurrency(),
        size_t max_threads = std::thread::hardware_concurrency() * 4
    ) : min_threads(std::max<size_t>(1, min_threads)),
        max_threads(std::max(min_threads, max_threads)),
        task_queue(std::max<size_t>(1, min_threads)),
        control_barrier(2, [this] { adjust_workers(); }) 
    {
        workers.reserve(max_threads);
        add_workers(min_threads);
        adjust_thread = std::jthread([this](std::stop_token st) {
            while (!st.stop_requested()) {
                control_barrier.arrive_and_wait();
                std::this_thread::sleep_for(100ms);
            }
        });
    }

    ~DynamicThreadPool() noexcept {
        shutdown();
    }

    template <typename F, typename... Args>
    requires std::invocable<F, Args...>
    [[nodiscard]] auto enqueue(F&& f, Args&&... args) {
        using return_type = std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            [func = std::forward<F>(f), ...args = std::forward<Args>(args)]() mutable {
                return std::invoke(func, args...);
            });

        std::future<return_type> res = task->get_future();
        {
            std::shared_lock lock(workers_mutex);
            if (stop_requested) {
                throw std::runtime_error("Enqueue on stopped ThreadPool");
            }
            task_queue.push([task] { (*task)(); });
        }
        return res;
    }

    void shutdown() noexcept {
        if (!stop_requested.exchange(true)) {
            adjust_thread.request_stop();
            control_barrier.arrive_and_drop();
            std::unique_lock lock(workers_mutex);
            for (auto& w : workers) {
                if (w.joinable()) w.join();
            }
        }
    }

private:
    struct Worker {
        std::jthread thread;
        std::vector<std::exception_ptr> exceptions;
    };

    void worker_main(std::stop_token st, size_t queue_index) {
        LockFreeStealQueue<void()>::local_index = queue_index + 1;
        std::vector<std::function<void()>> local_tasks;
        local_tasks.reserve(10);

        while (!st.stop_requested()) {
            if (task_queue.try_pop_bulk(local_tasks, 10)) {
                for (auto& task : local_tasks) {
                    try {
                        task();
                    } catch (...) {
                        std::lock_guard lock(eptr_mutex);
                        exceptions.emplace_back(std::current_exception());
                    }
                }
                local_tasks.clear();
                continue;
            }

            std::this_thread::sleep_for(1ms);
        }
    }

    void add_workers(size_t n) {
        std::unique_lock lock(workers_mutex);
        for (size_t i = 0; i < n && workers.size() < max_threads; ++i) {
            workers.emplace_back(Worker{
                std::jthread([this, idx = workers.size()](std::stop_token st) {
                    worker_main(st, idx);
                }),
                {}
            });
        }
    }

    void adjust_workers() {
        if (stop_requested) return;

        const size_t current_tasks = task_queue.size();
        const size_t current_workers = workers.size();

        if (current_tasks > current_workers * 2 && current_workers < max_threads) {
            add_workers(std::min(current_tasks/2, max_threads - current_workers));
        } else if (current_tasks < current_workers / 2 && current_workers > min_threads) {
            std::unique_lock lock(workers_mutex);
            size_t remove_num = std::min(current_workers - min_threads, current_workers - current_tasks/2);
            auto it = std::remove_if(workers.begin(), workers.end(), 
                [&, cnt=0](auto&) mutable { return ++cnt > (current_workers - remove_num); });
            for (auto i = it; i != workers.end(); ++i) {
                if (i->thread.joinable()) {
                    i->thread.request_stop();
                    i->thread.join();
                }
            }
            workers.erase(it, workers.end());
        }
    }

    const size_t min_threads;
    const size_t max_threads;
    std::atomic<bool> stop_requested{false};

    LockFreeStealQueue<std::function<void()>> task_queue;
    std::vector<Worker> workers;
    mutable std::shared_mutex workers_mutex;
    std::mutex eptr_mutex;
    std::vector<std::exception_ptr> exceptions;

    std::jthread adjust_thread;
    std::barrier<> control_barrier;
};
