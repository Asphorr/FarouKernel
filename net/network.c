#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <stdbool.h>
#include <stdarg.h>
#include <getopt.h>
#include <ctype.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/time.h>
#include <netdb.h>

/*
 * Configuration Constants
 */
#define DEFAULT_PORT 8080
#define DEFAULT_BACKLOG 10
#define DEFAULT_BUF_SIZE 4096
#define DEFAULT_THREAD_POOL_SIZE 20
#define DEFAULT_MAX_QUEUE_SIZE 100
#define DEFAULT_MAX_CONNECTIONS 1000
#define DEFAULT_CONNECTION_TIMEOUT 60  // seconds
#define DEFAULT_MAX_REQUESTS_PER_MINUTE 60
#define DEFAULT_LOG_LEVEL LOG_INFO
#define MAX_COMMAND_LINE_LENGTH 1024
#define RATE_LIMIT_WINDOW 60 // seconds
#define RECV_TIMEOUT_INTERNAL 5 // seconds for select in recv_full

// Log levels
typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_FATAL
} log_level_t;

// Client states (Simplified - mainly for potential future use, timeout handled differently now)
typedef enum {
    CLIENT_NEW,
    CLIENT_ACTIVE,
    CLIENT_CLOSING // Could be set if shutdown is initiated gracefully
} client_state_t;

// recv_full return codes
typedef enum {
    RECV_OK = 0,
    RECV_EOF = -1,      // Connection closed by peer
    RECV_TIMEOUT = -2,  // Timeout occurred
    RECV_ERROR = -3     // Other socket error
} recv_status_t;


// Global configuration
typedef struct {
    int port;
    int backlog;
    int buf_size;
    int thread_pool_size;
    int max_queue_size;
    int max_connections;
    int connection_timeout;
    int max_requests_per_minute;
    bool use_ipv6;
    log_level_t log_level;
    char *log_file;
    FILE *log_fp;
} server_config_t;

// Rate limiting data
typedef struct {
    char ip[INET6_ADDRSTRLEN];
    time_t first_request_time;
    int request_count;
} rate_limit_entry_t;

// Client connection data
typedef struct {
    int socket;
    struct sockaddr_storage address;
    char ip[INET6_ADDRSTRLEN];
    int port;
    time_t last_activity;
    client_state_t state;
    int request_count;
} client_data_t;

// Work queue item
typedef struct work_item {
    client_data_t client;
    struct work_item *next;
} work_item_t;

// Thread pool
typedef struct {
    pthread_t *threads;
    int thread_count;
    work_item_t *work_queue_head;
    work_item_t *work_queue_tail;
    int queue_size;
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_not_empty;
    pthread_cond_t queue_not_full;
    bool shutdown;
    int active_connections;
    pthread_mutex_t conn_mutex;
    rate_limit_entry_t *rate_limits;
    int rate_limit_size;
    int rate_limit_capacity;
    pthread_mutex_t rate_limit_mutex;
} thread_pool_t;

// Global variables
static server_config_t config;
static thread_pool_t thread_pool;
static int server_fd = -1;
static volatile sig_atomic_t server_running = 1;

// Function declarations
void print_usage(const char *program_name);
void parse_command_line(int argc, char *argv[]);
void init_config();
void log_message(log_level_t level, const char *format, ...);
int setup_server_socket();
int initialize_thread_pool(int thread_count);
void* worker_thread(void *arg);
// void* timeout_monitor_thread(void *arg); // Removed
void enqueue_client(client_data_t client);
int dequeue_client(client_data_t *client);
bool check_and_update_rate_limit(const char *ip);
void sanitize_input(char *buffer);
bool is_valid_command(const char *command);
void handle_client(client_data_t client);
recv_status_t recv_full(int sockfd, char *buffer, size_t len, int timeout_sec);
int send_full(int sockfd, const char *buffer, size_t len);
void cleanup_thread_pool();
void close_client_connection(client_data_t *client, bool update_count);
void signal_handler(int sig);

// --- Implementation ---

void print_usage(const char *program_name) {
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  -p, --port PORT              Port to listen on (default: %d)\n", DEFAULT_PORT);
    printf("  -b, --backlog BACKLOG        Connection backlog (default: %d)\n", DEFAULT_BACKLOG);
    printf("  -s, --buffer-size SIZE       Buffer size in bytes (default: %d)\n", DEFAULT_BUF_SIZE);
    printf("  -t, --threads COUNT          Thread pool size (default: %d)\n", DEFAULT_THREAD_POOL_SIZE);
    printf("  -q, --queue-size SIZE        Work queue size (default: %d)\n", DEFAULT_MAX_QUEUE_SIZE);
    printf("  -c, --max-connections COUNT  Maximum connections (default: %d)\n", DEFAULT_MAX_CONNECTIONS);
    printf("  -i, --timeout SECONDS        Connection inactivity timeout in seconds (default: %d)\n", DEFAULT_CONNECTION_TIMEOUT);
    printf("  -r, --rate-limit COUNT       Max requests per minute per IP (default: %d)\n", DEFAULT_MAX_REQUESTS_PER_MINUTE);
    printf("  -6, --ipv6                   Enable IPv6 support\n");
    printf("  -l, --log-level LEVEL        Log level (0=DEBUG, 1=INFO, 2=WARNING, 3=ERROR, 4=FATAL) (default: %d)\n", DEFAULT_LOG_LEVEL);
    printf("  -f, --log-file FILE          Log file path (default: stderr)\n");
    printf("  -h, --help                   Show this help message\n");
}

