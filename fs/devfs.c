#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>       /* VFS */
#include <linux/pagemap.h>  /* generic_file_llseek */
#include <linux/slab.h>     /* kzalloc, kfree */
#include <linux/time.h>     /* current_time */
#include <linux/uaccess.h>  /* copy_to_user, copy_from_user */
#include <linux/mutex.h>    /* Mutex for locking */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mikhail / Improved");
MODULE_DESCRIPTION("An improved, simple, in-memory device file system");

#define DEVFS_SUPER_MAGIC 0x20240500
#define DEVFS_DEFAULT_BUF_SIZE PAGE_SIZE
#define DEVFS_FILENAME "buffer"

// Forward declarations
static int devfs_fill_super(struct super_block *sb, void *data, int silent);
static struct dentry *devfs_mount(struct file_system_type *fs_type, int flags,
                           const char *dev_name, void *data);
static void devfs_evict_inode(struct inode *inode);

// Read/Write for the in-memory file
static ssize_t devfs_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos);
static ssize_t devfs_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos);

/*
 * Internal representation for our in-memory file's data.
 * Stored in inode->i_private
 */
struct devfs_inode_info {
	char *buffer;
	size_t buffer_size;
	struct mutex lock; /* Protects buffer and i_size */
};


//=============================================================================
// FILE SYSTEM TYPE
//=============================================================================

static struct file_system_type devfs_type = {
	.owner		= THIS_MODULE,
	.name		= "devfs_improved",
	.mount		= devfs_mount,
	.kill_sb	= kill_litter_super, // Good for in-memory FS
	.fs_flags	= FS_USERNS_MOUNT,
};

//=============================================================================
// OPERATIONS
//=============================================================================

/* File operations for our regular (buffer) file */
static const struct file_operations devfs_file_operations = {
	.owner		= THIS_MODULE,
	.llseek		= generic_file_llseek, // Use the VFS generic
	.read		= devfs_read,
	.write		= devfs_write,
	/* .open, .release, .ioctl, .mmap could be added if needed */
};

/*
 * Inode operations for the ROOT DIRECTORY.
 * We use simple_lookup because we manually populate the directory
 * in fill_super using d_instantiate.
 * If we wanted to support dynamic mkdir/create, we would
 * need to provide our own implementations here.
 */
static const struct inode_operations devfs_dir_inode_operations = {
	.lookup = simple_lookup,
};

/* Super block operations */
static const struct super_operations devfs_super_operations = {
	.statfs		= simple_statfs,
	// .drop_inode	= generic_delete_inode, // Calls clear_inode
	.evict_inode    = devfs_evict_inode,    // Needed to free i_private
};


//=============================================================================
// IMPLEMENTATIONS
//=============================================================================

/**
 * devfs_evict_inode - Called when inode refcount reaches zero.
 * @inode: The inode being evicted.
 *
 * Frees the private data buffer associated with our file inode.
 */
static void devfs_evict_inode(struct inode *inode)
{
	struct devfs_inode_info *info = inode->i_private;

	pr_debug("devfs: Evicting inode %lu\n", inode->i_ino);
	truncate_inode_pages_final(&inode->i_data);
	clear_inode(inode); // Standard VFS cleanup
	if (info) {
		// Free the buffer we allocated
		kfree(info->buffer);
		kfree(info);
	}
}


/**
 * devfs_get_inode - Create a new inode
 * @sb: Superblock
 * @mode: Inode mode (permissions and type)
 * @dev: device number (if S_ISBLK or S_ISCHR)
 *
 * Allocates a new inode and sets some default parameters.
 * Returns: Pointer to inode, or NULL on failure.
 */
static struct inode *devfs_get_inode(struct super_block *sb,
						const struct inode *dir,
						umode_t mode,
						dev_t dev)
{
	struct inode *inode = new_inode(sb);

	if (inode) {
		inode->i_mode = mode;
		inode->i_uid.val = 0;
		inode->i_gid.val = 0;
		//inode_init_owner(inode, dir, mode); // Use this for current user
		inode->i_blocks = 0;
		inode->i_atime = inode->i_mtime = inode->i_ctime = current_time(inode);
		switch (mode & S_IFMT) {
		case S_IFREG:
			pr_debug("devfs: Creating REG file inode\n");
			inode->i_op = &devfs_dir_inode_operations; //FIXME: Should be file_inode_ops if we had them
			inode->i_fop = &devfs_file_operations;
			break;
		case S_IFDIR:
			pr_debug("devfs: Creating DIR file inode\n");
			inode->i_op = &devfs_dir_inode_operations;
			inode->i_fop = &simple_dir_operations; // Allows readdir etc.
			inc_nlink(inode); /* Dirs get '.' link */
			break;
        		 default:
			pr_err("devfs: Unsupported inode type\n");
			// Should free inode here, but new_inode/iput handles this
			return ERR_PTR(-EINVAL);
		}
	}
	return inode;
}


/**
 * devfs_read - Read from the in-memory file
 */
static ssize_t devfs_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
	struct inode *inode = file_inode(filp);
	struct devfs_inode_info *info = inode->i_private;
	ssize_t retval = 0;
	size_t file_size;

	if (!info || !info->buffer)
		return -EIO;

	mutex_lock(&info->lock);

	file_size = i_size_read(inode);

	if (*ppos >= file_size) {
		pr_debug("devfs: Read EOF (pos=%lld, size=%zu)\n", *ppos, file_size);
		goto out_unlock; /* EOF */
	}

	// Adjust count if it reads past the end
	if (count > file_size - *ppos)
		count = file_size - *ppos;

	pr_debug("devfs: Reading %zu bytes at pos %lld (size=%zu)\n",
		 count, *ppos, file_size);

	if (copy_to_user(buf, info->buffer + *ppos, count)) {
		retval = -EFAULT;
		goto out_unlock;
	}

	*ppos += count;
	retval = count;

