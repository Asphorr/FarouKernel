#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/atomic.h>
#include <linux/device.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mikhail");
MODULE_DESCRIPTION("Improved Character Device Driver");

#define BUFFER_SIZE 1024

struct my_device {
    struct cdev cdev;
    struct class *class;
    dev_t devno;
    atomic_t open_count;
    char *buffer;
    struct mutex lock;
};

static struct my_device *my_device;

static int my_open(struct inode *inode, struct file *file);
static int my_release(struct inode *inode, struct file *file);
static ssize_t my_read(struct file *file, char __user *buf, size_t count, loff_t *off);
static ssize_t my_write(struct file *file, const char __user *buf, size_t count, loff_t *off);
static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

static struct file_operations my_fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .read = my_read,
    .write = my_write,
    .unlocked_ioctl = my_ioctl,
};

static int __init my_init(void)
{
    int retval;
    dev_t devno;

    my_device = kzalloc(sizeof(*my_device), GFP_KERNEL);
    if (!my_device) {
        pr_err("Failed to allocate device structure\n");
        return -ENOMEM;
    }

    atomic_set(&my_device->open_count, 0);
    my_device->buffer = kzalloc(BUFFER_SIZE, GFP_KERNEL);
    if (!my_device->buffer) {
        pr_err("Failed to allocate buffer\n");
        kfree(my_device);
        return -ENOMEM;
    }

    mutex_init(&my_device->lock);

    retval = alloc_chrdev_region(&devno, 0, 1, "mychar");
    if (retval < 0) {
        pr_err("Failed to allocate major number\n");
        kfree(my_device->buffer);
        kfree(my_device);
        return retval;
    }
    my_device->devno = devno;

    cdev_init(&my_device->cdev, &my_fops);
    my_device->cdev.owner = THIS_MODULE;

    retval = cdev_add(&my_device->cdev, devno, 1);
    if (retval < 0) {
        pr_err("Failed to add device\n");
        unregister_chrdev_region(devno, 1);
        kfree(my_device->buffer);
        kfree(my_device);
        return retval;
    }

    my_device->class = class_create(THIS_MODULE, "myclass");
    if (IS_ERR(my_device->class)) {
        pr_err("Failed to create device class\n");
        cdev_del(&my_device->cdev);
        unregister_chrdev_region(devno, 1);
        kfree(my_device->buffer);
        kfree(my_device);
        return PTR_ERR(my_device->class);
    }

    device_create(my_device->class, NULL, devno, NULL, "mychar");

    pr_info("Device initialized successfully\n");
    return 0;
}

static void __exit my_cleanup(void)
{
    device_destroy(my_device->class, my_device->devno);
    class_destroy(my_device->class);
    cdev_del(&my_device->cdev);
    unregister_chrdev_region(my_device->devno, 1);
    kfree(my_device->buffer);
    kfree(my_device);
    pr_info("Device removed successfully\n");
}

static int my_open(struct inode *inode, struct file *file)
{
    mutex_lock(&my_device->lock);
    atomic_inc(&my_device->open_count);
    mutex_unlock(&my_device->lock);
    pr_debug("Device opened, open count: %d\n", atomic_read(&my_device->open_count));
    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    mutex_lock(&my_device->lock);
    atomic_dec(&my_device->open_count);
    mutex_unlock(&my_device->lock);
    pr_debug("Device closed, open count: %d\n", atomic_read(&my_device->open_count));
    return 0;
}

static ssize_t my_read(struct file *filp, char __user *buf, size_t count, loff_t *off)
{
    ssize_t retval;
    int read_count;

    if (mutex_lock_interruptible(&my_device->lock))
        return -ERESTARTSYS;

    read_count = min(count, BUFFER_SIZE - (int)*off);
    if (read_count <= 0) {
        mutex_unlock(&my_device->lock);
        return 0;
    }

    retval = copy_to_user(buf, my_device->buffer + *off, read_count);
    if (retval) {
        mutex_unlock(&my_device->lock);
        return -EFAULT;
    }

    *off += read_count;
    mutex_unlock(&my_device->lock);
    return read_count;
}

static ssize_t my_write(struct file *filp, const char __user *buf, size_t count, loff_t *off)
{
    ssize_t retval;
    int write_count;

    if (mutex_lock_interruptible(&my_device->lock))
        return -ERESTARTSYS;

    write_count = min(count, BUFFER_SIZE - (int)*off);
    if (write_count <= 0) {
        mutex_unlock(&my_device->lock);
        return 0;
    }

    retval = copy_from_user(my_device->buffer + *off, buf, write_count);
    if (retval) {
        mutex_unlock(&my_device->lock);
        return -EFAULT;
    }

    *off += write_count;
    mutex_unlock(&my_device->lock);
    return write_count;
}

static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
        // Add IOCTL commands here
    default:
        return -ENOTTY;
    }
    return 0;
}

module_init(my_init);
module_exit(my_cleanup);