void parse_command_line(int argc, char *argv[]) {
    // (Keep original getopt_long logic, but add checks for atoi results if needed)
    // ... [Original parse_command_line code] ...

    // Ensure log file handling is safe
    while ((c = getopt_long(argc, argv, "p:b:s:t:q:c:i:r:6l:f:h", long_options, &option_index)) != -1) {
        switch (c) {
            // ... [Cases for p, b, s, t, q, c, i, r, 6, l] ...
             case 'p': {
                long p = strtol(optarg, NULL, 10);
                if (p <= 0 || p > 65535) {
                    fprintf(stderr, "Invalid port number '%s'. Using default: %d\n", optarg, DEFAULT_PORT);
                    config.port = DEFAULT_PORT;
                } else {
                    config.port = (int)p;
                }
                break;
            }
            // Add similar strtol checks for other numeric options (b, s, t, q, c, i, r, l)
            // ...

            case 'f':
                if (config.log_file) {
                    free(config.log_file); // Free previous if set multiple times
                }
                config.log_file = strdup(optarg);
                if (!config.log_file) {
                     perror("Failed to duplicate log file name");
                     // Decide how to handle: exit, use stderr, etc. Using stderr here.
                     config.log_fp = stderr;
                }
                break;

            case 'h':
                print_usage(argv[0]);
                exit(0);

            default:
                fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
                exit(1);
        }
    }


    // Open log file if specified (and strdup succeeded)
    if (config.log_file && config.log_fp == stderr) { // Only open if not already failed/stderr
        config.log_fp = fopen(config.log_file, "a");
        if (!config.log_fp) {
            fprintf(stderr, "Error opening log file %s: %s. Using stderr.\n",
                    config.log_file, strerror(errno));
            config.log_fp = stderr;
            // No need to free config.log_file here, cleanup happens at the end
        }
    }
}

void init_config() {
    config.port = DEFAULT_PORT;
    config.backlog = DEFAULT_BACKLOG;
    config.buf_size = DEFAULT_BUF_SIZE;
    config.thread_pool_size = DEFAULT_THREAD_POOL_SIZE;
    config.max_queue_size = DEFAULT_MAX_QUEUE_SIZE;
    config.max_connections = DEFAULT_MAX_CONNECTIONS;
    config.connection_timeout = DEFAULT_CONNECTION_TIMEOUT;
    config.max_requests_per_minute = DEFAULT_MAX_REQUESTS_PER_MINUTE;
    config.use_ipv6 = false;
    config.log_level = DEFAULT_LOG_LEVEL;
    config.log_file = NULL;
    config.log_fp = stderr;
}

void log_message(log_level_t level, const char *format, ...) {
    if (level < config.log_level) {
        return;
    }

    // Using a static mutex is okay for moderate load, but consider async logging for high performance
    static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&log_mutex);

    time_t now;
    char time_str[20];
    va_list args;

    time(&now);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));

    const char *level_str[] = {"DEBUG", "INFO", "WARNING", "ERROR", "FATAL"};

    fprintf(config.log_fp, "[%s] [%s] ", time_str, level_str[level]);

    va_start(args, format);
    vfprintf(config.log_fp, format, args);
    va_end(args);

    fprintf(config.log_fp, "\n");
    fflush(config.log_fp); // Ensure log message is written immediately

    pthread_mutex_unlock(&log_mutex);
}

int setup_server_socket() {
    int sockfd = -1; // Initialize to invalid
    struct addrinfo hints, *result = NULL, *rp = NULL;
    int yes = 1;
    char port_str[6];

    snprintf(port_str, sizeof(port_str), "%d", config.port);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = config.use_ipv6 ? AF_INET6 : AF_UNSPEC; // Allow either if not forced
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    int status = getaddrinfo(NULL, port_str, &hints, &result);
    if (status != 0) {
        log_message(LOG_ERROR, "getaddrinfo error: %s", gai_strerror(status));
        return -1;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd == -1) {
            log_message(LOG_DEBUG, "socket() failed: %s", strerror(errno));
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
            log_message(LOG_ERROR, "Failed to set SO_REUSEADDR: %s", strerror(errno));
            close(sockfd);
            sockfd = -1; // Mark as failed
            continue; // Try next address
        }

        // For IPv6, allow IPv4 connections on the same socket if possible
        if (rp->ai_family == AF_INET6) {
            int ipv6only = 0;
            if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &ipv6only, sizeof(ipv6only)) == -1) {
                // Non-fatal, some systems might not support it or default differently
                log_message(LOG_WARNING, "Failed to unset IPV6_V6ONLY (non-fatal): %s", strerror(errno));
            }
        }

        if (bind(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) {
            log_message(LOG_DEBUG, "Successfully bound to address.");
            break; // Success
        } else {
             log_message(LOG_WARNING, "bind() failed: %s", strerror(errno));
             close(sockfd);
             sockfd = -1; // Mark as failed
        }
    }

    freeaddrinfo(result); // Free the list early

    if (sockfd == -1 || rp == NULL) { // Check if loop finished without success
        log_message(LOG_ERROR, "Could not bind to any address on port %d", config.port);
        return -1;
    }

    // Set socket to non-blocking *before* listen/accept
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        log_message(LOG_ERROR, "Failed to get socket flags: %s", strerror(errno));
        close(sockfd);
        return -1;
    }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        log_message(LOG_ERROR, "Failed to set socket to non-blocking: %s", strerror(errno));
        close(sockfd);
        return -1;
    }


    if (listen(sockfd, config.backlog) == -1) {
        log_message(LOG_ERROR, "Failed to listen on socket: %s", strerror(errno));
        close(sockfd);
        return -1;
    }

    return sockfd;
}

