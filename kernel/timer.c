#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/timekeeping.h>
#include <linux/timerfd.h>
#include <linux/workqueue.h>

// Define the major number for the timer device node
#define TIMER_MAJOR 123

// Structure representing a single timer instance
struct timer {
    // The actual timer object
    struct timer_list list;

    // The time at which the timer should fire
    u64 expires;

    // A flag indicating whether the timer has been started or stopped
    bool running;
};

// Function prototypes
static int timer_open(struct inode *inode, struct file *filp);
static int timer_release(struct inode *inode, struct file *filp);
static ssize_t timer_read(struct file *filp, char __user *buf, size_t len, loff_t *off);
static ssize_t timer_write(struct file *filp, const char __user *buf, size_t len, loff_t *off);
static int timer_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static void timer_interrupt(struct timer_list *t);
static int timer_start(struct timer *tmr);
static int timer_stop(struct timer *tmr);

// Module initialization function
static int __init timer_init(void)
{
    int ret;

    // Register the character device driver
    ret = register_chrdev(TIMER_MAJOR, "timer", &timer_fops);
    if (ret) {
        pr_err("Failed to register character device driver\n");
        return ret;
    }

    // Allocate memory for the timer structure
    struct timer *tmr = kmalloc(sizeof(*tmr), GFP_KERNEL);
    if (!tmr) {
        pr_err("Failed to allocate memory for timer structure\n");
        goto err_kmalloc;
    }

    // Initialize the timer structure
    tmr->running = false;
    tmr->expires = 0;
    INIT_LIST_HEAD(&tmr->list);

    // Add the timer to the system's work queue
    add_timer(&tmr->list);

    return 0;

err_kmalloc:
    unregister_chrdev(TIMER_MAJOR, "timer");
    return ret;
}

// Module exit function
static void __exit timer_exit(void)
{
    // Remove the timer from the system's work queue
    remove_timer(&tmr->list);

    // Free the memory allocated for the timer structure
    kfree(tmr);

    // Unregister the character device driver
    unregister_chrdev(TIMER_MAJOR, "timer");
}

// File operations structure for the timer device node
static const struct file_operations timer_fops = {
    .owner   = THIS_MODULE,
    .open    = timer_open,
    .release = timer_release,
    .read    = timer_read,
    .write   = timer_write,
    .llseek = noop_llseek,
    .unlocked_ioctl = timer_ioctl,
};

// Open method for the timer device node
static int timer_open(struct inode *inode, struct file *filp)
{
    // Get the pointer to the timer structure associated with the device node
    struct timer *tmr = container_of(inode->i_cdev, struct timer, cdev);

    // Check if the timer is already open
    if (tmr->running) {
        return -EBUSY;
    }

    // Start the timer
    return timer_start(tmr);
}

// Release method for the timer device node
static int timer_release(struct inode *inode, struct file *filp)
{
    // Get the pointer to the timer structure associated with the device node
    struct timer *tmr = container_of(inode->i_cdev, struct timer, cdev);

    // Stop the timer
    return timer_stop(tmr);
}

// Read method for the timer device node
static ssize_t timer_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
    // Get the pointer to the timer structure associated with the device node
    struct timer *tmr = filp->private_data;

    // Return the current value of the timer
    return copy_to_user(buf, &tmr->expires, sizeof(tmr->expires));
}

// Write method for the timer device node
static ssize_t timer_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
    // Get the pointer to the timer structure associated with the device node
    struct timer *tmr = filp->private_data;

    // Set the new value of the timer
    return copy_from_user(&tmr->expires, buf, sizeof(tmr->expires));
}

// Ioctl method for the timer device node
static int timer_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    // Get the pointer to the timer structure associated with the device node
    struct timer *tmr = filp->private_data;

    // Handle the ioctl command
    switch (cmd) {
    case TIMER_START:
        return timer_start(tmr);
    case TIMER_STOP:
        return timer_stop(tmr);
    default:
        return -EINVAL;
    }
}

// Interrupt handler for the timer interrupt
static irqreturn_t timer_interrupt(int irq, void *dev_id)
{
    // Get the pointer to the timer structure associated with the device node
    struct timer *tmr = dev_id;

    // Update the timer's expiration time
    tmr->expires += msecs_to_jiffies(1);

    // Wake up any processes waiting on the timer
    wake_up_interruptible(&tmr->wait);

    return IRQ_HANDLED;
}

// Start the timer
static int timer_start(struct timer *tmr)
{
    // Make sure the timer isn't already running
    if (tmr->running) {
        return -EBUSY;
    }

    // Enable the timer interrupt
    enable_irq(tmr->irq);

    // Mark the timer as running
    tmr->running = true;

    return 0;
}

// Stop the timer
static int timer_stop(struct timer *tmr)
{
    // Disable the timer interrupt
    disable_irq(tmr->irq);

    // Mark the timer as stopped
    tmr->running = false;

    return 0;
}

module_init(timer_init);
module_exit(timer_exit);

MODULE_DESCRIPTION("Simple timer example");
MODULE_AUTHOR("Mikhail");
MODULE_VERSION("1.0");
MODULE_LICENSE("GPL");
