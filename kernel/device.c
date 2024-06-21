#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/minor.h>
#include <asm/ioctl.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mikhail");
MODULE_DESCRIPTION("Optimized Character Device Driver");

// Define constants for the module parameters
#define MAJOR_NUM 245 // Major number for the character device
#define MINOR_NUM 0   // Minor number for the character device
#define BUFFER_SIZE 1024 // Size of the character device buffer

// Structure representing the character device
typedef struct _my_device {
    struct cdev cdev;     // The character device structure
    struct class *class; // Pointer to the device's class
    dev_t devno;          // Device number
    atomic_t open_count; // Number of times the device has been opened
    char *buffer;        // Buffer for storing data
} my_device_t;

// Function prototypes
int my_open(struct inode *inode, struct file *file);
int my_release(struct inode *inode, struct file *file);
ssize_t my_read(struct file *file, char __user *buf, size_t count, loff_t *offset);
ssize_t my_write(struct file *file, const char __user *buf, size_t count, loff_t *offset);
long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

// Global variables
static my_device_t *my_device; // The character device object

// Initialize the character device
static int __init my_init(void)
{
    int retval;

    // Allocate memory for the character device structure
    my_device = kmalloc(sizeof(my_device_t), GFP_KERNEL);
    if (!my_device) {
        pr_err("%s: Failed to allocate memory for the character device structure\n", __func__);
        return -ENOMEM;
    }

    // Allocate memory for the character device buffer
    my_device->buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
    if (!my_device->buffer) {
        pr_err("%s: Failed to allocate memory for the character device buffer\n", __func__);
        kfree(my_device);
        return -ENOMEM;
    }

    // Set up the character device structure
    memset(my_device, 0, sizeof(my_device_t));
    my_device->cdev.owner = THIS_MODULE;
    cdev_init(&my_device->cdev, &my_fops);
    my_device->class = class_create(THIS_MODULE, "myclass");
    if (IS_ERR(my_device->class)) {
        pr_err("%s: Failed to create class\n", __func__);
        kfree(my_device->buffer);
        kfree(my_device);
        return PTR_ERR(my_device->class);
    }

    // Register the character device
    retval = alloc_chrdev_region(&my_device->devno, MINOR_NUM, 1, "mychar");
    if (retval < 0) {
        pr_err("%s: Failed to register character device\n", __func__);
        class_destroy(my_device->class);
        kfree(my_device->buffer);
        kfree(my_device);
        return retval;
    }

    // Add the character device to the system
    cdev_add(&my_device->cdev, my_device->devno, 1);

    // Create the character device node
    device_create(my_device->class, NULL, my_device->devno, NULL, "mychar%d", MINOR_NUM);

    // Print a message indicating successful initialization
    pr_info("%s: Initialized character device %s\n", __func__, "/dev/mychar");

    return 0;
}

// Cleanup function called when the kernel module is removed
static void __exit my_cleanup(void)
{
    // Remove the character device node
    device_destroy(my_device->class, my_device->devno);

    // Remove the character device from the system
    cdev_del(&my_device->cdev);

    // Unregister the character device
    unregister_chrdev_region(my_device->devno, 1);

    // Destroy the class
    class_destroy(my_device->class);

    // Free the memory allocated for the character device buffer
    kfree(my_device->buffer);

    // Free the memory allocated for the character device structure
    kfree(my_device);

    // Print a message indicating successful cleanup
    pr_info("%s: Cleaned up character device %s\n", __func__, "/dev/mychar");
}

// File operations structure for the character device
static struct file_operations my_fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .read = my_read,
    .write = my_write,
    .unlocked_ioctl = my_ioctl,
};

// Open method for the character device
int my_open(struct inode *inode, struct file *file)
{
    // Increment the open count
    atomic_inc(&my_device->open_count);

    // Print a message indicating that the device was opened
    pr_debug("%s: Opened character device %s\n", __func__, "/dev/mychar");

    return 0;
}

// Release method for the character device
int my_release(struct inode *inode, struct file *file)
{
    // Decrement the open count
    atomic_dec(&my_device->open_count);

    // Print a message indicating that the device was closed
    pr_debug("%s: Closed character device %s\n", __func__, "/dev/mychar");

    return 0;
}

// Read method for the character device
ssize_t my_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
    ssize_t retval;
    int count;

    // Check whether there are any characters left to read from the buffer
    count = min(len, BUFFER_SIZE - (int)*off);
    if (count <= 0) {
        pr_warn("%s: No more data available to read\n", __func__);
        return 0;
    }

    // Copy the data from the buffer into userspace
    retval = copy_to_user(buf, my_device->buffer + *off, count);
    if (retval) {
        pr_err("%s: Failed to copy data to userspace\n", __func__);
        return -EFAULT;
    }

    // Update the offset
    *off += count;

    // Return the number of bytes copied
    return count;
}

// Write method for the character device
ssize_t my_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
    ssize_t retval;
    int count;

    // Check whether we've reached the end of the buffer
    count = min(len, BUFFER_SIZE - (int)*off);
    if (count <= 0) {
        pr_warn("%s: Reached maximum capacity of buffer\n", __func__);
        return 0;
    }

    // Copy the data from userspace into the buffer
    retval = copy_from_user(my_device->buffer + *off, buf, count);
    if (retval) {
        pr_err("%s: Failed to copy data from userspace\n", __func__);
        return -EFAULT;
    }

    // Update the offset
    *off += count;

    // Return the number of bytes copied
    return count;
}

// IOCTL method for the character device
long my_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
case MYCHAR_IOC_MAGIC:
// Handle custom ioctls here
break;
default:
// Delegate unknown ioctls to the default handler
return -ENOTTY;
}


return 0;
}

module_init(my_init);
module_exit(my_cleanup);