int initialize_thread_pool(int thread_count) {
    int i;
    bool mutex_init = false, cond_init = false, conn_mutex_init = false, rate_mutex_init = false;

    thread_pool.threads = malloc(thread_count * sizeof(pthread_t));
    if (!thread_pool.threads) {
        log_message(LOG_ERROR, "Failed to allocate memory for thread pool threads: %s", strerror(errno));
        goto cleanup;
    }

    thread_pool.rate_limits = malloc(config.max_connections * sizeof(rate_limit_entry_t));
     if (!thread_pool.rate_limits) {
        log_message(LOG_ERROR, "Failed to allocate memory for rate limiting: %s", strerror(errno));
        goto cleanup;
    }

    thread_pool.thread_count = thread_count;
    thread_pool.work_queue_head = NULL;
    thread_pool.work_queue_tail = NULL;
    thread_pool.queue_size = 0;
    thread_pool.shutdown = false;
    thread_pool.active_connections = 0;
    thread_pool.rate_limit_size = 0;
    thread_pool.rate_limit_capacity = config.max_connections;


    if (pthread_mutex_init(&thread_pool.queue_mutex, NULL) != 0) {
        log_message(LOG_ERROR, "Failed to initialize queue mutex: %s", strerror(errno));
        goto cleanup;
    }
    mutex_init = true;
    if (pthread_mutex_init(&thread_pool.conn_mutex, NULL) != 0) {
        log_message(LOG_ERROR, "Failed to initialize connection mutex: %s", strerror(errno));
        goto cleanup;
    }
    conn_mutex_init = true;
     if (pthread_mutex_init(&thread_pool.rate_limit_mutex, NULL) != 0) {
        log_message(LOG_ERROR, "Failed to initialize rate limit mutex: %s", strerror(errno));
        goto cleanup;
    }
    rate_mutex_init = true;

    if (pthread_cond_init(&thread_pool.queue_not_empty, NULL) != 0 ||
        pthread_cond_init(&thread_pool.queue_not_full, NULL) != 0) {
        log_message(LOG_ERROR, "Failed to initialize condition variables: %s", strerror(errno));
        goto cleanup;
    }
    cond_init = true;

    for (i = 0; i < thread_count; i++) {
        if (pthread_create(&thread_pool.threads[i], NULL, worker_thread, NULL) != 0) {
            log_message(LOG_ERROR, "Failed to create worker thread %d: %s", i, strerror(errno));
            thread_pool.thread_count = i; // Only cleanup the threads actually created
            // Signal shutdown to stop already created threads before joining
            thread_pool.shutdown = true;
            pthread_cond_broadcast(&thread_pool.queue_not_empty); // Wake up any waiting threads
            goto cleanup_threads; // Go to specific cleanup stage
        }
    }

    log_message(LOG_INFO, "Thread pool initialized with %d threads", thread_count);
    return 0; // Success

cleanup_threads:
    // Wait for successfully created threads to exit if creation failed mid-way
    for (int j = 0; j < thread_pool.thread_count; j++) {
         pthread_join(thread_pool.threads[j], NULL);
    }

cleanup:
    // Cleanup resources allocated so far
    if (cond_init) {
        pthread_cond_destroy(&thread_pool.queue_not_empty);
        pthread_cond_destroy(&thread_pool.queue_not_full);
    }
     if (rate_mutex_init) pthread_mutex_destroy(&thread_pool.rate_limit_mutex);
    if (conn_mutex_init) pthread_mutex_destroy(&thread_pool.conn_mutex);
    if (mutex_init) pthread_mutex_destroy(&thread_pool.queue_mutex);
    free(thread_pool.rate_limits); // Safe to free NULL
    free(thread_pool.threads); // Safe to free NULL
    thread_pool.threads = NULL;
    thread_pool.rate_limits = NULL;
    return -1; // Failure
}

void* worker_thread(void *arg) {
    client_data_t client;

    while (1) {
        pthread_mutex_lock(&thread_pool.queue_mutex);

        // Use while for spurious wakeups
        while (thread_pool.queue_size == 0 && !thread_pool.shutdown) {
            pthread_cond_wait(&thread_pool.queue_not_empty, &thread_pool.queue_mutex);
        }

        // If shutdown was signaled while waiting, exit
        if (thread_pool.shutdown && thread_pool.queue_size == 0) {
            pthread_mutex_unlock(&thread_pool.queue_mutex);
            pthread_exit(NULL);
        }

        // Dequeue should always succeed if queue_size > 0
        if (dequeue_client(&client) == 0) {
             pthread_mutex_unlock(&thread_pool.queue_mutex);
             handle_client(client); // Handle the client outside the lock
        } else {
            // Should not happen if logic is correct, but handle defensively
             pthread_mutex_unlock(&thread_pool.queue_mutex);
             if (!thread_pool.shutdown) {
                 log_message(LOG_WARNING, "Worker woke up but queue was empty unexpectedly.");
             }
        }
    }

    return NULL; // Should not be reached
}

// Removed timeout_monitor_thread

