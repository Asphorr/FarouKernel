#include "mydevice.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>     // For sysconf
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>      // Potentially for error reporting if needed, or remove if not

// --- Constants ---
#define DEFAULT_PAGE_SIZE 4096
#define MAX_BUFFER_SIZE_MULTIPLIER 10

// --- Helper Macros ---
#define min(a, b) (((a) < (b)) ? (a) : (b))

// --- Function Implementations ---

/**
 * @brief Initializes a new my_device structure.
 *
 * Allocates memory for the device structure and its internal buffer.
 * Initializes mutex and atomic counter.
 *
 * @param dev Pointer to a pointer where the new device structure will be stored.
 * @return 0 on success, negative errno on failure.
 */
int my_device_init(struct my_device **dev) {
    struct my_device *new_dev = NULL;
    long page_size_val = -1;
    size_t buffer_size_to_use = 0;
    int ret = 0;

    // 1. Allocate device structure
    new_dev = malloc(sizeof(struct my_device));
    if (!new_dev) {
        ret = -ENOMEM;
        goto error_out;
    }
    // Initialize fields to known state (optional but good practice)
    new_dev->buffer = NULL;
    new_dev->buffer_size = 0;
    // atomic_init(&new_dev->open_count, 0); // Initialize later after mutex
    
    // 2. Determine buffer size
    page_size_val = sysconf(_SC_PAGESIZE);
    buffer_size_to_use = (page_size_val > 0) ? (size_t)page_size_val : DEFAULT_PAGE_SIZE;
    new_dev->buffer_size = buffer_size_to_use;

    // 3. Allocate buffer
    new_dev->buffer = malloc(new_dev->buffer_size);
    if (!new_dev->buffer) {
        ret = -ENOMEM;
        goto error_free_dev;
    }
    memset(new_dev->buffer, 0, new_dev->buffer_size);

    // 4. Initialize mutex
    ret = pthread_mutex_init(&new_dev->lock, NULL);
    if (ret != 0) {
        // pthread_mutex_init returns the error number directly
        ret = -ret; // Convert to negative errno
        goto error_free_buffer;
    }

    // 5. Initialize atomic counter
    atomic_init(&new_dev->open_count, 0);

    // Success
    *dev = new_dev;
    return 0;

// --- Error Handling Cleanup ---
error_free_buffer:
    free(new_dev->buffer);
    new_dev->buffer = NULL; // Avoid double free if cleanup is called later

error_free_dev:
    free(new_dev);
    new_dev = NULL;

error_out:
    *dev = NULL; // Ensure caller gets NULL on failure
    return ret;
}

/**
 * @brief Cleans up and frees a my_device structure.
 *
 * Destroys mutex, frees buffer, and frees the device structure.
 * Safe to call with NULL.
 *
 * @param dev Pointer to the device structure to clean up.
 */
void my_device_cleanup(struct my_device *dev) {
    if (dev) {
        // No need to check return value, best effort cleanup
        pthread_mutex_destroy(&dev->lock); 
        free(dev->buffer);
        free(dev);
    }
}

/**
 * @brief Increments the open count for the device.
 *
 * @param dev Pointer to the device structure.
 * @return 0 on success, -EINVAL if dev is NULL.
 */
int my_open(struct my_device *dev) {
    if (!dev) {
        return -EINVAL;
    }
    // Atomically increment open count
    atomic_fetch_add(&dev->open_count, 1);
    return 0;
}

/**
 * @brief Decrements the open count for the device.
 *
 * @param dev Pointer to the device structure.
 * @return 0 on success, -EINVAL if dev is NULL.
 */
int my_release(struct my_device *dev) {
    if (!dev) {
        return -EINVAL;
    }
    // Atomically decrement open count
    atomic_fetch_sub(&dev->open_count, 1);
    // Consider adding a check: if (atomic_load(&dev->open_count) < 0) { /* error? */ }
    // For basic refactoring, keep it simple.
    return 0;
}

/**
 * @brief Reads data from the device buffer.
 *
 * Reads up to 'count' bytes from the device buffer starting at 'off'
 * into 'buf'. Updates 'off' to reflect the number of bytes read.
 *
 * @param dev Pointer to the device structure.
 * @param buf Buffer to read data into.
 * @param count Maximum number of bytes to read.
 * @param off Pointer to the offset in the device buffer to start reading from.
 *            This value is updated on successful read.
 * @return Number of bytes read, 0 on EOF or invalid offset, negative errno on error.
 */
ssize_t my_read(struct my_device *dev, char *buf, size_t count, off_t *off) {
    ssize_t bytes_read = 0;

    if (!dev || !buf || !off) {
        return -EINVAL;
    }

    pthread_mutex_lock(&dev->lock);

    // Check offset validity (after locking)
    // A negative offset is invalid for this simple buffer model
    if (*off < 0 || *off >= dev->buffer_size) {
        // Return 0 for EOF or invalid offset (consistent with reaching end)
        bytes_read = 0;
        goto unlock_and_return;
    }

    // Calculate how much data is available to read from the offset
    size_t available = dev->buffer_size - *off;
    size_t read_count = min(count, available);

    if (read_count > 0) {
        memcpy(buf, dev->buffer + *off, read_count);
        *off += read_count;
        bytes_read = read_count;
    } else {
        // Nothing to read (count was 0 or offset was at/past end)
        bytes_read = 0;
    }

unlock_and_return:
    pthread_mutex_unlock(&dev->lock);
    return bytes_read;
}

