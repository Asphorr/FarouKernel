#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/atomic.h>
#include <linux/device.h>
#include <linux/ioctl.h>
#include <linux/mm.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mikhail");
MODULE_DESCRIPTION("Improved Character Device Driver with Dynamic Buffer");

#define MY_MAGIC 'M'
#define MY_IOCTL_GET_BUFFER_SIZE _IOR(MY_MAGIC, 1, int)
#define MY_IOCTL_SET_BUFFER_SIZE _IOW(MY_MAGIC, 2, int)
#define MY_IOCTL_CLEAR_BUFFER    _IO(MY_MAGIC, 3)
#define MY_IOCTL_GET_OPEN_COUNT  _IOR(MY_MAGIC, 4, int)

struct my_device {
    struct cdev cdev;
    struct class *class;
    dev_t devno;
    atomic_t open_count;
    char *buffer;
    size_t buffer_size;
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

    retval = alloc_chrdev_region(&devno, 0, 1, "mydynamicchar");
    if (retval < 0) {
        pr_err("Не удалось выделить major number\n");
        return retval;
    }

    my_device = devm_kzalloc(NULL, sizeof(*my_device), GFP_KERNEL);
    if (!my_device) {
        pr_err("Не удалось выделить структуру устройства\n");
        retval = -ENOMEM;
        goto err_unreg_region;
    }
    my_device->devno = devno;
    atomic_set(&my_device->open_count, 0);
    mutex_init(&my_device->lock);

    // Начальный размер буфера
    my_device->buffer_size = PAGE_SIZE;
    my_device->buffer = devm_kzalloc(NULL, my_device->buffer_size, GFP_KERNEL);
    if (!my_device->buffer) {
        pr_err("Не удалось выделить буфер\n");
        retval = -ENOMEM;
        goto err_free_dev;
    }

    cdev_init(&my_device->cdev, &my_fops);
    my_device->cdev.owner = THIS_MODULE;

    retval = cdev_add(&my_device->cdev, devno, 1);
    if (retval < 0) {
        pr_err("Не удалось добавить устройство\n");
        goto err_free_buffer;
    }

    my_device->class = class_create(THIS_MODULE, "mydynamicclass");
    if (IS_ERR(my_device->class)) {
        pr_err("Не удалось создать класс устройства\n");
        retval = PTR_ERR(my_device->class);
        goto err_del_cdev;
    }

    if (device_create(my_device->class, NULL, devno, NULL, "mydynamicchar") == NULL) {
        pr_err("Не удалось создать файл устройства\n");
        retval = -EINVAL;
        goto err_destroy_class;
    }

    pr_info("Устройство с динамическим буфером инициализировано успешно\n");
    return 0;

err_destroy_class:
    class_destroy(my_device->class);
err_del_cdev:
    cdev_del(&my_device->cdev);
err_free_buffer:
    // devm_kzalloc освободит буфер
err_free_dev:
    // devm_kzalloc освободит структуру устройства
err_unreg_region:
    unregister_chrdev_region(devno, 1);
    return retval;
}

static void __exit my_cleanup(void)
{
    device_destroy(my_device->class, my_device->devno);
    class_destroy(my_device->class);
    unregister_chrdev_region(my_device->devno, 1);
    pr_info("Устройство с динамическим буфером удалено успешно\n");
}

static int my_open(struct inode *inode, struct file *file)
{
    mutex_lock(&my_device->lock);
    atomic_inc(&my_device->open_count);
    mutex_unlock(&my_device->lock);
    pr_debug("Устройство открыто, количество открытий: %d\n", atomic_read(&my_device->open_count));
    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    mutex_lock(&my_device->lock);
    atomic_dec(&my_device->open_count);
    mutex_unlock(&my_device->lock);
    pr_debug("Устройство закрыто, количество открытий: %d\n", atomic_read(&my_device->open_count));
    return 0;
}

static ssize_t my_read(struct file *filp, char __user *buf, size_t count, loff_t *off)
{
    ssize_t retval;
    size_t available = my_device->buffer_size - (size_t)*off;

    if (mutex_lock_interruptible(&my_device->lock))
        return -ERESTARTSYS;

    if (*off >= my_device->buffer_size || available == 0) {
        mutex_unlock(&my_device->lock);
        return 0;
    }

    size_t read_count = min_t(size_t, count, available);

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
    size_t available = my_device->buffer_size - (size_t)*off;

    if (mutex_lock_interruptible(&my_device->lock))
        return -ERESTARTSYS;

    if (*off >= my_device->buffer_size || available == 0) {
        mutex_unlock(&my_device->lock);
        return 0;
    }

    size_t write_count = min_t(size_t, count, available);

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
    int retval = 0;
    int __user *argp = (int __user *)arg;
    int value;
    char *new_buffer;

    switch (cmd) {
        case MY_IOCTL_GET_BUFFER_SIZE:
            if (copy_to_user(argp, &my_device->buffer_size, sizeof(int)))
                retval = -EFAULT;
            break;

        case MY_IOCTL_SET_BUFFER_SIZE:
            if (get_user(value, argp))
                return -EFAULT;

            if (value <= 0 || value > 10 * PAGE_SIZE) // Ограничение размера
                return -EINVAL;

            new_buffer = kmalloc(value, GFP_KERNEL);
            if (!new_buffer)
                return -ENOMEM;

            mutex_lock(&my_device->lock);
            if (my_device->buffer)
                kfree(my_device->buffer);
            my_device->buffer = new_buffer;
            my_device->buffer_size = value;
            mutex_unlock(&my_device->lock);

            pr_info("Размер буфера изменен на: %d\n", value);
            break;

        case MY_IOCTL_CLEAR_BUFFER:
            mutex_lock(&my_device->lock);
            memset(my_device->buffer, 0, my_device->buffer_size);
            mutex_unlock(&my_device->lock);
            pr_info("Буфер очищен\n");
            break;

        case MY_IOCTL_GET_OPEN_COUNT:
            value = atomic_read(&my_device->open_count);
            if (copy_to_user(argp, &value, sizeof(int)))
                retval = -EFAULT;
            break;

        default:
            retval = -ENOTTY;
            break;
    }
    return retval;
}

module_init(my_init);
module_exit(my_cleanup);