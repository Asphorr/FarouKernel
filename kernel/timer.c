#include <linux/init.h>
#include <linux/module.h>
#include <linux/sysdev.h>
#include <linux/proc_fs.h>
#include <linux/semaphore.h>
#include <asm/uaccess.h>

#define PROC_ROOT		0
#define TIMER_NAME	"timer"
#define TIMER_MINOR	123

struct timer {
    struct timer_list list;
    unsigned int expires;
};

static struct timer *timer;
static struct semaphore sem;

static int timer_open(struct inode *inode, struct file *file)
{
    down(&sem);
    return 0;
}

static int timer_release(struct inode *inode, struct file *file)
{
    up(&sem);
    return 0;
}

static ssize_t timer_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    return 0;
}

static ssize_t timer_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    return 0;
}

static int timer_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
    case TIMER_SET:
        timer->expires = arg;
        mod_timer(&timer->list, timer->expires);
        break;
    case TIMER_GET:
        return put_user(jiffies_to_msecs(timer->expires), (int __user *)arg);
    default:
        return -ENOIOCTLCMD;
    }
    return 0;
}

static void timer_interrupt(struct timer_list *t)
{
    up(&sem);
}

static int __init timer_init(void)
{
    int ret;

    ret = register_chrdev(TIMER_MAJOR, TIMER_NAME, &timer_fops);
    if (ret) {
        printk(KERN_ERR "Timer: unable to register character device\n");
        return ret;
    }

    timer = kmalloc(sizeof(struct timer), GFP_KERNEL);
    if (!timer) {
        printk(KERN_ERR "Timer: unable to allocate memory for timer structure\n");
        return -ENOMEM;
    }

    init_timer(&timer->list);
    timer->expires = jiffies + HZ;
    mod_timer(&timer->list, timer->expires);

    return 0;
}

static void __exit timer_exit(void)
{
    del_timer(&timer->list);
    kfree(timer);

    unregister_chrdev(TIMER_MAJOR, TIMER_NAME);
}

module_init(timer_init);
module_exit(timer_exit);

MODULE_LICENSE("GPL");
