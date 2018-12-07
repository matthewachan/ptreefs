#ifndef _LINUX_PTREEFS_H
#define _LINUX_PTREEFS_H

extern void ptree_create_files(struct super_block *sb,
		struct dentry *root);
extern struct dentry *ptree_create_file(struct super_block *sb,
		struct dentry *dir, const char *name);
extern struct dentry *ptree_create_dir(struct super_block *sb,
		struct dentry *parent, const char *name);
extern void ptree_dfs(struct super_block *sb,
		struct dentry *root, struct task_struct *task);
extern struct inode *ptree_make_inode(struct super_block *sb,
		int mode);
extern struct super_operations ptree_s_ops;

extern int ptree_fill_super(struct super_block *sb,
		void *data, int silent);

extern struct dentry *ptree_mount(struct file_system_type *fs_type,
		int flags, const char *dev_name,
		void * data);
extern struct file_system_type ptree_fs_type;

extern int __init ptreefs_init(void);


#endif
