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
MODULE_DESCRIPTION("My Character Device Driver");

static struct cdev my_cdev;
static struct class *my_class;

static int my_open(struct file *filp)
{
    printk(KERN_INFO "Opening my char dev\n");
    return 0;
}

static int my_close(struct file *filp)
{
    printk(KERN_INFO "Closing my char dev\n");
    return 0;
}

static ssize_t my_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
    printk(KERN_INFO "Reading from my char dev\n");
    return 0;
}

static ssize_t my_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
    printk(KERN_INFO "Writing to my char dev\n");
    return 0;
}

static int my_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    printk(KERN_INFO "IOCTL called on my char dev\n");
    switch (cmd) {
        case MY_IOCTL_CMD:
            break;
        default:
            return -EFAULT;
    }
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_close,
    .read = my_read,
    .write = my_write,
    .unlocked_ioctl = my_ioctl,
};

static int __init my_init(void)
{
    major_ = register_chrdev(MY_MAJOR, "mychar", &fops);
    if (major_ < 0) {
        printk(KERN_ERR "Failed to register char dev\n");
        return major_;
    }

    my_class = class_create(THIS_MODULE, "myclass", NULL, 0);
    if (IS_ERR(my_class)) {
        printk(KERN_ERR "Failed to create class\n");
        unregister_chrdev(major_, "mychar");
        return PTR_ERR(my_class);
    }

    cdev_alloc(&my_cdev, 1, MY_MINOR, major_, my_class, NULL, NULL, NULL, NULL);
    if (IS_ERR(my_cdev)) {
        printk(KERN_ERR "Failed to allocate cdev\n");
        class_destroy(my_class);
        unregister_chrdev(major_, "mychar");
        return PTR_ERR(my_cdev);
    }

    printk(KERN_INFO "Char dev registered successfully\n");
    return 0;
}

static void __exit my_exit(void)
{
    cdev_del(&my_cdev);
    class_destroy(my_class);
    unregister_chrdev(major_, "mychar");
    printk(KERN_INFO "Char dev unregistered successfully\n");
}

module_init(my_init);
module_exit(my_exit);
