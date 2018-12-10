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

static void repslash(char* oristr)
{
        char *p = oristr;
        while(*p != '\0') {
                if (*p == '/')
                        *p = '-';
                p++;
        }
};

static int ptreefs_open_file(struct inode *inode, struct file *file)
{
	printk("I am open\n");
	return simple_open(inode, file);
};

static ssize_t ptreefs_read_file(struct file *file, char __user *buf,
size_t count, loff_t *ppos)
{
	printk("I am read\n");
	return 0;
};

static int ptreefs_rmdir(struct inode *dir, struct dentry *dentry)
{
	struct dentry *child;

	if (!simple_empty(dentry)) {
		list_for_each_entry(child, &dentry->d_subdirs, d_child) {
			spin_lock_nested(&child->d_lock, DENTRY_D_LOCK_NESTED);
			ptreefs_rmdir(child->d_inode, child);
			spin_unlock(&child->d_lock);
		}
	} else
		simple_rmdir(dir, dentry);

	return 0;
};

static int ptreefs_update(struct inode *inode, struct file *file)
{
	const char *name = file->f_path.dentry->d_name.name;
	struct dentry *child, *dentry = file->f_path.dentry;
	struct task_struct *p, *parent;
	long pid, c_pid, dir_pid;
	int flag;

	/*Find task by name (alternative for generality)*/
	if (kstrtol(name, 10, &pid))
		return -EFAULT;

	read_lock(&tasklist_lock);
	parent = find_task_by_vpid(pid);
	read_unlock(&tasklist_lock);

	/*Process missing handler*/
	if (!parent)
		return -EFAULT;

	read_lock(&tasklist_lock);
	spin_lock(&dentry->d_lock);
	/*travrse subdirs first*/
	list_for_each_entry(child, &dentry->d_subdirs, d_child) {
		spin_lock_nested(&child->d_lock, DENTRY_D_LOCK_NESTED);

		if (!(child->d_inode->i_mode & S_IFDIR))
			goto next;

		name = child->d_name.name;
		if (kstrtol(name, 10, &dir_pid)) {
			spin_unlock(&child->d_lock);
			spin_unlock(&dentry->d_lock);
			read_unlock(&tasklist_lock);
			printk("kstrtol failed\n");
			return -EFAULT;
		}

		p = find_task_by_vpid(dir_pid);

		if (!p) {
			printk("[%ld] terminates\n", dir_pid);
			//ptreefs_rmdir(child->d_inode, child);
		} else if (p->real_parent->pid != pid)
			printk("[%ld] reparenting\n", dir_pid);
		else {
			//printk("[%ld] not changed\n", dir_pid);
		}

next:
		spin_unlock(&child->d_lock);
	}

	/*traverse process children in pstree first*/
	list_for_each_entry(p, &parent->children, sibling) {
		c_pid = (long)task_pid_nr(p);
		flag = 0;

		list_for_each_entry(child, &dentry->d_subdirs, d_child) {
			spin_lock_nested(&child->d_lock, DENTRY_D_LOCK_NESTED);

			if (!(child->d_inode->i_mode & S_IFDIR))
				goto next_p;

			name = child->d_name.name;
			if (kstrtol(name, 10, &dir_pid)) {
				spin_unlock(&child->d_lock);
				spin_unlock(&dentry->d_lock);
				read_unlock(&tasklist_lock);
				printk("kstrtol failed\n");
				return -EFAULT;
			}

			if (c_pid == dir_pid)
				flag = 1;
next_p:
			spin_unlock(&child->d_lock);
		}
		/*need to create new dir*/
		if (flag == 0)
			printk("[%ld] newly generated\n", c_pid);
	}

	spin_unlock(&dentry->d_lock);
	read_unlock(&tasklist_lock);

	return 0;
};

int ptreefs_dir_open(struct inode *inode, struct file *file)
{
	static struct qstr cursor_name = QSTR_INIT(".", 1);
	struct dentry *dentry = file->f_path.dentry;
	const char *name = dentry->d_name.name;

	// if (inode->i_sb->s_root ==
	// 	file->f_path.dentry->d_parent) {
	printk("Dir[%s] open\n", name);
	if (!simple_empty(dentry))
		printk("Dir[%s] not empty\n", name);
	ptreefs_update(inode, file);
	// }
	file->private_data = d_alloc(file->f_path.dentry, &cursor_name);

	return file->private_data ? 0 : -ENOMEM;
};

static ssize_t ptreefs_write_file(struct file *file, const char __user *buf,
size_t count, loff_t *ppos)
{
	printk("I am write\n");
	return count;
};

const struct file_operations ptreefs_file_operations = {
	.open		= ptreefs_open_file,
	.read		= ptreefs_read_file,
	.write		= ptreefs_write_file,
};

const struct file_operations ptreefs_dir_operations = {
	.open		= ptreefs_dir_open,
	.release	= dcache_dir_close,
	.llseek		= dcache_dir_lseek,
	.read		= ptreefs_read_file,
	.iterate	= dcache_readdir,
	.fsync		= noop_fsync,
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
};

struct dentry *ptree_create_dir(struct super_block *sb,
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
	inode->i_fop = &ptreefs_dir_operations;

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

static void ptree_create_files(struct super_block *sb,
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
			/* Get PID for task */
			pid = (long)task_pid_nr(p);
			sprintf(s_pid, "%ld", pid);

			/* Create a directory for task */
			subdir = ptree_create_dir(sb, subdir, s_pid);

			get_task_comm(name, p);
                        repslash(name);
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
