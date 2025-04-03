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

// Log levels
typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR,
    LOG_FATAL
} log_level_t;

// Client states
typedef enum {
    CLIENT_NEW,
    CLIENT_ACTIVE,
    CLIENT_CLOSING
} client_state_t;

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
void* timeout_monitor_thread(void *arg);
void enqueue_client(client_data_t client);
int dequeue_client(client_data_t *client);
bool check_and_update_rate_limit(const char *ip);
void sanitize_input(char *buffer);
bool is_valid_command(const char *command);
void handle_client(client_data_t client);
int recv_full(int sockfd, char *buffer, size_t len, int timeout);
int send_full(int sockfd, const char *buffer, size_t len);
void cleanup_thread_pool();
void close_client_connection(client_data_t *client);
void signal_handler(int sig);

// Print program usage
void print_usage(const char *program_name) {
    printf("Usage: %s [options]\n", program_name);
    printf("Options:\n");
    printf("  -p, --port PORT              Port to listen on (default: %d)\n", DEFAULT_PORT);
    printf("  -b, --backlog BACKLOG        Connection backlog (default: %d)\n", DEFAULT_BACKLOG);
    printf("  -s, --buffer-size SIZE       Buffer size in bytes (default: %d)\n", DEFAULT_BUF_SIZE);
    printf("  -t, --threads COUNT          Thread pool size (default: %d)\n", DEFAULT_THREAD_POOL_SIZE);
    printf("  -q, --queue-size SIZE        Work queue size (default: %d)\n", DEFAULT_MAX_QUEUE_SIZE);
    printf("  -c, --max-connections COUNT  Maximum connections (default: %d)\n", DEFAULT_MAX_CONNECTIONS);
    printf("  -i, --timeout SECONDS        Connection timeout in seconds (default: %d)\n", DEFAULT_CONNECTION_TIMEOUT);
    printf("  -r, --rate-limit COUNT       Max requests per minute per IP (default: %d)\n", DEFAULT_MAX_REQUESTS_PER_MINUTE);
    printf("  -6, --ipv6                   Enable IPv6 support\n");
    printf("  -l, --log-level LEVEL        Log level (0=DEBUG, 1=INFO, 2=WARNING, 3=ERROR, 4=FATAL) (default: 1)\n");
    printf("  -f, --log-file FILE          Log file path (default: stderr)\n");
    printf("  -h, --help                   Show this help message\n");
}

