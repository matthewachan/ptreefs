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
#include <linux/parser.h>
#include <linux/fsnotify.h>
#include <linux/seq_file.h>

static const struct super_operations ptreefs_s_ops = {
        .statfs         = simple_statfs,
        .drop_inode     = generic_delete_inode,
};

static int ptree_fill_super(struct super_block *sb, void *data, int silent)
{
        int err;
        static struct tree_descr ptree_files[] = {{""}};
        struct inode *inode;

        err  =  simple_fill_super(sb, PTREEFS_MAGIC, ptree_files);
        if (err)
                goto fail;
        sb->s_op = &ptreefs_s_ops;

        inode = new_inode(sb);
        if (!inode)
                goto fail;

        inode->i_mtime = inode->i_atime = inode->i_ctime = CURRENT_TIME;
        inode->i_mode = S_IFDIR | S_IRUGO | S_IXUGO | S_IWUSR;
        inode->i_op = &simple_dir_inode_operations;
        inode->i_fop = &simple_dir_operations;

        sb->s_root = d_make_root(inode);
        if (sb->s_root)
                return 0;

        pr_err("get root dentry failed\n");

        return 0;

fail:
        return -ENOMEM;
}

static struct dentry *ptree_mount(struct file_system_type *fs_type,
                int flags, const char *dev_name,
                void *data)
{
        return mount_single(fs_type, flags, data, ptree_fill_super);
}

static struct file_system_type ptree_fs_type = {
        .owner          = THIS_MODULE,
        .name		= "ptreefs",
        .mount		= ptree_mount,
        .kill_sb	= kill_litter_super
};

static int __init ptreefs_init(void)
{
        return register_filesystem(&ptree_fs_type);
}
module_init(ptreefs_init);