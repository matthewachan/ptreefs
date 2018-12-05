/* -*- linux-c -*- --------------------------------------------------------- *
 *
 * linux/fs/ptreefs/inode.c
 *
 * This file is part of the Linux kernel and is made available under
 * the terms of the GNU General Public License, version 2, or at your
 * option, any later version, incorporated herein by reference.
 *
 * ------------------------------------------------------------------------- */


#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/namei.h>
#include <linux/slab.h>
#include <linux/mount.h>
#include <linux/tty.h>
#include <linux/mutex.h>
#include <linux/magic.h>
#include <linux/idr.h>
/* #include <linux/devpts_fs.h> */
#include <linux/parser.h>
#include <linux/fsnotify.h>
#include <linux/seq_file.h>

#define PTREEFS_MAGIC	0x15782921
#define DEVPTS_DEFAULT_MODE 0600

static void ptree_create_files(struct super_block *sb,
		struct dentry *root)
{
	ptree_create_file(sb, root, "init");

}

static void ptree_create_file(struct super_block *sb,
		struct dentry *dir, const char *name)
{
	struct inode *inode;
	struct dentry *dentry;
	struct qstr qname;

	qname.name	= name;
	qname.len	= strlen(name);
	qname.hash	= full_name_hash(name, qname.len);

	inode = ptree_make_inode(sb, S_IFREG | 0644);
	if (!inode)
		return -ENOMEM;
	inode_>i_fop = &simple_dir_operations;

	dentry = d_alloc(dir, &qname);
	if (!dentry)
		return -ENOMEM;

	d_add(dentry, inode);
	return dentry;
}

/* Helper function to create an inode */
static struct inode *ptree_make_inode(struct super_block *sb,
		int mode)
{
	struct inode *inode;
	inode = new_inode(sb);

	if (!inode)
		return -ENOMEM;

	inode->i_ino = 1;
	inode->i_mtime = inode->i_atime = inode->i_ctime = CURRENT_TIME;
	inode->i_mode = mode;
	inode->i_blksize = PAGE_CACHE_SIZE;
	inode->i_blocks = 0;
	inode->i_uid = inode->i_gid = 0;

	return inode;
}

static struct super_operations ptree_s_ops = {
	.statfs		= simple_statfs,
	.drop_inode	= generic_delete_inode,
};

static int ptree_fill_super(struct super_block *sb,
		void *data, int silent)
{
	struct inode *root;
	struct dentry *root_dentry;

	/* Set superblock parameters */
	sb->s_maxbytes		= MAX_LFS_FILESIZE;
	sb->s_blocksize		= PAGE_CACHE_SIZE;
	sb->s_blocksize_bits	= PAGE_CACHE_SHIFT;
	sb->s_magic		= PTREEFS_MAGIC;
	sb->s_op		= &ptree_s_ops;

	/* Create root inode */
	root = ptree_make_inode(sb, S_IFDIR | 0755);
	if (!root)
		return -ENOMEM;

	root->i_op	= &simple_dir_inode_operations;
	root->i_fop	= &simple_dir_operations;

	/* Create root dentry */
	root_dentry = d_make_root(root);	
	sb->s_root = root_dentry;
	if (!sb->s_root)
		return -ENOMEM;

	/* Create ptree files */
	ptree_create_files(sb, root_dentry);

	return 0;
}

static struct dentry *ptree_mount(struct file_system_type *fs_type,
		int flags, const char *dev_name,
		void * data)
{
	return mount_single(fs_type, flags, data, ptree_fill_super);
}

static struct file_system_type ptree_fs_type = {
	.owner		= THIS_MODULE,
	.name		= "ptreefs",
	.mount		= ptree_mount,
	.kill_sb	= kill_litter_super,
};

static int __init ptreefs_init(void)
{
	int err = register_filesystem(&ptree_fs_type);
	return err;
}

module_init(ptreefs_init)
