#ifndef _DEVFS_H
#define _DEVFS_H

#include <sys/types.h>
#include <sys/stat.h>

/* Device file operations */
struct devfs_ops {
    int (*open)(struct file *file);
    int (*close)(struct file *file);
    ssize_t (*read)(struct file *file, char __user *buf, size_t count, loff_t *ppos);
    ssize_t (*write)(struct file *file, const char __user *buf, size_t count, loff_t *ppos);
    int (*ioctl)(struct file *file, unsigned int cmd, unsigned long arg);
    int (*mmap)(struct file *file, struct vm_area_struct *vma);
};

/* Device file descriptor */
struct devfs_dfd {
    struct file file;
    struct devfs_ops ops;
};

/* Device file */
struct devfs_device {
    dev_t dev;
    struct cdev cdev;
    struct devfs_ops ops;
};

#define DEVFSDirectory(name, parent) \
    ((struct devfs_directory *)kmalloc(sizeof(struct devfs_directory), GFP_KERNEL))->name = name; \
    ((struct devfs_directory *)kmalloc(sizeof(struct devfs_directory), GFP_KERNEL))->parent = parent;

#define DEVFSPrivateData(priv) \
    ((struct devfs_private_data *)kmalloc(sizeof(struct devfs_private_data), GFP_KERNEL))->priv = priv;

#define DEVFSSetup(dev, type, major, minor, name, private_data) \
    do { \
        dev->dev = MkDev(major, minor); \
        dev->type = type; \
        dev->name = name; \
        dev->private_data = private_data; \
    } while (0)

#define DEVFSOpen(dev, flags) \
    do { \
        if (!try_module_get(THIS_MODULE)) \
            return -ENODEV; \
        return devfs_open(dev, flags); \
    } while (0)

#define DEVFSClose(dev) \
    do { \
        module_put(THIS_MODULE); \
        devfs_close(dev); \
    } while (0)

#define DEVFSIoctl(dev, cmd, arg) \
    devfs_ioctl(dev, cmd, arg)

#define DEVFSMMap(dev, vma) \
    devfs_mmap(dev, vma)

#define DEVFSEvent(dev, event) \
    devfs_event(dev, event)

#endif  // _DEVFS_H
