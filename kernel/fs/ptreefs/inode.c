#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/highmem.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/backing-dev.h>
#include <linux/sched.h>
#include <linux/parser.h>
#include <linux/magic.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

/*
 * 1. init ptreefs module
 */
static struct file_system_type ptreefs_fs_type = {
	.name		= "ptreefs",
	.mount		= ptreefs_mount,
	.kill_sb	= ptreefs_kill_sb
};

int __init ptreefs_init(void)
{
        static unsigned long once;

        if (test_and_set_bit(0, &once))
        	return 0;
        return register_filesystem(&ptreefs_fs_type);
}
core_initcall(ptreefs_init);

/*
 * 2. mount ptreefs
 */
 static struct dentry *ptreefs_mount(struct file_system_type *fs_type,
 			int flags, const char *dev_name,
 			void *data)
 {
 	return mount_single(fs_type, flags, data, ptreefs_fill_super);
 }

/*
 * 3. ptreefs_fill_super
 */
 static int
 devpts_fill_super(struct super_block *s, void *data, int silent)
 {
 	struct inode *inode;

        sb->s_blocksize = PAGE_CACHE_SIZE;
	sb->s_blocksize_bits = PAGE_CACHE_SHIFT;
	sb->s_magic = PTREEFS_MAGIC;
	sb->s_op = &ptreefs_s_ops;

 	inode = new_inode(s);
 	if (!inode)
 		goto fail;
 	inode->i_ino = 1;
 	inode->i_mtime = inode->i_atime = inode->i_ctime = CURRENT_TIME;
 	inode->i_mode = S_IFDIR | S_IRUGO | S_IXUGO | S_IWUSR;
 	inode->i_op = &simple_dir_inode_operations;
 	inode->i_fop = &simple_dir_operations;
 	set_nlink(inode, 2);

 	s->s_root = d_make_root(inode);
 	if (s->s_root)
 		return 0;

 	pr_err("get root dentry failed\n");

 fail:
 	return -ENOMEM;
 }

/*
 * 4. super_operations
 */
static const struct super_operations ptreefs_sops = {
        .statfs         = simple_statfs,
        .drop_inode     = generic_delete_inode,
};