void enqueue_client(client_data_t client) {
    work_item_t *item = malloc(sizeof(work_item_t));
    if (!item) {
        log_message(LOG_ERROR, "Failed to allocate memory for work item: %s", strerror(errno));
        close(client.socket); // Close the socket if we can't queue it
        // No need to decrement active_connections here, it wasn't incremented yet
        return;
    }

    item->client = client;
    item->client.last_activity = time(NULL); // Set initial activity time
    item->client.state = CLIENT_NEW;
    item->client.request_count = 0;
    item->next = NULL;

    pthread_mutex_lock(&thread_pool.queue_mutex);

    // Wait until the queue is not full or server is shutting down
    while (thread_pool.queue_size >= config.max_queue_size && !thread_pool.shutdown) {
        pthread_cond_wait(&thread_pool.queue_not_full, &thread_pool.queue_mutex);
    }

    if (thread_pool.shutdown) {
        pthread_mutex_unlock(&thread_pool.queue_mutex);
        free(item);
        close(client.socket);
        log_message(LOG_INFO, "Server shutting down, rejecting new connection from %s:%d", client.ip, client.port);
        return;
    }

    // Add item to queue
    if (thread_pool.queue_tail == NULL) { // Queue was empty
        thread_pool.work_queue_head = item;
        thread_pool.work_queue_tail = item;
    } else {
        thread_pool.work_queue_tail->next = item;
        thread_pool.work_queue_tail = item;
    }
    thread_pool.queue_size++;

    // Signal that queue is not empty
    pthread_cond_signal(&thread_pool.queue_not_empty);
    pthread_mutex_unlock(&thread_pool.queue_mutex);

    // Increment active connections *after* successfully queueing
    pthread_mutex_lock(&thread_pool.conn_mutex);
    thread_pool.active_connections++;
    int current_active = thread_pool.active_connections; // Read while holding lock
    pthread_mutex_unlock(&thread_pool.conn_mutex);
    log_message(LOG_INFO, "Connection accepted from %s:%d (active: %d)", client.ip, client.port, current_active);

}

int dequeue_client(client_data_t *client) {
    // Assumes queue_mutex is already locked
    if (thread_pool.queue_size == 0 || thread_pool.work_queue_head == NULL) {
        return -1; // Queue is empty
    }

    work_item_t *item = thread_pool.work_queue_head;
    *client = item->client;

    thread_pool.work_queue_head = item->next;
    if (thread_pool.work_queue_head == NULL) { // Queue became empty
        thread_pool.work_queue_tail = NULL;
    }
    thread_pool.queue_size--;

    // Signal that queue might not be full anymore
    pthread_cond_signal(&thread_pool.queue_not_full);

    free(item);
    return 0; // Success
}


bool check_and_update_rate_limit(const char *ip) {
    pthread_mutex_lock(&thread_pool.rate_limit_mutex);

    time_t current_time = time(NULL);
    bool allowed = true;
    int found_idx = -1;

    // --- Optional: Cleanup old entries periodically ---
    // Can be done here, e.g., every 100 checks, or in a separate task
    // For simplicity, we rely on the check logic to effectively ignore old entries.
    // ---

    // Search for existing entry and clean up expired ones inline
    int write_idx = 0;
    for (int read_idx = 0; read_idx < thread_pool.rate_limit_size; read_idx++) {
        // Check if entry is expired
        if ((current_time - thread_pool.rate_limits[read_idx].first_request_time) > RATE_LIMIT_WINDOW) {
            // Expired, skip it (don't copy to write_idx)
            continue;
        }

        // Entry is still valid, check if it's the one we're looking for
        if (found_idx == -1 && strcmp(thread_pool.rate_limits[read_idx].ip, ip) == 0) {
            found_idx = write_idx; // Found it at the current write position
            // Update count
            thread_pool.rate_limits[read_idx].request_count++;
            if (thread_pool.rate_limits[read_idx].request_count > config.max_requests_per_minute) {
                allowed = false;
                // Don't break yet, continue compacting the array
            }
        }

        // Compact the array: Copy valid entry to write_idx if needed
        if (read_idx != write_idx) {
            thread_pool.rate_limits[write_idx] = thread_pool.rate_limits[read_idx];
        }
        write_idx++;
    }
    // Update the actual size after compaction
    thread_pool.rate_limit_size = write_idx;


    // If not found and allowed so far, add a new entry if space permits
    if (allowed && found_idx == -1) {
        if (thread_pool.rate_limit_size < thread_pool.rate_limit_capacity) {
            strncpy(thread_pool.rate_limits[thread_pool.rate_limit_size].ip, ip, INET6_ADDRSTRLEN - 1);
            thread_pool.rate_limits[thread_pool.rate_limit_size].ip[INET6_ADDRSTRLEN - 1] = '\0'; // Ensure null termination
            thread_pool.rate_limits[thread_pool.rate_limit_size].first_request_time = current_time;
            thread_pool.rate_limits[thread_pool.rate_limit_size].request_count = 1;
            thread_pool.rate_limit_size++;
        } else {
            // Table is full of *active* (non-expired) entries
            log_message(LOG_WARNING, "Rate limit table full (%d entries), cannot track new IP: %s. Allowing request.",
                        thread_pool.rate_limit_capacity, ip);
            // Policy decision: Allow the request anyway when table is full.
            // Alternative: Deny the request.
        }
    }

    pthread_mutex_unlock(&thread_pool.rate_limit_mutex);
    return allowed;
}


