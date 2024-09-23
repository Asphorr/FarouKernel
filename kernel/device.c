#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>  // Mutex for thread safety

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mikhail");
MODULE_DESCRIPTION("Character Device with Improved Memory Management");

// Constants
#define BUFFER_SIZE 1024

// Character Device Structure
typedef struct _my_device {
    struct cdev cdev;     // Character device structure
    struct class *class;  // Device class
    dev_t devno;          // Device number
    atomic_t open_count;  // Open counter
    char *buffer;         // Buffer for data storage
    struct mutex lock;    // Mutex for thread safety
} my_device_t;

// Function prototypes
int my_open(struct inode *inode, struct file *file);
int my_release(struct inode *inode, struct file *file);
ssize_t my_read(struct file *file, char __user *buf, size_t count, loff_t *offset);
ssize_t my_write(struct file *file, const char __user *buf, size_t count, loff_t *offset);
long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

// Global variable
static my_device_t *my_device;

// Initialize character device
static int __init my_init(void)
{
    int retval;
    dev_t devno;

    // Memory allocation for device
    my_device = kzalloc(sizeof(my_device_t), GFP_KERNEL);
    if (!my_device) {
        pr_err("%s: Failed to allocate device structure\n", __func__);
        return -ENOMEM;
    }

    // Memory allocation for buffer
    my_device->buffer = kzalloc(BUFFER_SIZE, GFP_KERNEL);
    if (!my_device->buffer) {
        pr_err("%s: Failed to allocate buffer\n", __func__);
        kfree(my_device);
        return -ENOMEM;
    }

    // Mutex initialization for thread safety
    mutex_init(&my_device->lock);

    // Dynamic major number allocation
    retval = alloc_chrdev_region(&devno, 0, 1, "mychar");
    if (retval < 0) {
        pr_err("%s: Failed to allocate major number\n", __func__);
        kfree(my_device->buffer);
        kfree(my_device);
        return retval;
    }

    my_device->devno = devno;

    // Initialize character device
    cdev_init(&my_device->cdev, &my_fops);
    my_device->cdev.owner = THIS_MODULE;

    // Add device to the system
    retval = cdev_add(&my_device->cdev, devno, 1);
    if (retval < 0) {
        pr_err("%s: Failed to add device\n", __func__);
        unregister_chrdev_region(devno, 1);
        kfree(my_device->buffer);
        kfree(my_device);
        return retval;
    }

    // Create class and device node
    my_device->class = class_create(THIS_MODULE, "myclass");
    if (IS_ERR(my_device->class)) {
        pr_err("%s: Failed to create device class\n", __func__);
        cdev_del(&my_device->cdev);
        unregister_chrdev_region(devno, 1);
        kfree(my_device->buffer);
        kfree(my_device);
        return PTR_ERR(my_device->class);
    }

    device_create(my_device->class, NULL, my_device->devno, NULL, "mychar");

    pr_info("%s: Device initialized successfully\n", __func__);
    return 0;
}

// Cleanup function for module removal
static void __exit my_cleanup(void)
{
    // Cleanup resources
    device_destroy(my_device->class, my_device->devno);
    class_destroy(my_device->class);
    cdev_del(&my_device->cdev);
    unregister_chrdev_region(my_device->devno, 1);

    kfree(my_device->buffer);
    kfree(my_device);

    pr_info("%s: Device removed successfully\n", __func__);
}

// File operations structure
static struct file_operations my_fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .read = my_read,
    .write = my_write,
    .unlocked_ioctl = my_ioctl,
};

// Open function
int my_open(struct inode *inode, struct file *file)
{
    // Increment open count atomically
    atomic_inc(&my_device->open_count);
    pr_debug("%s: Device opened\n", __func__);
    return 0;
}

// Release function
int my_release(struct inode *inode, struct file *file)
{
    atomic_dec(&my_device->open_count);
    pr_debug("%s: Device closed\n", __func__);
    return 0;
}

// Read function
ssize_t my_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
    ssize_t retval;
    int count;

    // Protect access with mutex
    if (mutex_lock_interruptible(&my_device->lock))
        return -ERESTARTSYS;

    count = min(len, BUFFER_SIZE - (int)*off);
    if (count <= 0) {
        mutex_unlock(&my_device->lock);
        return 0;
    }

    retval = copy_to_user(buf, my_device->buffer + *off, count);
    if (retval) {
        mutex_unlock(&my_device->lock);
        return -EFAULT;
    }

    *off += count;
    mutex_unlock(&my_device->lock);
    return count;
}

// Write function
ssize_t my_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
    ssize_t retval;
    int count;

    if (mutex_lock_interruptible(&my_device->lock))
        return -ERESTARTSYS;

    count = min(len, BUFFER_SIZE - (int)*off);
    if (count <= 0) {
        mutex_unlock(&my_device->lock);
        return 0;
    }

    retval = copy_from_user(my_device->buffer + *off, buf, count);
    if (retval) {
        mutex_unlock(&my_device->lock);
        return -EFAULT;
    }

    *off += count;
    mutex_unlock(&my_device->lock);
    return count;
}

// IOCTL function
long my_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
        // Implement IOCTL commands here
    default:
        return -ENOTTY;
    }
}

module_init(my_init);
module_exit(my_cleanup);
