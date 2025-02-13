#include "mydevice.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int my_device_init(struct my_device **dev) {
    struct my_device *new_dev = malloc(sizeof(struct my_device));
    if (!new_dev) return -ENOMEM;

    long page_size = sysconf(_SC_PAGESIZE);
    new_dev->buffer_size = (page_size > 0) ? page_size : 4096;
    
    new_dev->buffer = malloc(new_dev->buffer_size);
    if (!new_dev->buffer) {
        free(new_dev);
        return -ENOMEM;
    }
    memset(new_dev->buffer, 0, new_dev->buffer_size);

    pthread_mutex_init(&new_dev->lock, NULL);
    atomic_init(&new_dev->open_count, 0);

    *dev = new_dev;
    return 0;
}

void my_device_cleanup(struct my_device *dev) {
    if (dev) {
        pthread_mutex_destroy(&dev->lock);
        free(dev->buffer);
        free(dev);
    }
}

int my_open(struct my_device *dev) {
    if (!dev) return -EINVAL;
    atomic_fetch_add(&dev->open_count, 1);
    return 0;
}

int my_release(struct my_device *dev) {
    if (!dev) return -EINVAL;
    atomic_fetch_sub(&dev->open_count, 1);
    return 0;
}

ssize_t my_read(struct my_device *dev, char *buf, size_t count, off_t *off) {
    if (!dev || !buf || !off) return -EINVAL;

    pthread_mutex_lock(&dev->lock);
    if (*off >= dev->buffer_size) {
        pthread_mutex_unlock(&dev->lock);
        return 0;
    }

    size_t available = dev->buffer_size - *off;
    size_t read_count = (count < available) ? count : available;
    memcpy(buf, dev->buffer + *off, read_count);
    *off += read_count;
    pthread_mutex_unlock(&dev->lock);
    return read_count;
}

ssize_t my_write(struct my_device *dev, const char *buf, size_t count, off_t *off) {
    if (!dev || !buf || !off) return -EINVAL;

    pthread_mutex_lock(&dev->lock);
    if (*off >= dev->buffer_size) {
        pthread_mutex_unlock(&dev->lock);
        return 0;
    }

    size_t available = dev->buffer_size - *off;
    size_t write_count = (count < available) ? count : available;
    memcpy(dev->buffer + *off, buf, write_count);
    *off += write_count;
    pthread_mutex_unlock(&dev->lock);
    return write_count;
}

int my_ioctl(struct my_device *dev, unsigned int cmd, void *arg) {
    if (!dev || !arg) return -EINVAL;

    switch (cmd) {
        case MY_IOCTL_GET_BUFFER_SIZE:
            *(size_t*)arg = dev->buffer_size;
            break;

        case MY_IOCTL_SET_BUFFER_SIZE: {
            size_t new_size = *(size_t*)arg;
            long page_size = sysconf(_SC_PAGESIZE);
            size_t max_size = 10 * ((page_size > 0) ? page_size : 4096);

            if (new_size == 0 || new_size > max_size) 
                return -EINVAL;

            pthread_mutex_lock(&dev->lock);
            char *new_buf = malloc(new_size);
            if (!new_buf) {
                pthread_mutex_unlock(&dev->lock);
                return -ENOMEM;
            }
            free(dev->buffer);
            dev->buffer = new_buf;
            dev->buffer_size = new_size;
            pthread_mutex_unlock(&dev->lock);
            break;
        }

        case MY_IOCTL_CLEAR_BUFFER:
            pthread_mutex_lock(&dev->lock);
            memset(dev->buffer, 0, dev->buffer_size);
            pthread_mutex_unlock(&dev->lock);
            break;

        case MY_IOCTL_GET_OPEN_COUNT:
            *(int*)arg = atomic_load(&dev->open_count);
            break;

        default: return -ENOTTY;
    }
    return 0;
}
