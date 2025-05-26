#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>    // For pthread related functionalities used in the implementation.
                        // While the ThreadPool struct is opaque, this include hints at the underlying model.
#include <stdatomic.h>  // For atomic operations that will be necessary in the implementation.
#include <stdbool.h>    // For the 'bool' type.
#include <stddef.h>     // For 'size_t' and 'NULL'.

// --- Task Definition ---

/**
 * @brief Defines a task to be executed by a worker thread in the pool.
 */
typedef struct {
    /**
     * @brief Pointer to the function that will be executed.
     * @param argument The argument to be passed to the function.
     */
    void (*function)(void* argument);

    /**
     * @brief Argument to be passed to the task function.
     * @note The caller is responsible for managing the lifetime of this argument.
     * It must remain valid until the task has completed execution. The thread
     * pool does not make a copy of the argument data nor does it manage its memory.
     */
    void* argument;
} ThreadPoolTask;

// --- Opaque Thread Pool Structure ---

/**
 * @brief Opaque pointer to the internal thread pool structure.
 * Users interact with the thread pool via a pointer to this type,
 * without needing to know its internal details.
 */
typedef struct ThreadPool ThreadPool;

// --- Error Codes ---

/**
 * @brief Enumerates error codes that can be returned by thread pool API functions.
 */
typedef enum {
    THREAD_POOL_SUCCESS = 0,            /**< Operation completed successfully. */
    THREAD_POOL_ERROR_INVALID_ARG = -1, /**< An invalid argument was provided (e.g., NULL pool pointer where not allowed, invalid size parameters). */
    THREAD_POOL_ERROR_ALLOCATION = -2,  /**< Memory allocation failed during an operation. */
    THREAD_POOL_ERROR_INIT_FAILURE = -3,/**< Failed to initialize internal pool components (e.g., threads, mutexes, condition variables). This error is implicitly signified by `thread_pool_create` returning NULL. */
    THREAD_POOL_ERROR_QUEUE_FULL = -4,  /**< The task queue is full and cannot accept new tasks (applies to bounded queues). */
    THREAD_POOL_ERROR_POOL_SHUTDOWN = -5,/**< The operation was attempted on a pool that is currently shutting down or has already been shut down. */
    THREAD_POOL_ERROR_OTHER = -10       /**< An unspecified or internal error occurred. */
} ThreadPoolError;

// --- Function Declarations ---

/**
 * @brief Creates and initializes a new thread pool.
 *
 * This function sets up a thread pool with a specified minimum and maximum number of
 * worker threads, and a defined capacity for its task queue(s).
 * If `min_threads` is less than `max_threads`, the pool's implementation might
 * dynamically adjust the number of active threads within this range based on workload,
 * though the specifics of such scaling are implementation-defined.
 *
 * @param min_threads The minimum number of worker threads to maintain. Must be >= 1.
 * @param max_threads The maximum number of worker threads the pool can scale up to. Must be >= `min_threads`.
 * @param queue_capacity The maximum number of tasks that can be pending in the queue(s). Must be > 0.
 * This capacity might apply to a central queue or be distributed if per-thread queues are used.
 *
 * @return A pointer to the newly created `ThreadPool` instance on success.
 * @return `NULL` if an error occurred during creation (e.g., invalid arguments, memory allocation failure,
 * failure to create initial threads or synchronization primitives).
 * If `NULL` is returned, `ThreadPoolError` types like `THREAD_POOL_ERROR_ALLOCATION`,
 * `THREAD_POOL_ERROR_INVALID_ARG`, or `THREAD_POOL_ERROR_INIT_FAILURE` would be the conceptual cause.
 */
ThreadPool* thread_pool_create(size_t min_threads, size_t max_threads, size_t queue_capacity);

/**
 * @brief Submits a new task to the thread pool for asynchronous execution.
 *
 * The task is added to an internal queue, and a worker thread will execute it
 * when one becomes available. This function is designed to be thread-safe,
 * allowing multiple application threads to submit tasks concurrently.
 *
 * @param pool A non-NULL pointer to an initialized `ThreadPool` instance.
 * @param task The `ThreadPoolTask` to be executed. Note the lifetime requirements
 * for `task.argument` as described in the `ThreadPoolTask` struct.
 *
 * @return `THREAD_POOL_SUCCESS` if the task was successfully submitted.
 * @return `THREAD_POOL_ERROR_INVALID_ARG` if `pool` is `NULL`.
 * @return `THREAD_POOL_ERROR_POOL_SHUTDOWN` if the pool is shutting down or has been shut down.
 * @return `THREAD_POOL_ERROR_QUEUE_FULL` if the task queue is currently full.
 * @return `THREAD_POOL_ERROR_ALLOCATION` if an internal memory allocation failed (e.g., for a queue node).
 */