void sanitize_input(char *buffer) {
    if (!buffer) return;

    size_t len = strlen(buffer); // Use strlen *after* potential modification by recv_full

    // Remove trailing newline/carriage return robustly
    if (len > 0 && buffer[len - 1] == '\n') buffer[--len] = '\0';
    if (len > 0 && buffer[len - 1] == '\r') buffer[--len] = '\0';


    // Truncate excessively long commands (redundant if buffer size is limit, but safe)
    if (len >= MAX_COMMAND_LINE_LENGTH) { // Use >= to account for null terminator space
        buffer[MAX_COMMAND_LINE_LENGTH - 1] = '\0';
        len = MAX_COMMAND_LINE_LENGTH - 1;
         log_message(LOG_WARNING, "Input truncated to %d bytes", MAX_COMMAND_LINE_LENGTH);
    }

    // Remove control characters and non-printable characters (except newline/cr handled above)
    for (size_t i = 0; i < len; i++) {
        // iscntrl is locale-dependent, be careful. Check ASCII range directly?
        // Or just allow printable chars + space.
        if (!isprint((unsigned char)buffer[i]) && !isspace((unsigned char)buffer[i])) {
             buffer[i] = '?'; // Replace with placeholder instead of space
        }
    }

    // Trim leading whitespace (optional, but good practice)
    size_t start = 0;
    while (start < len && isspace((unsigned char)buffer[start])) {
        start++;
    }
    if (start > 0) {
        memmove(buffer, buffer + start, len - start + 1); // +1 for null terminator
        len -= start;
    }

    // Trim trailing whitespace (already done partially by newline removal)
    while (len > 0 && isspace((unsigned char)buffer[len - 1])) {
        buffer[--len] = '\0';
    }
}

bool is_valid_command(const char *command) {
    // List of allowed commands (case-sensitive)
    const char *allowed_commands[] = {"help", "info", "exit", "quit", "echo", NULL};

    // Check for empty command after sanitization
    if (command == NULL || command[0] == '\0') {
        return false; // Treat empty input as invalid/noop
    }

    for (int i = 0; allowed_commands[i] != NULL; i++) {
        // Check if the command starts with an allowed keyword
        size_t cmd_len = strlen(allowed_commands[i]);
        if (strncmp(command, allowed_commands[i], cmd_len) == 0) {
            // If it's "echo", it needs a space after it (or be just "echo")
            if (strcmp(allowed_commands[i], "echo") == 0) {
                return true; // Allow "echo" or "echo ..."
            }
            // For other commands, ensure it's the *whole* command
            if (command[cmd_len] == '\0' || isspace((unsigned char)command[cmd_len])) {
                 return true;
            }
        }
    }

    return false;
}

// Receive data with timeout, returning status enum
recv_status_t recv_full(int sockfd, char *buffer, size_t len, int timeout_sec) {
    fd_set read_fds;
    struct timeval tv;
    size_t bytes_received = 0;
    ssize_t n;

    if (len == 0) return RECV_OK; // Nothing to receive

    // Clear buffer (important!)
    memset(buffer, 0, len);

    // Loop to handle potential partial reads or interruptions
    while (bytes_received < len - 1) { // Leave space for null terminator
        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);
        tv.tv_sec = timeout_sec;
        tv.tv_usec = 0;

        // Wait for data or timeout
        int select_result = select(sockfd + 1, &read_fds, NULL, NULL, &tv);

        if (select_result == -1) {
            if (errno == EINTR) {
                continue; // Interrupted by signal, try again
            }
            log_message(LOG_ERROR, "select() error in recv_full: %s", strerror(errno));
            return RECV_ERROR; // Other select error
        } else if (select_result == 0) {
            // Timeout occurred
            if (bytes_received > 0) {
                // Return data received so far before timeout
                 buffer[bytes_received] = '\0'; // Null terminate
                 return RECV_OK; // Indicate success, but potentially partial due to timeout
            } else {
                return RECV_TIMEOUT; // Timeout with no data received
            }
        }

        // Data is available (select_result > 0 and FD_ISSET(sockfd, &read_fds))
        n = recv(sockfd, buffer + bytes_received, len - bytes_received - 1, 0);

        if (n == -1) {
            if (errno == EINTR) {
                continue; // Interrupted, try again
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                 // This shouldn't happen if select indicated readability, but handle defensively
                 // Could indicate socket error occurred between select and recv
                 log_message(LOG_WARNING, "recv() returned EAGAIN/EWOULDBLOCK after select returned ready");
                 // Treat as timeout for simplicity, or error? Let's treat as error.
                 return RECV_ERROR;
            }
            log_message(LOG_ERROR, "recv() error: %s", strerror(errno));
            return RECV_ERROR; // Other recv error
        } else if (n == 0) {
            // Connection closed by peer
            buffer[bytes_received] = '\0'; // Null terminate what we have
            return RECV_EOF; // End of File / Connection closed
        }

        // Successfully received n bytes
        bytes_received += n;

        // Check if the received chunk contains a newline (simple line-based protocol assumed)
        // This allows returning early if a full command line is received.
        if (memchr(buffer + bytes_received - n, '\n', n) != NULL) {
            break; // Found newline, consider message complete
        }
    }

    // Ensure null termination (should already be done by memset or break condition)
    buffer[bytes_received] = '\0';
    return RECV_OK; // Success, full buffer or newline received
}


int send_full(int sockfd, const char *buffer, size_t len) {
    size_t bytes_sent = 0;
    ssize_t n;

    while (bytes_sent < len) {
        // Consider adding MSG_NOSIGNAL flag if available to prevent SIGPIPE on broken connections
        // int flags = MSG_NOSIGNAL; // Linux specific
        int flags = 0; // Portable

        n = send(sockfd, buffer + bytes_sent, len - bytes_sent, flags);

        if (n == -1) {
            if (errno == EINTR) {
                continue; // Interrupted, try again
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Socket buffer is full, need to wait. With non-blocking sockets,
                // this requires using select/poll/epoll for writability.
                // For simplicity here, we'll treat it as an error, but a robust
                // server might handle it by waiting.
                log_message(LOG_WARNING, "send() returned EAGAIN/EWOULDBLOCK, buffer full? Treating as error.");
                return -1; // Error
            }
             if (errno == EPIPE) {
                 log_message(LOG_INFO, "send() failed: Broken pipe (client closed connection).");
                 return -1; // Treat as error / disconnect
             }
            log_message(LOG_ERROR, "send() error: %s", strerror(errno));
            return -1; // Other error
        }
        // n should be > 0 if no error
        bytes_sent += n;
    }

    return bytes_sent; // Return total bytes sent (should equal len)
}

