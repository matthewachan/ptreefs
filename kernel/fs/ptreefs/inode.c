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
#include <linux/pagemap.h>

static ssize_t default_read_file(struct file *file, char __user *buf,
size_t count, loff_t *ppos)
{
	printk("I am read\n");
	return 0;
}

static ssize_t default_write_file(struct file *file, const char __user *buf,
size_t count, loff_t *ppos)
{
	printk("I am write\n");
	return count;
}

const struct file_operations ptreefs_file_operations = {
	.read =		default_read_file,
	.write =	default_write_file,
	.open =		simple_open,
};

struct inode *ptree_make_inode(struct super_block *sb,
			       int mode)
{
	struct inode *inode;
	inode = new_inode(sb);

	/* if (!inode) */
	/* 	return -ENOMEM; */

	inode->i_ino = get_next_ino();
	inode->i_mtime = inode->i_atime = inode->i_ctime = CURRENT_TIME;
	inode->i_mode = mode;
	inode->i_blkbits = PAGE_CACHE_SIZE;
	inode->i_blocks = 0;
	/* inode->i_uid = inode->i_gid = 0; */

	return inode;
}

struct dentry *ptree_create_dir (struct super_block *sb,
				 struct dentry *parent, const char *name)
{
	struct dentry *dentry;
	struct inode *inode;
	struct qstr qname;

	qname.name	= name;
	qname.len	= strlen (name);
	qname.hash	= full_name_hash(name, qname.len);

	dentry = d_alloc(parent, &qname);
	/* if (! dentry) */
	/* 	goto out; */

	inode = ptree_make_inode(sb, S_IFDIR | 0777);
	/* if (! inode) */
	/* 	goto out_dput; */

	inode->i_op = &simple_dir_inode_operations;
	inode->i_fop = &simple_dir_operations;

	d_add(dentry, inode);
	return dentry;

	/* out_dput: */
	/* 	dput(dentry); */
	/* out: */
	/* 	return 0; */
};

struct dentry *ptree_create_file(struct super_block *sb,
		struct dentry *dir, const char *name)
{
	struct inode *inode;
	struct dentry *dentry;
	struct qstr qname;

	qname.name	= name;
	qname.len	= strlen(name);
	qname.hash	= full_name_hash(name, qname.len);

	inode = ptree_make_inode(sb, S_IFREG | 0777);
	/* if (!inode) */
	/* 	return -ENOMEM; */
	inode->i_fop = &ptreefs_file_operations;

	dentry = d_alloc(dir, &qname);
	/* if (!dentry) */
	/* 	return -ENOMEM; */

	d_add(dentry, inode);
	return dentry;
};

void ptree_create_files(struct super_block *sb,
		struct dentry *root)
{
	char *name = kmalloc(TASK_COMM_LEN + 5, GFP_KERNEL);
	struct dentry *subdir = root;
	struct task_struct *init = &init_task;
	struct task_struct *p = init;
	long pid;
	char s_pid[6];

	bool going_up = false;

	read_lock(&tasklist_lock);
	while (!going_up || likely(p != init)) {
		if (!going_up) {
			/* Get PID for init_task */
			pid = (long)task_pid_nr(p);
			sprintf(s_pid, "%ld", pid);

			/* Create a directory for init_task */
			subdir = ptree_create_dir(sb, subdir, s_pid);

			get_task_comm(name, p);
			strcat(name, ".name");
			ptree_create_file(sb, subdir, name);

		}
		if (!going_up && !list_empty(&p->children)) {
			p = list_last_entry(&p->children, struct task_struct,
					sibling);
		} else if (p->sibling.prev != &p->real_parent->children) {
			p = list_last_entry(&p->sibling, struct task_struct,
					sibling);
			subdir = subdir->d_parent;
			going_up = false;
		} else {
			p = p->real_parent;
			subdir = subdir->d_parent;
			going_up = true;
		}
	}
	read_unlock(&tasklist_lock);
	kfree(name);
};

// void ptree_dfs(struct super_block *sb,
// 		struct dentry *root, struct task_struct *task)
// {
// 	/* Create a file using the processes' name */
// 	/* char name[TASK_COMM_LEN]; */
// 	char *name = kmalloc(TASK_COMM_LEN, GFP_KERNEL);
// 	struct list_head *p;
// 	struct task_struct *child;
// 	struct dentry *subdir;
// 	struct task_struct *init = task;
// 	struct task_struct *p = init;
// 	long pid;
// 	char s_pid[6];
// 	bool going_up = false;
//
// 	/* Create file in current directory */
// 	get_task_comm(name, p);
// 	ptree_create_file(sb, root, name);
//
// 	/* Create directories for children and recurse */
// 	list_for_each(p, &(task->children)) {
// 		child = list_entry(p, struct task_struct, sibling);
// 		pid = (long)task_pid_nr(child);
// 		sprintf(s_pid, "%ld", pid);
// 		subdir = ptree_create_dir(sb, root, s_pid);
// 		if (subdir)
// 			ptree_dfs(sb, subdir, child);
//
// 	}
// 	while (!going_up || likely(p != init)) {
// 		if (!going_up) {
//
// 		}
// 	}
// 	kfree(name);
//
// };


static int ptree_open(struct inode *inode, struct file *filp)
{
        printk("I am open\n");
        return 0;
}

static ssize_t ptree_read(struct file *filp, char *buf,
                             size_t count, loff_t *offset)
{
        printk("I am read\n");
        return 0;
}

static ssize_t ptree_write(struct file *filp, const char *buf,
                              size_t count, loff_t *offset)
{
        printk("I am write\n");
        return 0;
}

static struct file_operations ptreefs_file_ops = {
        .open	= ptree_open,
        .read 	= ptree_read,
        .write  = ptree_write,
};

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

        inode->i_ino = 1;
        inode->i_mtime = inode->i_atime = inode->i_ctime = CURRENT_TIME;
        inode->i_mode = S_IFDIR | S_IRUGO | S_IXUGO | S_IWUSR;
        inode->i_op = &simple_dir_inode_operations;
        //inode->i_fop = &ptreefs_file_ops;
	inode->i_fop = &simple_dir_operations;

        sb->s_root = d_make_root(inode);
        if (!sb->s_root)
                goto fail;
	ptree_create_files(sb, sb->s_root);
	return 0;

fail:
	pr_err("get root dentry failed\n");
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
