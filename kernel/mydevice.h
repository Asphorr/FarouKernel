#ifndef MYDEVICE_H
#define MYDEVICE_H

#include <stddef.h>
#include <stdatomic.h>
#include <pthread.h>
#include <unistd.h>

#define MY_IOCTL_GET_BUFFER_SIZE 1
#define MY_IOCTL_SET_BUFFER_SIZE 2
#define MY_IOCTL_CLEAR_BUFFER    3
#define MY_IOCTL_GET_OPEN_COUNT  4

struct my_device {
    char *buffer;
    size_t buffer_size;
    pthread_mutex_t lock;
    atomic_int open_count;
};

int my_device_init(struct my_device **dev);
void my_device_cleanup(struct my_device *dev);
int my_open(struct my_device *dev);
int my_release(struct my_device *dev);
ssize_t my_read(struct my_device *dev, char *buf, size_t count, off_t *off);
ssize_t my_write(struct my_device *dev, const char *buf, size_t count, off_t *off);
int my_ioctl(struct my_device *dev, unsigned int cmd, void *arg);

#endif // MYDEVICE_H