void handle_client(client_data_t client) {
    int client_fd = client.socket;
    bool client_connected = true; // Track connection status

    // Note: active_connections was incremented in enqueue_client

    // Set socket to non-blocking mode (redundant if already set in setup_server_socket, but safe)
    // This is important for recv_full's select behavior and send_full's potential EAGAIN
    int flags = fcntl(client_fd, F_GETFL, 0);
    if (flags != -1) {
        if (fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
             log_message(LOG_WARNING, "Failed to set client socket %d to non-blocking: %s", client_fd, strerror(errno));
             // Continue anyway, but send/recv might block unexpectedly
        }
    } else {
         log_message(LOG_WARNING, "Failed to get client socket %d flags: %s", client_fd, strerror(errno));
    }


    char *buffer = malloc(config.buf_size);
    if (!buffer) {
        log_message(LOG_ERROR, "Failed to allocate buffer for client %s:%d: %s", client.ip, client.port, strerror(errno));
        close_client_connection(&client, true); // Close and update count
        return;
    }

    // Check rate limit *before* sending welcome (prevents wasting resources)
    if (!check_and_update_rate_limit(client.ip)) {
        log_message(LOG_WARNING, "Rate limit exceeded for %s:%d", client.ip, client.port);
        const char *msg = "429 Too Many Requests\n"; // Use a more standard-like message
        send_full(client_fd, msg, strlen(msg)); // Best effort send
        free(buffer);
        close_client_connection(&client, true); // Close and update count
        return;
    }

    // Send welcome message
    const char *welcome = "Welcome to the Enhanced Server!\n"
                         "Type 'help' for commands.\n"
                         "Type 'exit' or 'quit' to disconnect.\n> "; // Add prompt
    if (send_full(client_fd, welcome, strlen(welcome)) < 0) {
        log_message(LOG_INFO, "Failed to send welcome message to client %s:%d (disconnected?): %s",
                    client.ip, client.port, strerror(errno));
        free(buffer);
        close_client_connection(&client, true); // Close and update count
        return;
    }
    client.last_activity = time(NULL); // Update activity after successful send


    while (server_running && client_connected) {
        // Check for inactivity timeout *before* blocking on recv
        time_t now = time(NULL);
        if ((now - client.last_activity) > config.connection_timeout) {
            log_message(LOG_INFO, "Client %s:%d timed out due to inactivity (%d seconds)",
                       client.ip, client.port, config.connection_timeout);
            const char *msg = "Timeout: Closing connection due to inactivity.\n";
            send_full(client_fd, msg, strlen(msg)); // Best effort send
            client_connected = false; // Mark for loop exit
            break;
        }

        // Use a short internal timeout for recv_full to allow periodic checks
        recv_status_t status = recv_full(client_fd, buffer, config.buf_size, RECV_TIMEOUT_INTERNAL);

        if (status == RECV_TIMEOUT) {
            // No data received in the internal timeout window, loop again to check main timeout
            continue;
        } else if (status == RECV_ERROR) {
            // Logged within recv_full
            log_message(LOG_INFO, "Network error receiving from client %s:%d", client.ip, client.port);
            client_connected = false;
            break;
        } else if (status == RECV_EOF) {
            log_message(LOG_INFO, "Client %s:%d disconnected gracefully", client.ip, client.port);
            client_connected = false;
            break;
        }

        // If status == RECV_OK (data received)
        client.last_activity = time(NULL); // Update activity time
        client.request_count++;

        // Sanitize input (removes trailing newline, control chars, trims whitespace)
        sanitize_input(buffer);

        log_message(LOG_DEBUG, "Received from %s:%d: '%s'", client.ip, client.port, buffer);

        // Handle empty input after sanitization
        if (buffer[0] == '\0') {
             const char *prompt = "> ";
             send_full(client_fd, prompt, strlen(prompt)); // Re-send prompt
             continue;
        }

        // Check for exit command
        if (strcmp(buffer, "exit") == 0 || strcmp(buffer, "quit") == 0) {
            log_message(LOG_INFO, "Client %s:%d requested disconnect", client.ip, client.port);
            const char *msg = "Goodbye!\n";
            send_full(client_fd, msg, strlen(msg));
            client_connected = false; // Mark for loop exit
            break;
        }

        // Validate command *after* checking for exit
        if (!is_valid_command(buffer)) {
            const char *error = "Error: Invalid or unknown command. Type 'help'.\n> ";
            send_full(client_fd, error, strlen(error));
            continue;
        }

        // --- Process Valid Commands ---
        char response_buf[config.buf_size + 128]; // Buffer for responses

        if (strcmp(buffer, "help") == 0) {
            const char *help_msg =
                "Available commands:\n"
                "  help          - Show this help message\n"
                "  info          - Show server and connection information\n"
                "  echo <message> - Echo back your message\n"
                "  exit or quit  - Disconnect from the server\n> ";
            send_full(client_fd, help_msg, strlen(help_msg));
        }
        else if (strcmp(buffer, "info") == 0) {
             // Safely get active connections count
             pthread_mutex_lock(&thread_pool.conn_mutex);
             int current_active = thread_pool.active_connections;
             pthread_mutex_unlock(&thread_pool.conn_mutex);

            snprintf(response_buf, sizeof(response_buf),
                     "Server Version: 2.1\n"
                     "Server Port: %d\n"
                     "Total Active Connections: %d\n"
                     "Your IP: %s\n"
                     "Your Port: %d\n"
                     "Your Requests This Session: %d\n"
                     "Connection Timeout: %d seconds\n> ",
                     config.port, current_active,
                     client.ip, client.port, client.request_count,
                     config.connection_timeout);
            send_full(client_fd, response_buf, strlen(response_buf));
        }
        else if (strncmp(buffer, "echo ", 5) == 0) {
            // Ensure the response buffer is large enough
            snprintf(response_buf, sizeof(response_buf), "Echo: %s\n> ", buffer + 5);
            send_full(client_fd, response_buf, strlen(response_buf));
        }
         else if (strcmp(buffer, "echo") == 0) { // Handle "echo" without arguments
             const char *echo_empty = "Echo: \n> ";
             send_full(client_fd, echo_empty, strlen(echo_empty));
         }
        else {
            // Should not happen due to is_valid_command, but defensive
            const char *error = "Error: Command logic not implemented?\n> ";
            send_full(client_fd, error, strlen(error));
        }
    } // end while(server_running && client_connected)

    // Cleanup for this client
    free(buffer);
    close_client_connection(&client, true); // Close socket and decrement active count
}


