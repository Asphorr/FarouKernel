#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stop_token>
#include <atomic>
#include <memory>
#include <concepts>

class ThreadPool {
public:
    explicit ThreadPool(size_t threads = std::thread::hardware_concurrency())
     : stop_source(std::make_shared<std::stop_source>()) 
    {
        workers.reserve(threads);
        for(size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this](std::stop_token st) {
                worker_main(st);
            });
        }
    }

    ~ThreadPool() {
        stop_source->request_stop();
        for(auto& worker : workers) {
            if(worker.joinable()) worker.join();
        }
    }

    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<std::invoke_result_t<F, Args...>> 
    {
        using return_type = std::invoke_result_t<F, Args...>;
        
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
        std::future<return_type> res = task->get_future();
        {
            std::unique_lock lock(queue_mutex);
            if(stop_source->get_token().stop_requested())
                throw std::runtime_error("Enqueue on stopped ThreadPool");
            
            tasks.emplace([task](){ (*task)(); });
        }
        cv.notify_one();
        return res;
    }

    void wait_completion() {
        std::unique_lock lock(queue_mutex);
        completion_cv.wait(lock, [this]{
            return tasks.empty() && active_tasks == 0;
        });
    }

private:
    void worker_main(std::stop_token st) {
        while(!st.stop_requested()) {
            std::function<void()> task;
            {
                std::unique_lock lock(queue_mutex);
                cv.wait(lock, [this, st]{
                    return !tasks.empty() || st.stop_requested();
                });

                if(st.stop_requested() && tasks.empty()) 
                    return;

                task = std::move(tasks.front());
                tasks.pop();
                ++active_tasks;
            }

            try {
                task();
            } catch(...) {
                std::lock_guard lock(eptr_mutex);
                exceptions.push_back(std::current_exception());
            }

            {
                std::lock_guard lock(queue_mutex);
                if(--active_tasks == 0 && tasks.empty())
                    completion_cv.notify_all();
            }
        }
    }

    std::vector<std::jthread> workers;
    std::queue<std::function<void()>> tasks;
    std::shared_ptr<std::stop_source> stop_source;
    
    std::mutex queue_mutex;
    std::condition_variable cv;
    std::condition_variable completion_cv;
    std::atomic<int> active_tasks{0};
    
    std::mutex eptr_mutex;
    std::vector<std::exception_ptr> exceptions;
};
