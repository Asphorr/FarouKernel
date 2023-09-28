#include <linux/console.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/fcntl.h>
#include <linux/sysrq.h>
#include <asm/ioctls.h>

#define MAX_CONSOLES 8

static struct console consoles[MAX_CONSOLES];
static int con_num = 0;

static int console_open(struct file *file)
{
    int i;
    for (i = 0; i < MAX_CONSOLES; i++) {
        if (!consoles[i].device) {
            consoles[i].device = file->private_data;
            consoles[i].flags |= CON_ENABLED;
            return 0;
        }
    }
    return -EBUSY;
}

static int console_close(struct file *file)
{
    int i;
    for (i = 0; i < MAX_CONSOLES; i++) {
        if (consoles[i].device == file->private_data) {
            consoles[i].device = NULL;
            consoles[i].flags &= ~CON_ENABLED;
            return 0;
        }
    }
    return -ENODEV;
}

static ssize_t console_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    int i;
    for (i = 0; i < MAX_CONSOLES; i++) {
        if (consoles[i].device == file->private_data) {
            return consoles[i].read(buf, count, ppos);
        }
    }
    return -ENODEV;
}

static ssize_t console_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    int i;
    for (i = 0; i < MAX_CONSOLES; i++) {
        if (consoles[i].device == file->private_data) {
            return consoles[i].write(buf, count, ppos);
        }
    }
    return -ENODEV;
}

static int console_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int i;
    for (i = 0; i < MAX_CONSOLES; i++) {
        if (consoles[i].device == file->private_data) {
            return consoles[i].ioctl(cmd, arg);
        }
    }
    return -ENODEV;
}

static struct file_operations console_fops = {
    .owner = THIS_MODULE,
    .open = console_open,
    .release = console_close,
    .read = console_read,
    .write = console_write,
    .unlocked_ioctl = console_ioctl,
};

static int __init console_init(void)
{
    int ret;
    ret = register_chrdev(CON_MAJOR, "console", &console_fops);
    if (ret < 0) {
        printk(KERN_ERR "Console driver registration failed\n");
        return ret;
    }
    printk(KERN_INFO "Console driver registered successfully\n");
    return 0;
}

static void __exit console_exit(void)
{
    unregister_chrdev(CON_MAJOR, "console");
    printk(KERN_INFO "Console driver unregistered successfully\n");
}

module_init(console_init);
module_exit(console_exit);