// Close client connection and optionally update active count
void close_client_connection(client_data_t *client, bool update_count) {
    if (client->socket >= 0) {
        // Shutdown may fail if connection already closed, ignore error
        shutdown(client->socket, SHUT_RDWR);
        if (close(client->socket) == -1) {
             log_message(LOG_WARNING, "close() failed for socket %d: %s", client->socket, strerror(errno));
        }
        client->socket = -1; // Mark as closed
    }

    if (update_count) {
        pthread_mutex_lock(&thread_pool.conn_mutex);
        if (thread_pool.active_connections > 0) { // Prevent underflow
             thread_pool.active_connections--;
        }
         int current_active = thread_pool.active_connections; // Read while holding lock
        pthread_mutex_unlock(&thread_pool.conn_mutex);
        log_message(LOG_DEBUG, "Client %s:%d connection closed (active: %d)",
                   client->ip, client->port, current_active);
    }
}


void cleanup_thread_pool() {
    log_message(LOG_INFO, "Cleaning up thread pool...");
    thread_pool.shutdown = true; // Signal threads to stop accepting new work

    // Wake up all potentially waiting threads (on queue_not_empty or queue_not_full)
    pthread_mutex_lock(&thread_pool.queue_mutex);
    pthread_cond_broadcast(&thread_pool.queue_not_empty);
    pthread_cond_broadcast(&thread_pool.queue_not_full);
    pthread_mutex_unlock(&thread_pool.queue_mutex);

    // Wait for all worker threads to finish their current task and exit
    for (int i = 0; i < thread_pool.thread_count; i++) {
        if (thread_pool.threads[i]) { // Check if thread creation succeeded
             pthread_join(thread_pool.threads[i], NULL);
        }
    }
    log_message(LOG_DEBUG, "All worker threads joined.");

    // Free remaining work items in queue (these clients were accepted but never processed)
    pthread_mutex_lock(&thread_pool.queue_mutex);
    work_item_t *curr = thread_pool.work_queue_head;
    while (curr) {
        work_item_t *temp = curr;
        curr = curr->next;
        log_message(LOG_INFO, "Closing unprocessed connection from queue: %s:%d", temp->client.ip, temp->client.port);
        // Don't decrement active_connections here, they were never fully handled
        close_client_connection(&temp->client, false);
        free(temp);
    }
    thread_pool.work_queue_head = NULL;
    thread_pool.work_queue_tail = NULL;
    thread_pool.queue_size = 0;
    pthread_mutex_unlock(&thread_pool.queue_mutex);


    // Clean up thread pool resources
    pthread_mutex_destroy(&thread_pool.queue_mutex);
    pthread_mutex_destroy(&thread_pool.conn_mutex);
    pthread_mutex_destroy(&thread_pool.rate_limit_mutex);
    pthread_cond_destroy(&thread_pool.queue_not_empty);
    pthread_cond_destroy(&thread_pool.queue_not_full);

    free(thread_pool.threads);
    free(thread_pool.rate_limits);
    thread_pool.threads = NULL;
    thread_pool.rate_limits = NULL;
    log_message(LOG_INFO, "Thread pool resources cleaned up.");
}

void signal_handler(int sig) {
    // Ensure handler is re-entrant or uses only async-signal-safe functions
    // Setting a volatile sig_atomic_t flag is safe.
    if (sig == SIGINT || sig == SIGTERM) {
        // Use write for async-signal-safety if logging here is needed (complex)
        // For simplicity, log outside the handler after flag is set.
        server_running = 0;

        // Optional: Try to wake up accept() using the self-connect trick or pipe/eventfd
        // Self-connect:
        if (server_fd >= 0) {
            struct sockaddr_in sa;
            memset(&sa, 0, sizeof(sa));
            sa.sin_family = AF_INET;
            sa.sin_port = htons(config.port); // Connect to listening port
            inet_pton(AF_INET, "127.0.0.1", &sa.sin_addr); // Connect to loopback

            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock >= 0) {
                // Set non-blocking to prevent connect from hanging? Optional.
                // connect() on loopback should be fast.
                connect(sock, (struct sockaddr *)&sa, sizeof(sa));
                // We don't care about connect success/failure, just the side effect
                close(sock);
            }
        }
    }
     // else if (sig == SIGPIPE) {
     //    // Ignore SIGPIPE, handle EPIPE errors from send/write instead
     //    // Handled by setting sa.sa_handler = SIG_IGN below
     // }
}

