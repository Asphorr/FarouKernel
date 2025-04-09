#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>

// --- Task Definition ---

typedef struct {
    void (*function)(void*); // Function to execute
    void* argument;          // Argument to the function
} ThreadPoolTask;

// --- Opaque Thread Pool Structure ---

typedef struct ThreadPool ThreadPool;

// --- Error Codes ---

typedef enum {
    THREAD_POOL_SUCCESS = 0,
    THREAD_POOL_ERROR_ALLOCATION = -1,
    THREAD_POOL_ERROR_INIT = -2,
    THREAD_POOL_ERROR_INVALID_ARG = -3,
    THREAD_POOL_ERROR_SHUTDOWN = -4,
    THREAD_POOL_ERROR_QUEUE_FULL = -5, // If using bounded queues (optional)
    THREAD_POOL_ERROR_OTHER = -10
} ThreadPoolError;

// --- Function Declarations ---

/**
 * @brief Creates a new dynamic thread pool.
 *
 * @param min_threads Minimum number of worker threads. Must be >= 1.
 * @param max_threads Maximum number of worker threads. Must be >= min_threads.
 * @param queue_capacity Initial capacity for each worker's task queue.
 * @return A pointer to the created ThreadPool, or NULL on error.
 */
ThreadPool* thread_pool_create(size_t min_threads, size_t max_threads, size_t queue_capacity);

/**
 * @brief Submits a task to the thread pool.
 *
 * The pool will attempt to assign the task to a worker thread for execution.
 *
 * @param pool The thread pool instance.
 * @param task The task to be executed.
 * @return THREAD_POOL_SUCCESS on success, or an error code otherwise.
 */
ThreadPoolError thread_pool_submit_task(ThreadPool* pool, ThreadPoolTask task);

/**
 * @brief Shuts down the thread pool and waits for all tasks to complete (graceful).
 *
 * No new tasks can be submitted after calling this. Waits for currently
 * executing and queued tasks to finish before joining threads.
 *
 * @param pool The thread pool instance.
 */
void thread_pool_shutdown_graceful(ThreadPool* pool);

/**
 * @brief Shuts down the thread pool immediately.
 *
 * No new tasks can be submitted. Signals threads to stop. Does *not* wait
 * for queued tasks to complete, only for currently running tasks to finish
 * their current iteration (if possible) and exit.
 *
 * @param pool The thread pool instance.
 */
void thread_pool_shutdown_immediate(ThreadPool* pool);


/**
 * @brief Destroys the thread pool and frees associated resources.
 *
 * Ensure thread_pool_shutdown_graceful() or thread_pool_shutdown_immediate()
 * has been called before destroying the pool. Behavior is undefined if called
 * on an active pool.
 *
 * @param pool The thread pool instance.
 */
void thread_pool_destroy(ThreadPool* pool);

/**
 * @brief Gets the approximate number of tasks currently in the queues.
 *
 * This is an estimate as the queues are constantly changing.
 *
 * @param pool The thread pool instance.
 * @return Approximate number of tasks in all queues.
 */
size_t thread_pool_get_queue_size(const ThreadPool* pool);

/**
 * @brief Gets the current number of active worker threads.
 *
 * @param pool The thread pool instance.
 * @return Number of active worker threads.
 */
size_t thread_pool_get_active_workers(const ThreadPool* pool);


#endif // THREAD_POOL_H