// Parse command line arguments
void parse_command_line(int argc, char *argv[]) {
    static struct option long_options[] = {
        {"port", required_argument, 0, 'p'},
        {"backlog", required_argument, 0, 'b'},
        {"buffer-size", required_argument, 0, 's'},
        {"threads", required_argument, 0, 't'},
        {"queue-size", required_argument, 0, 'q'},
        {"max-connections", required_argument, 0, 'c'},
        {"timeout", required_argument, 0, 'i'},
        {"rate-limit", required_argument, 0, 'r'},
        {"ipv6", no_argument, 0, '6'},
        {"log-level", required_argument, 0, 'l'},
        {"log-file", required_argument, 0, 'f'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int c;
    
    while ((c = getopt_long(argc, argv, "p:b:s:t:q:c:i:r:6l:f:h", long_options, &option_index)) != -1) {
        switch (c) {
            case 'p':
                config.port = atoi(optarg);
                if (config.port <= 0 || config.port > 65535) {
                    fprintf(stderr, "Invalid port number. Using default: %d\n", DEFAULT_PORT);
                    config.port = DEFAULT_PORT;
                }
                break;
                
            case 'b':
                config.backlog = atoi(optarg);
                if (config.backlog <= 0) {
                    fprintf(stderr, "Invalid backlog. Using default: %d\n", DEFAULT_BACKLOG);
                    config.backlog = DEFAULT_BACKLOG;
                }
                break;
                
            case 's':
                config.buf_size = atoi(optarg);
                if (config.buf_size <= 0) {
                    fprintf(stderr, "Invalid buffer size. Using default: %d\n", DEFAULT_BUF_SIZE);
                    config.buf_size = DEFAULT_BUF_SIZE;
                }
                break;
                
            case 't':
                config.thread_pool_size = atoi(optarg);
                if (config.thread_pool_size <= 0) {
                    fprintf(stderr, "Invalid thread pool size. Using default: %d\n", DEFAULT_THREAD_POOL_SIZE);
                    config.thread_pool_size = DEFAULT_THREAD_POOL_SIZE;
                }
                break;
                
            case 'q':
                config.max_queue_size = atoi(optarg);
                if (config.max_queue_size <= 0) {
                    fprintf(stderr, "Invalid queue size. Using default: %d\n", DEFAULT_MAX_QUEUE_SIZE);
                    config.max_queue_size = DEFAULT_MAX_QUEUE_SIZE;
                }
                break;
                
            case 'c':
                config.max_connections = atoi(optarg);
                if (config.max_connections <= 0) {
                    fprintf(stderr, "Invalid max connections. Using default: %d\n", DEFAULT_MAX_CONNECTIONS);
                    config.max_connections = DEFAULT_MAX_CONNECTIONS;
                }
                break;
                
            case 'i':
                config.connection_timeout = atoi(optarg);
                if (config.connection_timeout <= 0) {
                    fprintf(stderr, "Invalid timeout. Using default: %d\n", DEFAULT_CONNECTION_TIMEOUT);
                    config.connection_timeout = DEFAULT_CONNECTION_TIMEOUT;
                }
                break;
                
            case 'r':
                config.max_requests_per_minute = atoi(optarg);
                if (config.max_requests_per_minute <= 0) {
                    fprintf(stderr, "Invalid rate limit. Using default: %d\n", DEFAULT_MAX_REQUESTS_PER_MINUTE);
                    config.max_requests_per_minute = DEFAULT_MAX_REQUESTS_PER_MINUTE;
                }
                break;
                
            case '6':
                config.use_ipv6 = true;
                break;
                
            case 'l':
                config.log_level = atoi(optarg);
                if (config.log_level < LOG_DEBUG || config.log_level > LOG_FATAL) {
                    fprintf(stderr, "Invalid log level. Using default: %d\n", DEFAULT_LOG_LEVEL);
                    config.log_level = DEFAULT_LOG_LEVEL;
                }
                break;
                
            case 'f':
                if (config.log_file) {
                    free(config.log_file);
                }
                config.log_file = strdup(optarg);
                break;
                
            case 'h':
                print_usage(argv[0]);
                exit(0);
                
            default:
                fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
                exit(1);
        }
    }

    // Open log file if specified
    if (config.log_file) {
        config.log_fp = fopen(config.log_file, "a");
        if (!config.log_fp) {
            fprintf(stderr, "Error opening log file %s: %s. Using stderr.\n", 
                    config.log_file, strerror(errno));
            config.log_fp = stderr;
        }
    }
}

// Initialize default configuration
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

// Thread-safe logging with timestamp and log level
void log_message(log_level_t level, const char *format, ...) {
    if (level < config.log_level) {
        return;
    }

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
    fflush(config.log_fp);

    pthread_mutex_unlock(&log_mutex);
}

// Set up and initialize the server socket
int setup_server_socket() {
    int sockfd;
    struct addrinfo hints, *result, *rp;
    int yes = 1;
    char port_str[6];
    
    // Convert port to string
    snprintf(port_str, sizeof(port_str), "%d", config.port);
    
    // Set up address hints
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = config.use_ipv6 ? AF_INET6 : AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // For wildcard IP address
    
    // Get address info
    int status = getaddrinfo(NULL, port_str, &hints, &result);
    if (status != 0) {
        log_message(LOG_ERROR, "getaddrinfo error: %s", gai_strerror(status));
        return -1;
    }
    
    // Try each address until we successfully bind
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sockfd == -1) {
            continue;
        }
        
        // Set socket options
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
            log_message(LOG_ERROR, "Failed to set SO_REUSEADDR: %s", strerror(errno));
            close(sockfd);
            freeaddrinfo(result);
            return -1;
        }
        
        // For IPv6, allow IPv4 connections on the same socket
        if (rp->ai_family == AF_INET6) {
            int ipv6only = 0;
            if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &ipv6only, sizeof(ipv6only)) == -1) {
                log_message(LOG_WARNING, "Failed to set IPV6_V6ONLY: %s", strerror(errno));
                // Not fatal, continue
            }
        }
        
        // Bind socket to address
        if (bind(sockfd, rp->ai_addr, rp->ai_addrlen) == 0) {
            // Binding succeeded
            break;
        }
        
        close(sockfd);
    }
    
    freeaddrinfo(result);
    
    if (rp == NULL) {
        log_message(LOG_ERROR, "Could not bind to any address");
        return -1;
    }
    
    // Set socket to non-blocking mode
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        log_message(LOG_ERROR, "Failed to get socket flags: %s", strerror(errno));
        close(sockfd);
        return -1;
    }
    
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
        log_message(LOG_ERROR, "Failed to set socket to non-blocking mode: %s", strerror(errno));
        close(sockfd);
        return -1;
    }
    
    // Start listening for connections
    if (listen(sockfd, config.backlog) == -1) {
        log_message(LOG_ERROR, "Failed to listen on socket: %s", strerror(errno));
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

// Initialize thread pool
int initialize_thread_pool(int thread_count) {
    // Allocate thread array
    thread_pool.threads = malloc(thread_count * sizeof(pthread_t));
    if (!thread_pool.threads) {
        log_message(LOG_ERROR, "Failed to allocate memory for thread pool");
        return -1;
    }

    thread_pool.thread_count = thread_count;
    thread_pool.work_queue_head = NULL;
    thread_pool.work_queue_tail = NULL;
    thread_pool.queue_size = 0;
    thread_pool.shutdown = false;
    thread_pool.active_connections = 0;
    
    // Allocate memory for rate limiting
    thread_pool.rate_limits = malloc(config.max_connections * sizeof(rate_limit_entry_t));
    if (!thread_pool.rate_limits) {
        log_message(LOG_ERROR, "Failed to allocate memory for rate limiting");
        free(thread_pool.threads);
        return -1;
    }
    
    thread_pool.rate_limit_size = 0;
    thread_pool.rate_limit_capacity = config.max_connections;

    // Initialize synchronization primitives
    if (pthread_mutex_init(&thread_pool.queue_mutex, NULL) != 0 ||
        pthread_mutex_init(&thread_pool.conn_mutex, NULL) != 0 ||
        pthread_mutex_init(&thread_pool.rate_limit_mutex, NULL) != 0 ||
        pthread_cond_init(&thread_pool.queue_not_empty, NULL) != 0 ||
        pthread_cond_init(&thread_pool.queue_not_full, NULL) != 0) {
        
        log_message(LOG_ERROR, "Failed to initialize thread pool synchronization");
        free(thread_pool.rate_limits);
        free(thread_pool.threads);
        return -1;
    }

    // Create worker threads
    for (int i = 0; i < thread_count; i++) {
        if (pthread_create(&thread_pool.threads[i], NULL, worker_thread, NULL) != 0) {
            log_message(LOG_ERROR, "Failed to create worker thread %d", i);
            thread_pool.thread_count = i; // Only cleanup the threads created
            cleanup_thread_pool();
            return -1;
        }
    }
    
    // Create connection timeout monitor thread
    pthread_t timeout_thread;
    if (pthread_create(&timeout_thread, NULL, timeout_monitor_thread, NULL) != 0) {
        log_message(LOG_ERROR, "Failed to create timeout monitor thread");
        // Not fatal, continue without timeout monitoring
    } else {
        pthread_detach(timeout_thread);
    }

    return 0;
}

// Worker thread function
void* worker_thread(void *arg) {
    client_data_t client;
    
    while (1) {
        // Get client from queue
        pthread_mutex_lock(&thread_pool.queue_mutex);
        
        while (thread_pool.queue_size == 0 && !thread_pool.shutdown) {
            pthread_cond_wait(&thread_pool.queue_not_empty, &thread_pool.queue_mutex);
        }
        
        if (thread_pool.shutdown) {
            pthread_mutex_unlock(&thread_pool.queue_mutex);
            pthread_exit(NULL);
        }
        
        int result = dequeue_client(&client);
        pthread_mutex_unlock(&thread_pool.queue_mutex);
        
        if (result == 0) {
            handle_client(client);
        }
    }

    return NULL;
}

// Monitor and close idle connections
void* timeout_monitor_thread(void *arg) {
    // Sleep interval
    struct timespec sleep_time;
    sleep_time.tv_sec = 1;
    sleep_time.tv_nsec = 0;
    
    while (!thread_pool.shutdown) {
        // Wait for 1 second
        nanosleep(&sleep_time, NULL);
        
        time_t current_time = time(NULL);
        
        // Lock the connection mutex
        pthread_mutex_lock(&thread_pool.conn_mutex);
        
        // Go through the work queue and check for timeouts
        pthread_mutex_lock(&thread_pool.queue_mutex);
        
        work_item_t *curr = thread_pool.work_queue_head;
        while (curr) {
            if (curr->client.state != CLIENT_CLOSING && 
                (current_time - curr->client.last_activity) > config.connection_timeout) {
                
                // Mark client for timeout
                curr->client.state = CLIENT_CLOSING;
                log_message(LOG_INFO, "Client %s:%d timed out after %d seconds of inactivity", 
                           curr->client.ip, curr->client.port, config.connection_timeout);
            }
            curr = curr->next;
        }
        
        pthread_mutex_unlock(&thread_pool.queue_mutex);
        pthread_mutex_unlock(&thread_pool.conn_mutex);
        
        // Clean up rate limiting entries older than 60 seconds
        pthread_mutex_lock(&thread_pool.rate_limit_mutex);
        
        for (int i = 0; i < thread_pool.rate_limit_size; i++) {
            if ((current_time - thread_pool.rate_limits[i].first_request_time) > 60) {
                // Remove this entry by replacing it with the last one
                if (i < thread_pool.rate_limit_size - 1) {
                    thread_pool.rate_limits[i] = thread_pool.rate_limits[thread_pool.rate_limit_size - 1];
                    i--; // Re-check this index next iteration
                }
                thread_pool.rate_limit_size--;
            }
        }
        
        pthread_mutex_unlock(&thread_pool.rate_limit_mutex);
    }
    
    return NULL;
}

// Add client to work queue
void enqueue_client(client_data_t client) {
    work_item_t *item = malloc(sizeof(work_item_t));
    if (!item) {
        log_message(LOG_ERROR, "Failed to allocate memory for work item");
        close(client.socket);
        return;
    }

    item->client = client;
    item->client.last_activity = time(NULL);
    item->client.state = CLIENT_NEW;
    item->client.request_count = 0;
    item->next = NULL;

    pthread_mutex_lock(&thread_pool.queue_mutex);
    
    // Wait until the queue is not full
    while (thread_pool.queue_size >= config.max_queue_size && !thread_pool.shutdown) {
        pthread_cond_wait(&thread_pool.queue_not_full, &thread_pool.queue_mutex);
    }
    
    if (thread_pool.shutdown) {
        pthread_mutex_unlock(&thread_pool.queue_mutex);
        free(item);
        close(client.socket);
        return;
    }

    // Add item to queue
    if (thread_pool.queue_size == 0) {
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
}

// Remove client from work queue
int dequeue_client(client_data_t *client) {
    if (thread_pool.queue_size == 0) {
        return -1;
    }

    work_item_t *item = thread_pool.work_queue_head;
    *client = item->client;

    thread_pool.work_queue_head = item->next;
    if (thread_pool.work_queue_head == NULL) {
        thread_pool.work_queue_tail = NULL;
    }

    thread_pool.queue_size--;
    
    // Signal that queue is not full
    pthread_cond_signal(&thread_pool.queue_not_full);
    
    free(item);
    return 0;
}

// Check and update rate limit for an IP
bool check_and_update_rate_limit(const char *ip) {
    pthread_mutex_lock(&thread_pool.rate_limit_mutex);
    
    time_t current_time = time(NULL);
    bool allowed = true;
    
    // Search for existing entry
    int i;
    for (i = 0; i < thread_pool.rate_limit_size; i++) {
        if (strcmp(thread_pool.rate_limits[i].ip, ip) == 0) {
            // Found existing entry
            if ((current_time - thread_pool.rate_limits[i].first_request_time) <= 60) {
                // Within 1 minute window
                thread_pool.rate_limits[i].request_count++;
                
                if (thread_pool.rate_limits[i].request_count > config.max_requests_per_minute) {
                    allowed = false;
                }
            } else {
                // Reset for new time window
                thread_pool.rate_limits[i].first_request_time = current_time;
                thread_pool.rate_limits[i].request_count = 1;
            }
            
            pthread_mutex_unlock(&thread_pool.rate_limit_mutex);
            return allowed;
        }
    }
    
    // No existing entry, create new one if we have space
    if (thread_pool.rate_limit_size < thread_pool.rate_limit_capacity) {
        strncpy(thread_pool.rate_limits[thread_pool.rate_limit_size].ip, ip, INET6_ADDRSTRLEN);
        thread_pool.rate_limits[thread_pool.rate_limit_size].first_request_time = current_time;
        thread_pool.rate_limits[thread_pool.rate_limit_size].request_count = 1;
        thread_pool.rate_limit_size++;
    } else {
        // No space, log a warning and allow the request
        log_message(LOG_WARNING, "Rate limit table full, can't track IP: %s", ip);
    }
    
    pthread_mutex_unlock(&thread_pool.rate_limit_mutex);
    return true;
}

// Sanitize input buffer
void sanitize_input(char *buffer) {
    if (!buffer) return;
    
    size_t len = strlen(buffer);
    
    // Truncate excessively long commands
    if (len > MAX_COMMAND_LINE_LENGTH) {
        buffer[MAX_COMMAND_LINE_LENGTH] = '\0';
        len = MAX_COMMAND_LINE_LENGTH;
    }
    
    // Remove control characters and non-printable characters
    for (size_t i = 0; i < len; i++) {
        if (iscntrl((unsigned char)buffer[i]) && buffer[i] != '\n' && buffer[i] != '\r') {
            buffer[i] = ' ';
        }
    }
    
    // Trim trailing whitespace
    while (len > 0 && isspace((unsigned char)buffer[len - 1])) {
        buffer[--len] = '\0';
    }
}

// Validate command
bool is_valid_command(const char *command) {
    // List of allowed commands
    const char *allowed_commands[] = {"help", "info", "exit", "quit", "echo", NULL};
    
    for (int i = 0; allowed_commands[i] != NULL; i++) {
        if (strncmp(command, allowed_commands[i], strlen(allowed_commands[i])) == 0) {
            return true;
        }
    }
    
    return false;
}

// Receive data with timeout
int recv_full(int sockfd, char *buffer, size_t len, int timeout) {
    fd_set read_fds;
    struct timeval tv;
    size_t bytes_received = 0;
    ssize_t n;
    
    // Clear buffer
    memset(buffer, 0, len);
    
    // Set up for select
    FD_ZERO(&read_fds);
    FD_SET(sockfd, &read_fds);
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    
    // Continue until we've received the expected bytes, error, or connection closed
    while (bytes_received < len - 1) {
        // Wait for data or timeout
        int select_result = select(sockfd + 1, &read_fds, NULL, NULL, &tv);
        
        if (select_result == -1) {
            if (errno == EINTR) {
                // Interrupted by signal, try again
                continue;
            }
            return -1;
        } else if (select_result == 0) {
            // Timeout
            if (bytes_received > 0) {
                // We already got some data, return what we have
                break;
            }
            return -2; // Special code for timeout
        }
        
        // Data is available, read it
        n = recv(sockfd, buffer + bytes_received, len - bytes_received - 1, 0);
        
        if (n == -1) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                // Interrupted or would block, try again
                continue;
            }
            return -1; // Error
        } else if (n == 0) {
            // Connection closed by client
            break;
        }
        
        bytes_received += n;
        
        // If we received a newline, consider the message complete
        if (buffer[bytes_received - 1] == '\n') {
            break;
        }
    }
    
    // Ensure null termination
    buffer[bytes_received] = '\0';
    
    return bytes_received;
}