ThreadPoolError thread_pool_submit_task(ThreadPool* pool, ThreadPoolTask task);

/**
 * @brief Initiates a graceful shutdown of the thread pool.
 *
 * After this call, the pool will no longer accept new tasks.
 * It will wait for all currently queued tasks to be processed and for all
 * actively executing tasks to complete their work before joining the worker threads.
 * This function is blocking and will not return until the shutdown process is complete.
 *
 * @param pool A pointer to the `ThreadPool` instance. If `pool` is `NULL`,
 * the function has no effect. It is safe to call on an already shut down pool.
 */
void thread_pool_shutdown_graceful(ThreadPool* pool);

/**
 * @brief Initiates an immediate shutdown of the thread pool.
 *
 * After this call, the pool will no longer accept new tasks.
 * Worker threads are signaled to terminate. Tasks that are currently being executed
 * by worker threads will be allowed to complete their current execution.
 * However, any tasks remaining in the queue will be discarded and will NOT be processed.
 * This function is blocking and will not return until all active worker threads
 * have completed their current task (if any) and exited.
 *
 * @param pool A pointer to the `ThreadPool` instance. If `pool` is `NULL`,
 * the function has no effect. It is safe to call on an already shut down pool.
 */
void thread_pool_shutdown_immediate(ThreadPool* pool);

/**
 * @brief Destroys the thread pool and frees all associated resources.
 *
 * This function should ONLY be called after the pool has been successfully shut down
 * using either `thread_pool_shutdown_graceful()` or `thread_pool_shutdown_immediate()`.
 * Attempting to destroy an active or not-yet-shutdown pool results in undefined behavior.
 * After this call, the `pool` pointer becomes invalid and must not be used.
 *
 * @param pool A pointer to the `ThreadPool` instance to be destroyed.
 * If `pool` is `NULL`, the function does nothing.
 */
void thread_pool_destroy(ThreadPool* pool);

/**
 * @brief Gets the approximate number of tasks currently pending in the pool's queue(s).
 *
 * This value is an estimate because the number of tasks can change concurrently
 * due to submissions by other threads or processing by worker threads.
 *
 * @param pool A non-NULL, valid pointer to the `ThreadPool` instance.
 * @return The approximate number of tasks in the queue(s).
 * Returns 0 if `pool` is `NULL` or if the pool is not properly initialized.
 */
size_t thread_pool_get_queue_size(const ThreadPool* pool);

/**
 * @brief Gets the current number of active worker threads in the pool.
 *
 * "Active" typically means threads that have been created and are currently running,
 * potentially including idle threads waiting for tasks. This count may vary if the
 * pool supports dynamic thread scaling between its configured `min_threads` and `max_threads`.
 *
 * @param pool A non-NULL, valid pointer to the `ThreadPool` instance.
 * @return The number of active worker threads.
 * Returns 0 if `pool` is `NULL` or if the pool is not properly initialized.
 */
size_t thread_pool_get_active_workers(const ThreadPool* pool);

/**
 * @brief Gets the configured minimum number of threads for the pool.
 *
 * @param pool A non-NULL, valid pointer to the `ThreadPool` instance.
 * @return The configured minimum number of worker threads.
 * Returns 0 if `pool` is `NULL` or if the pool is not properly initialized.
 */
size_t thread_pool_get_min_threads(const ThreadPool* pool);

/**
 * @brief Gets the configured maximum number of threads for the pool.
 *
 * @param pool A non-NULL, valid pointer to the `ThreadPool` instance.
 * @return The configured maximum number of worker threads.
 * Returns 0 if `pool` is `NULL` or if the pool is not properly initialized.
 */
size_t thread_pool_get_max_threads(const ThreadPool* pool);

#endif // THREAD_POOL_H
