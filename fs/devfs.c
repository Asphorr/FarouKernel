#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <asm/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mikhail");
MODULE_DESCRIPTION("A sample device file system");

static struct kmemleak_notifier devfs_kn = {
    .fn = NULL,
    .ctx = NULL,
};

static int __init devfs_init(void)
{
    return register_filesystem(&devfs_type);
}

static void __exit devfs_exit(void)
{
    unregister_filesystem(&devfs_type);
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .llseek = generic_file_llseek,
    .read = devfs_read,
    .write = devfs_write,
    .open = devfs_open,
    .release = devfs_release,
    .unlocked_ioctl = devfs_ioctl,
    .compat_ioctl = devfs_ compat_ioctl,
    .mmap = devfs_mmap,
};

static struct address_space_operations addr_ops = {
    .readpage = devfs_readpage,
    .writepage = devfs_writepage,
    .sync_page = devfs_sync_page,
};

static struct inode_operations inode_ops = {
    .create = devfs_create,
    .lookup = devfs_lookup,
    .link = devfs_link,
    .unlink = devfs_unlink,
    .rmdir = devfs_rmdir,
    .rename = devfs_rename,
    .setattr = devfs_setattr,
    .getattr = devfs_getattr,
};

static struct super_block sb = {
    .sb_fops = &fops,
    .sb_addr_ops = &addr_ops,
    .sb_inode_ops = &inode_ops,
    .sb_flags = SB_NODIRATIME | SB_RDONLY,
};

static struct dentry *devfs_mount(struct file_system_type *fs_type, int flags,
    const char *dev_name, void *data)
{
    return mount_nodev(fs_type, flags, &sb, dev_name);
}

static struct file_system_type devfs_type = {
    .name = "devfs",
    .mount = devfs_mount,
    .kill_sb = kill_litter_super,
};

static int devfs_parse_param(const char *options, struct devfs_params *params)
{
    int ret = 0;

    while (*options != '\0') {
        char *token;
        subsys_initcall_t func;

        token = strsep(&options, ",");
        if (!*token)
            continue;

        func = (subsys_initcall_t) token;
        ret = func();
        if (ret)
            break;
    }

    return ret;
}

static int __init devfs_init_module(void)
{
    int ret;

    ret = devfs_parse_param(boot_params, &devfs_params);
    if (ret)
        goto out1;

    ret = register_filesystem(&devfs_type);
    if (ret)
        goto out2;

    printk(KERN_INFO "Initialized devfs\n");

    return 0;
out2:
    unregister_filesystem(&devfs_type);
out1:
    return ret;
}

static void __exit devfs_exit_module(void)
{
    unregister_filesystem(&devfs_type);
}