out_unlock:
	mutex_unlock(&info->lock);
	return retval;
}

/**
 * devfs_write - Write to the in-memory file
 */
static ssize_t devfs_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
	struct inode *inode = file_inode(filp);
	struct devfs_inode_info *info = inode->i_private;
	ssize_t retval = -ENOMEM; // Assume out of memory

	if (!info || !info->buffer)
		return -EIO;

	mutex_lock(&info->lock);

	// Check if write exceeds our allocated buffer size
	if (*ppos >= info->buffer_size) {
		pr_warn("devfs: Write beyond allocated buffer (pos=%lld, max=%zu)\n",
			*ppos, info->buffer_size);
		retval = -ENOSPC; // No space left on device
		goto out_unlock;
	}

       // Prevent writing past the end of our fixed-size buffer
	if (count > info->buffer_size - *ppos) {
		count = info->buffer_size - *ppos;
         }

	pr_debug("devfs: Writing %zu bytes at pos %lld (max_size=%zu)\n",
		 count, *ppos, info->buffer_size);

	if (copy_from_user(info->buffer + *ppos, buf, count)) {
		retval = -EFAULT;
		goto out_unlock;
	}

	*ppos += count;
	retval = count;

	// Update file size if we wrote past the previous end
	if (*ppos > i_size_read(inode)) {
		i_size_write(inode, *ppos);
		pr_debug("devfs: New file size %lld\n", *ppos);
	}
        // Update timestamp
	inode->i_mtime = inode->i_ctime = current_time(inode);

out_unlock:
	mutex_unlock(&info->lock);
	return retval;
}


/**
 * devfs_fill_super - Fill superblock information, create root and files.
 */
int devfs_fill_super(struct super_block *sb, void *data, int silent)
{
	struct inode *root_inode = NULL;
	struct inode *file_inode = NULL;
	struct dentry *root_dentry = NULL;
	struct dentry *file_dentry = NULL;
	struct devfs_inode_info *info = NULL;
	int err = -ENOMEM;

	sb->s_maxbytes		= MAX_LFS_FILESIZE;
	sb->s_blocksize		= PAGE_SIZE;
	sb->s_blocksize_bits	= PAGE_SHIFT;
	sb->s_magic		= DEVFS_SUPER_MAGIC;
	sb->s_op		= &devfs_super_operations;
	sb->s_time_gran		= 1; // nanosecond timestamp granularity

	// 1. Create Root Inode
	root_inode = devfs_get_inode(sb, NULL, S_IFDIR | 0755, 0);
	if (IS_ERR(root_inode)) {
	     err = PTR_ERR(root_inode);
	     goto failed;
	}
	root_inode->i_ino = 1; // Typically 1, but not required
	pr_info("devfs: Root inode created\n");

	// 2. Create Root Dentry
	root_dentry = d_make_root(root_inode);
	if (!root_dentry)
		goto failed_iput_root;
	sb->s_root = root_dentry;


       // 3. Create the In-Memory File Inode
       file_inode = devfs_get_inode(sb, root_inode, S_IFREG | 0666, 0);
       if (IS_ERR(file_inode)) {
	     err = PTR_ERR(file_inode);
             goto failed; // d_make_root will iput(root_inode) on failure
       }
       file_inode->i_ino = 2;

       // 3a. Allocate private data (the buffer) for the file
       info = kzalloc(sizeof(struct devfs_inode_info), GFP_KERNEL);
       if (!info)
	       goto failed_iput_file;

       info->buffer = kzalloc(DEVFS_DEFAULT_BUF_SIZE, GFP_KERNEL);
        if (!info->buffer) {
		kfree(info);
		goto failed_iput_file;
	}
	info->buffer_size = DEVFS_DEFAULT_BUF_SIZE;
	mutex_init(&info->lock);
	file_inode->i_private = info;
	i_size_write(file_inode, 0); // Initially empty

	// 4. Create the file Dentry and link it
	file_dentry = d_alloc_name(root_dentry, DEVFS_FILENAME);
	if (!file_dentry)
		goto failed_iput_file;

	d_add(file_dentry, file_inode);
	// d_instantiate(file_dentry, file_inode); // Use d_add

	pr_info("devfs: Filesystem mounted, file '%s' created.\n", DEVFS_FILENAME);
	return 0; // SUCCESS!

failed_iput_file:
	iput(file_inode); // Will call evict_inode to free private data
	goto failed;
failed_iput_root:
       iput(root_inode);
failed:
	pr_err("devfs: Mount failed with error %d\n", err);
	return err;
}

/**
 * devfs_mount - Mount the devfs filesystem
 */
struct dentry *devfs_mount(struct file_system_type *fs_type, int flags,
			   const char *dev_name, void *data)
{
	pr_info("devfs: Mounting...\n");
	// mount_nodev is appropriate for filesystems that don't
	// sit on a block device.
	return mount_nodev(fs_type, flags, data, devfs_fill_super);
}


//=============================================================================
// MODULE INIT / EXIT
//=============================================================================

/**
 * devfs_init - Initialize the devfs module
 */
static int __init devfs_init(void)
{
	int ret;

	ret = register_filesystem(&devfs_type);
	if (ret) {
		pr_err("devfs: Failed to register filesystem\n");
		return ret;
	}

	pr_info("devfs: Module loaded\n");
	return 0;
}

/**
 * devfs_exit - Cleanup and exit the devfs module
 */
static void __exit devfs_exit(void)
{
	unregister_filesystem(&devfs_type);
	pr_info("devfs: Module unloaded\n");
}

module_init(devfs_init);
module_exit(devfs_exit);
