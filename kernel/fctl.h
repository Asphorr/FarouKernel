#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>

struct fcb {
   unsigned int magic;
   unsigned int flags;
   unsigned int mode;
   unsigned int offset;
};

static struct kmem_cache *fcb_cache;

static struct fcb *alloc_fcb(void)
{
   struct fcb *fcb;

   fcb = kmem_cache_zalloc(fcb_cache, GFP_KERNEL);
   if (!fcb)
       return ERR_PTR(-ENOMEM);

   fcb->magic = FCB_MAGIC;
   fcb->flags = 0;
   fcb->mode = 0;
   fcb->offset = 0;

   return fcb;
}

static void free_fcb(struct fcb *fcb)
{
   kmem_cache_free(fcb_cache, fcb);
}

static int fcb_open(struct inode *inode, struct file *filp)
{
   struct fcb *fcb;

   fcb = alloc_fcb();
   if (IS_ERR(fcb))
       return PTR_ERR(fcb);

   filp->private_data = fcb;

   return 0;
}

static int fcb_release(struct inode *inode, struct file *filp)
{
   struct fcb *fcb = filp->private_data;

   free_fcb(fcb);

   return 0;
}

static int __init init_fctl(void)
{
   fcb_cache = kmem_cache_create("fcb_cache", sizeof(struct fcb), 0, SLAB_HWCACHE_ALIGN, NULL);
   if (!fcb_cache)
       return -ENOMEM;

   printk(KERN_INFO "FCTL: initialized\n");
   return 0;
}

static void __exit exit_fctl(void)
{
   kmem_cache_destroy(fcb_cache);
   printk(KERN_INFO "FCTL: unloaded\n");
}

module_init(init_fctl);
module_exit(exit_fctl);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("File Control Block driver");