// Send complete message, handling partial writes
int send_full(int sockfd, const char *buffer, size_t len) {
    size_t bytes_sent = 0;
    ssize_t n;
    
    while (bytes_sent < len) {
        n = send(sockfd, buffer + bytes_sent, len - bytes_sent, 0);
        
        if (n == -1) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                // Interrupted or would block, try again
                continue;
            }
            return -1; // Error
        }
        
        bytes_sent += n;
    }
    
    return bytes_sent;
}

// Handle client connection
void handle_client(client_data_t client) {
    int client_fd = client.socket;
    
    // Update active connections count
    pthread_mutex_lock(&thread_pool.conn_mutex);
    thread_pool.active_connections++;
    pthread_mutex_unlock(&thread_pool.conn_mutex);

    log_message(LOG_INFO, "Connection from %s:%d (active: %d)", 
                client.ip, client.port, thread_pool.active_connections);
    
    // Set socket to non-blocking mode
    int flags = fcntl(client_fd, F_GETFL, 0);
    if (flags != -1) {
        fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
    }

    char *buffer = malloc(config.buf_size);
    if (!buffer) {
        log_message(LOG_ERROR, "Failed to allocate buffer for client %s:%d", client.ip, client.port);
        close_client_connection(&client);
        return;
    }

    // Check rate limit
    if (!check_and_update_rate_limit(client.ip)) {
        log_message(LOG_WARNING, "Rate limit exceeded for %s:%d", client.ip, client.port);
        const char *msg = "Rate limit exceeded. Please try again later.\n";
        send_full(client_fd, msg, strlen(msg));
        free(buffer);
        close_client_connection(&client);
        return;
    }

    // Send welcome message
    const char *welcome = "Welcome to the server!\n"
                         "Type 'help' for a list of commands.\n"
                         "Type 'exit' or 'quit' to disconnect.\n";
    if (send_full(client_fd, welcome, strlen(welcome)) < 0) {
        log_message(LOG_ERROR, "Failed to send welcome message to client %s:%d: %s", 
                    client.ip, client.port, strerror(errno));
        free(buffer);
        close_client_connection(&client);
        return;
    }

    while (server_running && client.state != CLIENT_CLOSING) {
        int bytes_read = recv_full(client_fd, buffer, config.buf_size, 5); // 5 second timeout
        
        if (bytes_read == -2) {
            // Timeout, but connection still active
            continue;
        } else if (bytes_read < 0) {
            log_message(LOG_ERROR, "Error receiving data from client %s:%d: %s", 
                        client.ip, client.port, strerror(errno));
            break;
        } else if (bytes_read == 0) {
            log_message(LOG_INFO, "Client %s:%d disconnected", client.ip, client.port);
            break;
        }

        // Update last activity time
        client.last_activity = time(NULL);
        client.request_count++;
        
        // Trim trailing newline
        if (buffer[bytes_read - 1] == '\n') {
            buffer[bytes_read - 1] = '\0';
        }
        
        // Sanitize input
        sanitize_input(buffer);
        
        log_message(LOG_DEBUG, "Received from %s:%d: %s", client.ip, client.port, buffer);
        
        // Check for exit command
        if (strcmp(buffer, "exit") == 0 || strcmp(buffer, "quit") == 0) {
            log_message(LOG_INFO, "Client %s:%d requested disconnect", client.ip, client.port);
            const char *msg = "Goodbye!\n";
            send_full(client_fd, msg, strlen(msg));
            break;
        }
        
        // Validate command
        if (!is_valid_command(buffer)) {
            const char *error = "Error: Invalid command. Type 'help' for a list of commands.\n";
            send_full(client_fd, error, strlen(error));
            continue;
        }
        
        // Process commands
        if (strcmp(buffer, "help") == 0) {
            const char *help_msg = 
                "Available commands:\n"
                "  help - Show this help message\n"
                "  info - Show server information\n"
                "  echo <message> - Echo back your message\n"
                "  exit or quit - Disconnect from the server\n";
            send_full(client_fd, help_msg, strlen(help_msg));
        }
        else if (strcmp(buffer, "info") == 0) {
            char info_msg[1024];
            snprintf(info_msg, sizeof(info_msg),
                     "Server Information:\n"
                     "  Version: 2.0\n"
                     "  Port: %d\n"
                     "  Active Connections: %d\n"
                     "  Thread Pool Size: %d\n"
                     "  Client Requests: %d\n",
                     config.port, thread_pool.active_connections,
                     config.thread_pool_size, client.request_count);
            send_full(client_fd, info_msg, strlen(info_msg));
        }
        else if (strncmp(buffer, "echo ", 5) == 0) {
            char response[config.buf_size + 32];
            snprintf(response, sizeof(response), "Echo: %s\n", buffer + 5);
            send_full(client_fd, response, strlen(response));
        }
        else {
            // Unknown command (should not happen due to validation)
            const char *error = "Error: Command not implemented\n";
            send_full(client_fd, error, strlen(error));
        }
    }

    // Cleanup
    free(buffer);
    close_client_connection(&client);
}