int main(int argc, char *argv[]) {
    init_config();
    parse_command_line(argc, argv); // Handles args and opens log file

    log_message(LOG_INFO, "Server starting...");

    // Set up signal handlers
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask); // Ensure no signals are blocked within handler
    sa.sa_flags = 0; // No SA_RESTART, let accept be interrupted

    if (sigaction(SIGINT, &sa, NULL) == -1 || sigaction(SIGTERM, &sa, NULL) == -1) {
         log_message(LOG_FATAL, "Failed to set signal handlers: %s", strerror(errno));
         return 1;
    }

    // Ignore SIGPIPE globally so errors are returned by send/write as EPIPE
    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &sa, NULL) == -1) {
         log_message(LOG_WARNING, "Failed to ignore SIGPIPE: %s", strerror(errno));
         // Non-fatal, but send errors might cause crashes if not handled carefully
    }


    server_fd = setup_server_socket();
    if (server_fd < 0) {
        // setup_server_socket already logged the error
        log_message(LOG_FATAL, "Failed to initialize server socket. Exiting.");
        return 1;
    }

    char addr_type[8] = "IPv4/Any";
    if (config.use_ipv6) {
        strcpy(addr_type, "IPv6/Any");
    }
    log_message(LOG_INFO, "Server (%s) listening on port %d", addr_type, config.port);

    if (initialize_thread_pool(config.thread_pool_size) != 0) {
        log_message(LOG_FATAL, "Failed to initialize thread pool. Exiting.");
        close(server_fd);
        return 1;
    }

    log_message(LOG_INFO, "Server started successfully. Waiting for connections...");

    // Main accept loop
    while (server_running) {
        struct sockaddr_storage client_addr;
        socklen_t client_len = sizeof(client_addr);

        // accept is blocking, but server_fd is non-blocking. This is unusual.
        // accept on a non-blocking listener returns EAGAIN/EWOULDBLOCK if no connection pending.
        // We should use select/poll/epoll here to wait for connections efficiently.
        // --- Simplified approach (busy-wait, less efficient) ---
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

        if (client_fd < 0) {
            if (!server_running) {
                // Shutdown initiated while potentially blocked/spinning in accept
                break;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No pending connections, expected for non-blocking accept.
                // Sleep briefly to avoid pegging CPU, or better: use select/poll.
                struct timespec ts = {0, 10000000}; // 10ms sleep
                nanosleep(&ts, NULL);
                continue;
            } else if (errno == EINTR) {
                // Interrupted by our signal handler, loop should terminate soon
                continue;
            } else {
                // Actual accept error
                log_message(LOG_ERROR, "Failed to accept connection: %s", strerror(errno));
                // Potentially rate-limit logging this error if it repeats rapidly
                struct timespec ts = {0, 100000000}; // 100ms sleep after error
                nanosleep(&ts, NULL);
                continue;
            }
        }

        // --- Connection Accepted ---

        // Check max connections *before* processing further
        pthread_mutex_lock(&thread_pool.conn_mutex);
        bool max_conn_reached = (thread_pool.active_connections >= config.max_connections);
        pthread_mutex_unlock(&thread_pool.conn_mutex);

        if (max_conn_reached) {
            log_message(LOG_WARNING, "Max connections (%d) reached. Rejecting new connection.", config.max_connections);
            const char *msg = "503 Service Unavailable\n";
            send(client_fd, msg, strlen(msg), 0); // Best effort send
            close(client_fd);
            continue;
        }

        // Prepare client data
        client_data_t client;
        client.socket = client_fd;
        memcpy(&client.address, &client_addr, client_len);

        // Get IP address and port as string
        if (client_addr.ss_family == AF_INET) {
            struct sockaddr_in *s = (struct sockaddr_in *)&client_addr;
            inet_ntop(AF_INET, &s->sin_addr, client.ip, sizeof(client.ip));
            client.port = ntohs(s->sin_port);
        } else if (client_addr.ss_family == AF_INET6) {
            struct sockaddr_in6 *s = (struct sockaddr_in6 *)&client_addr;
            inet_ntop(AF_INET6, &s->sin6_addr, client.ip, sizeof(client.ip));
            client.port = ntohs(s->sin6_port);
        } else {
            strncpy(client.ip, "unknown", sizeof(client.ip) - 1);
            client.ip[sizeof(client.ip) - 1] = '\0';
            client.port = 0;
            log_message(LOG_WARNING, "Accepted connection with unknown address family: %d", client_addr.ss_family);
        }

        // Enqueue the client (this also increments active_connections on success)
        enqueue_client(client);

    } // end while(server_running)

    // --- Shutdown Sequence ---
    log_message(LOG_INFO, "Shutdown signal received. Cleaning up...");

    // Close the listening socket to prevent new connections
    if (server_fd >= 0) {
        close(server_fd);
        server_fd = -1;
    }

    // Clean up the thread pool (waits for threads, closes remaining connections)
    cleanup_thread_pool();

    // Close log file if needed
    if (config.log_fp != stderr && config.log_fp != NULL) {
        fclose(config.log_fp);
        config.log_fp = NULL;
    }
    if (config.log_file) {
        free(config.log_file);
        config.log_file = NULL;
    }

    // Final log message to stderr in case log file was closed
    fprintf(stderr, "[%ld] Server shutdown complete.\n", (long)time(NULL));

    return 0;
}
