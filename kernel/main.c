#include <algorithm>
#include <atomic>
#include <concepts>
#include <condition_variable>
#include <exception>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <span>
#include <stop_token>
#include <thread>
#include <type_traits>
#include <vector>

class ThreadPool {
public:
    explicit ThreadPool(size_t threads = std::thread::hardware_concurrency())
        : stop_source(std::make_shared<std::stop_source>())
    {
        if(threads == 0)
            throw std::invalid_argument("Thread count must be positive");
        
        workers.reserve(threads);
        for(size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this](std::stop_token st) {
                worker_main(st);
            });
        }
    }

    ~ThreadPool() noexcept {
        shutdown();
    }

    template<typename F, typename... Args>
    requires std::invocable<F, Args...>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<std::invoke_result_t<F, Args...>> 
    {
        using return_type = std::invoke_result_t<F, Args...>;
        
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            [func = std::forward<F>(f), ...args = std::forward<Args>(args)]() mutable {
                return std::invoke(func, args...);
            }
        );
        
        std::future<return_type> res = task->get_future();
        {
            std::unique_lock lock(queue_mutex);
            if(stop_source->stop_requested())
                throw std::runtime_error("Enqueue on stopped ThreadPool");
            
            tasks.emplace([task = std::move(task)]() { (*task)(); });
            ++total_tasks;
        }
        cv.notify_one();
        return res;
    }

    void wait_completion() {
        std::unique_lock lock(queue_mutex);
        completion_cv.wait(lock, [this]{
            return tasks.empty() && active_tasks == 0;
        });
        check_exceptions();
    }

    void shutdown() noexcept {
        if(!stop_source->stop_requested()) {
            stop_source->request_stop();
            cv.notify_all();
            for(auto& worker : workers) {
                if(worker.joinable())
                    worker.join();
            }
            check_exceptions();
        }
    }

    size_t thread_count() const noexcept {
        return workers.size();
    }

    size_t pending_tasks() const noexcept {
        std::unique_lock lock(queue_mutex);
        return tasks.size();
    }

private:
    void worker_main(std::stop_token st) noexcept {
        while(!st.stop_requested()) {
            std::optional<std::function<void()>> task;
            {
                std::unique_lock lock(queue_mutex);
                cv.wait(lock, [&] {
                    return !tasks.empty() || st.stop_requested();
                });

                if(st.stop_requested())
                    return;

                task = std::move(tasks.front());
                tasks.pop();
                ++active_tasks;
            }

            try {
                (*task)();
            } catch(...) {
                std::lock_guard lock(eptr_mutex);
                exceptions.push_back(std::current_exception());
            }

            {
                std::lock_guard lock(queue_mutex);
                --active_tasks;
                if(tasks.empty() && active_tasks == 0) {
                    completion_cv.notify_all();
                }
            }
        }
    }

    void check_exceptions() {
        std::lock_guard lock(eptr_mutex);
        if(!exceptions.empty()) {
            auto eptr = exceptions.front();
            exceptions.erase(exceptions.begin());
            std::rethrow_exception(eptr);
        }
    }

    std::vector<std::jthread> workers;
    std::queue<std::function<void()>> tasks;
    std::shared_ptr<std::stop_source> stop_source;
    
    mutable std::mutex queue_mutex;
    std::condition_variable cv;
    std::condition_variable completion_cv;
    std::atomic<size_t> active_tasks{0};
    std::atomic<size_t> total_tasks{0};
    
    std::mutex eptr_mutex;
    std::vector<std::exception_ptr> exceptions;
};