// Close client connection and update counters
void close_client_connection(client_data_t *client) {
    if (client->socket >= 0) {
        shutdown(client->socket, SHUT_RDWR);
        close(client->socket);
        client->socket = -1;
    }
    
    // Update active connections count
    pthread_mutex_lock(&thread_pool.conn_mutex);
    thread_pool.active_connections--;
    pthread_mutex_unlock(&thread_pool.conn_mutex);
    
    log_message(LOG_DEBUG, "Client %s:%d connection closed (active: %d)", 
               client->ip, client->port, thread_pool.active_connections);
}

// Clean up thread pool resources
void cleanup_thread_pool() {
    thread_pool.shutdown = true;
    
    // Signal all threads to exit
    pthread_mutex_lock(&thread_pool.queue_mutex);
    pthread_cond_broadcast(&thread_pool.queue_not_empty);
    pthread_mutex_unlock(&thread_pool.queue_mutex);
    
    // Wait for all worker threads to exit
    for (int i = 0; i < thread_pool.thread_count; i++) {
        pthread_join(thread_pool.threads[i], NULL);
    }
    
    // Free remaining work items in queue
    work_item_t *curr = thread_pool.work_queue_head;
    while (curr) {
        work_item_t *temp = curr;
        curr = curr->next;
        if (temp->client.socket >= 0) {
            close(temp->client.socket);
        }
        free(temp);
    }
    
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
}

