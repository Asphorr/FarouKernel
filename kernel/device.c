#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/minor.h>
#include <asm/ioctl.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mikhail");
MODULE_DESCRIPTION("Optimized Character Device Driver");

// Define constants for the module parameters
#define MAJOR_NUM 245 // Major number for the character device
#define MINOR_NUM 0   // Minor number for the character device

// Structure representing the character device
typedef struct _my_device {
    struct cdev cdev;     // The character device structure
    struct class *class; // Pointer to the device's class
    dev_t devno;          // Device number
    atomic_t open_count; // Number of times the device has been opened
} my_device_t;

// Function prototypes
int my_open(struct inode *inode, struct file *file);
int my_release(struct inode *inode, struct file *file);
ssize_t my_read(struct file *file, char __user *buf, size_t count, loff_t *offset);
ssize_t my_write(struct file *file, const char __user *buf, size_t count, loff_t *offset);
long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

// Global variables
static my_device_t my_device; // The character device object

// Initialize the character device
static int __init my_init(void)
{
    int retval;

    // Allocate memory for the character device structure
    my_device = kmalloc(sizeof(*my_device), GFP_KERNEL);
    if (!my_device) {
        pr_err("%s: Failed to allocate memory for the character device structure\n", __func__);
        return -ENOMEM;
    }

    // Set up the character device structure
    memset(my_device, 0, sizeof(*my_device));
    my_device->cdev.owner = THIS_MODULE;
    my_device->cdev.ops = &my_fops;
    my_device->class = class_create(THIS_MODULE, "myclass");
    if (IS_ERR(my_device->class)) {
        pr_err("%s: Failed to create class\n", __func__);
        kfree(my_device);
        return PTR_ERR(my_device->class);
    }

    // Register the character device
    retval = alloc_chrdev_region(&my_device->devno, MAJOR_NUM, MINOR_NUM, "mychar");
    if (retval < 0) {
        pr_err("%s: Failed to register character device\n", __func__);
        class_destroy(my_device->class);
        kfree(my_device);
        return retval;
    }

    // Create the character device node
    retval = device_create(my_device->class, NULL, my_device->devno, NULL, "mychar%d", MINOR_NUM);
    if (retval < 0) {
        pr_err("%s: Failed to create character device node\n", __func__);
        unregister_chrdev_region(my_device->devno, MINOR_NUM);
        class_destroy(my_device->class);
        kfree(my_device);
        return retval;
    }

    // Print a message indicating successful initialization
    pr_info("%s: Initialized character device %s\n", __func__, "/dev/mychar");

    return 0;
}

// Cleanup function called when the kernel module is removed
static void __exit my_cleanup(void)
{
    // Remove the character device node
    device_destroy(my_device->class, my_device->devno);

    // Unregister the character device
    unregister_chrdev_region(my_device->devno, MINOR_NUM);

    // Destroy the class
    class_destroy(my_device->class);

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

// Check whether there are any characters left to read from the buffer
if (*off >= BUFFER_SIZE || !atomic_read(&my_device->buffer[*off])) {
    pr_warn("%s: No more data available to read\n", __func__);
    return 0;
}

// Copy the next character from the buffer into userspace
get_user(retval, buf);
put_user(atomic_read(&my_device->buffer[*off]), buf);

// Update the offset and number of bytes remaining in the buffer
++*off;
--len;

// Return the number of bytes copied
return retval;
}

// Write method for the character device
ssize_t my_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
ssize_t retval;

// Check whether we've reached the end of the buffer
if (*off == BUFFER_SIZE) {
    pr_warn("%s: Reached maximum capacity of buffer\n", __func__);
    return 0;
}

// Copy the next character from userspace into the buffer
put_user(get_user(retval, buf), &my_device->buffer[*off]);

// Update the offset and number of bytes remaining in the buffer
++*off;
--len;

// Return the number of bytes copied
return retval;
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