/**
 * @brief Writes data to the device buffer.
 *
 * Writes up to 'count' bytes from 'buf' to the device buffer starting at 'off'.
 * Updates 'off' to reflect the number of bytes written.
 *
 * @param dev Pointer to the device structure.
 * @param buf Buffer containing data to write.
 * @param count Maximum number of bytes to write.
 * @param off Pointer to the offset in the device buffer to start writing to.
 *            This value is updated on successful write.
 * @return Number of bytes written, 0 if offset is at/beyond buffer end, negative errno on error.
 */
ssize_t my_write(struct my_device *dev, const char *buf, size_t count, off_t *off) {
    ssize_t bytes_written = 0;

    if (!dev || !buf || !off) {
        return -EINVAL;
    }

    pthread_mutex_lock(&dev->lock);

    // Check offset validity (after locking)
    // A negative offset is invalid for this simple buffer model
    if (*off < 0 || *off >= dev->buffer_size) {
        // Return 0: Cannot write past end of buffer or at invalid offset
        bytes_written = 0;
        goto unlock_and_return;
    }

    // Calculate how much space is available to write from the offset
    size_t available = dev->buffer_size - *off;
    size_t write_count = min(count, available);

    if (write_count > 0) {
        memcpy(dev->buffer + *off, buf, write_count);
        *off += write_count;
        bytes_written = write_count;
    } else {
         // Nothing to write (count was 0 or offset was at/past end)
        bytes_written = 0;
    }


unlock_and_return:
    pthread_mutex_unlock(&dev->lock);
    return bytes_written;
}

/**
 * @brief Handles IOCTL commands for the device.
 *
 * Supports getting/setting buffer size, clearing buffer, getting open count.
 *
 * @param dev Pointer to the device structure.
 * @param cmd IOCTL command code.
 * @param arg Argument for the IOCTL command (type depends on cmd).
 * @return 0 on success, negative errno on failure.
 */
int my_ioctl(struct my_device *dev, unsigned int cmd, void *arg) {
    // Basic validation
    if (!dev) {
        return -EINVAL;
    }
     // arg validation happens within switch for commands that need it

    int ret = 0;

    switch (cmd) {
        case MY_IOCTL_GET_BUFFER_SIZE:
            if (!arg) return -EINVAL;
            *(size_t*)arg = dev->buffer_size;
            break;

        case MY_IOCTL_SET_BUFFER_SIZE: {
            if (!arg) return -EINVAL;

            size_t new_size = *(size_t*)arg;
            long page_size_val = -1;
            size_t page_size_to_use = 0;
            size_t max_size = 0;
            char *new_buf = NULL;
            
            // --- Parameter Validation ---
            if (new_size == 0) {
                return -EINVAL; // Cannot set buffer size to 0
            }

            page_size_val = sysconf(_SC_PAGESIZE);
            page_size_to_use = (page_size_val > 0) ? (size_t)page_size_val : DEFAULT_PAGE_SIZE;
            max_size = MAX_BUFFER_SIZE_MULTIPLIER * page_size_to_use;

            if (new_size > max_size) {
                // Requested size exceeds maximum allowed
                return -EINVAL;
            }

            // --- Optional: Check if device is busy ---
            // If you want to prevent resizing while device is open:
            /*
            if (atomic_load(&dev->open_count) > 0) {
                return -EBUSY; // Device is in use
            }
            */

            // --- Resize Logic ---
            pthread_mutex_lock(&dev->lock);

            // Check if size is actually changing
            if (new_size == dev->buffer_size) {
                // No change needed
                ret = 0;
                goto ioctl_set_unlock_and_return;
            }

            new_buf = malloc(new_size);
            if (!new_buf) {
                ret = -ENOMEM;
                 // *** CRITICAL FIX: Unlock before returning on error ***
                goto ioctl_set_unlock_and_return;
            }
             // Clear new buffer for consistency
            memset(new_buf, 0, new_size);

            // NOTE: Data is NOT preserved in this resize, matching original behavior.
            // If data preservation is needed, use realloc or memcpy before freeing.

            // Free old buffer and assign new one
            free(dev->buffer);
            dev->buffer = new_buf;
            dev->buffer_size = new_size;
            
            // new_buf now belongs to dev, don't free it here

            ret = 0; // Success

        ioctl_set_unlock_and_return:
            pthread_mutex_unlock(&dev->lock);
            return ret;
        } // end case MY_IOCTL_SET_BUFFER_SIZE

        case MY_IOCTL_CLEAR_BUFFER:
            // No argument needed for this command
            pthread_mutex_lock(&dev->lock);
            memset(dev->buffer, 0, dev->buffer_size);
            pthread_mutex_unlock(&dev->lock);
            break;

        case MY_IOCTL_GET_OPEN_COUNT:
            if (!arg) return -EINVAL;
            // Get current open count atomically
            *(int*)arg = atomic_load(&dev->open_count);
            break;

        default:
            // Unknown IOCTL command
            ret = -ENOTTY; // Standard error for invalid ioctl command
            break;
    }
    
    return ret; // Return 0 for successfully handled commands, or error code
}