// Signal handler for graceful shutdown
void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        log_message(LOG_INFO, "Received signal %d, shutting down...", sig);
        server_running = 0;
        
        // Wake up the accept thread if it's blocked
        if (server_fd >= 0) {
            // Create a connection to ourselves to break out of accept()
            struct sockaddr_in sa;
            sa.sin_family = AF_INET;
            sa.sin_port = htons(config.port);
            sa.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock >= 0) {
                connect(sock, (struct sockaddr *)&sa, sizeof(sa));
                close(sock);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    // Initialize configuration with defaults
    init_config();
    
    // Parse command-line arguments
    parse_command_line(argc, argv);
    
    // Set up signal handlers
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    
    // Initialize server socket
    server_fd = setup_server_socket();
    if (server_fd < 0) {
        log_message(LOG_FATAL, "Failed to initialize server socket");
        return 1;
    }
    
    char addr_type[8] = "IPv4";
    if (config.use_ipv6) {
        strcpy(addr_type, "IPv6");
    }
    
    log_message(LOG_INFO, "Server (%s) listening on port %d", addr_type, config.port);
    
    // Initialize thread pool
    if (initialize_thread_pool(config.thread_pool_size) != 0) {
        log_message(LOG_FATAL, "Failed to initialize thread pool");
        close(server_fd);
        return 1;
    }
    
    log_message(LOG_INFO, "Thread pool initialized with %d threads", config.thread_pool_size);
    log_message(LOG_INFO, "Server started successfully");
    
    // Accept client connections
    while (server_running) {
        struct sockaddr_storage client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        
        if (!server_running) {
            // Server is shutting down
            break;
        }
        
        if (client_fd < 0) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                // Interrupted by signal or would block, try again
                continue;
            }
            
            log_message(LOG_ERROR, "Failed to accept connection: %s", strerror(errno));
            continue;
        }
        
        // Check if we've reached max connections
        pthread_mutex_lock(&thread_pool.conn_mutex);
        bool max_conn_reached = (thread_pool.active_connections >= config.max_connections);
        pthread_mutex_unlock(&thread_pool.conn_mutex);
        
        if (max_conn_reached) {
            const char *msg = "Server too busy. Please try again later.\n";
            send(client_fd, msg, strlen(msg), 0);
            close(client_fd);
            log_message(LOG_WARNING, "Connection rejected: maximum connections reached");
            continue;
        }
        
        // Set up client data
        client_data_t client;
        client.socket = client_fd;
        memcpy(&client.address, &client_addr, client_len);
        
        // Get IP address as string
        if (client_addr.ss_family == AF_INET) {
            struct sockaddr_in *s = (struct sockaddr_in *)&client_addr;
            inet_ntop(AF_INET, &s->sin_addr, client.ip, INET6_ADDRSTRLEN);
            client.port = ntohs(s->sin_port);
        } else if (client_addr.ss_family == AF_INET6) {
            struct sockaddr_in6 *s = (struct sockaddr_in6 *)&client_addr;
            inet_ntop(AF_INET6, &s->sin6_addr, client.ip, INET6_ADDRSTRLEN);
            client.port = ntohs(s->sin6_port);
        } else {
            strncpy(client.ip, "unknown", INET6_ADDRSTRLEN);
            client.port = 0;
        }
        
        // Add client to work queue
        enqueue_client(client);
    }
    
    // Cleanup
    log_message(LOG_INFO, "Server shutting down...");
    close(server_fd);
    cleanup_thread_pool();
    
    // Close log file if needed
    if (config.log_fp != stderr && config.log_fp != NULL) {
        fclose(config.log_fp);
    }
    
    if (config.log_file) {
        free(config.log_file);
    }
    
    log_message(LOG_INFO, "Server shutdown complete");
    
    return 0;
}
