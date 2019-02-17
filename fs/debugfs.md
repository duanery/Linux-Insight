# debugfs

> [![40](https://github.com/duanery/picture/blob/master/github/github_black_40px.png)](https://duanery.github.io)
> &nbsp;&nbsp;
> ***duanery*** &nbsp;
> 2019年2月17日
> 
> Linux爱好者

debugfs是调试文件系统，必须很方便于调试。内核调试除了printk之外，还可以通过文件的方式来读取内核中的一个变量，读取内核的一个数组等。这些文件还可以存放到目录中来管理。

## 挂载

```bash
mount -t debugfs none /sys/kernel/debug
ls /sys/kernel/debug
```

## 文件API

| 函数                                                         | 描述                                                         |
| ------------------------------------------------------------ | ------------------------------------------------------------ |
| struct dentry *debugfs_create_u8(const char *name, umode_t mode, struct dentry *parent, u8 *value) | 把一个u8类型的内核变量转换为以name为名字的文件，存放在parent目录下。文件的访问模式是mode。 |
| struct dentry *debugfs_create_u16(const char *name, umode_t mode, struct dentry *parent, u16 *value) | 把一个u16类型的内核变量转换为以name为名字的文件，存放在parent目录下。文件的访问模式是mode。 |
| struct dentry *debugfs_create_u16(const char *name, umode_t mode, struct dentry *parent, u32 *value) | 把一个u32类型的内核变量转换为以name为名字的文件，存放在parent目录下。文件的访问模式是mode。 |
| struct dentry *debugfs_create_u16(const char *name, umode_t mode, struct dentry *parent, u64 *value) | 把一个u64类型的内核变量转换为以name为名字的文件，存放在parent目录下。文件的访问模式是mode。 |
| struct dentry *debugfs_create_ulong(const char *name, umode_t mode, struct dentry *parent, unsigned long *value) | 把一个ulong类型的内核变量转换为以name为名字的文件，存放在parent目录下。文件的访问模式是mode。 |
| struct dentry *debugfs_create_x8(const char *name, umode_t mode, struct dentry *parent, u8 *value) | 把一个u8类型的内核变量转换为以name为名字的文件，存放在parent目录下。文件的访问模式是mode。以`0x01`这样的16进制格式显示。 |
| struct dentry *debugfs_create_x16(const char *name, umode_t mode, struct dentry *parent, u16 *value) | 把一个u16类型的内核变量转换为以name为名字的文件，存放在parent目录下。文件的访问模式是mode。以`0x0001`这样的16进制格式显示。 |
| struct dentry *debugfs_create_x32(const char *name, umode_t mode, struct dentry *parent, u32 *value) | 把一个u32类型的内核变量转换为以name为名字的文件，存放在parent目录下。文件的访问模式是mode。以`0x00000001`这样的16进制格式显示。 |
| struct dentry *debugfs_create_x64(const char *name, umode_t mode, struct dentry *parent, u64 *value) | 把一个u64类型的内核变量转换为以name为名字的文件，存放在parent目录下。文件的访问模式是mode。以`0x0000000000000001`这样的16进制格式显示。 |
| struct dentry *debugfs_create_size_t(const char *name, umode_t mode, struct dentry *parent, size_t *value) | 把一个size_t类型的内核变量转换为以name为名字的文件，存放在parent目录下。文件的访问模式是mode。 |
| struct dentry *debugfs_create_atomic_t(const char *name, umode_t mode, struct dentry *parent, atomic_t *value) | 把一个atomic_t类型的内核变量转换为以name为名字的文件，存放在parent目录下。文件的访问模式是mode。 |
| struct dentry *debugfs_create_bool(const char *name, umode_t mode, struct dentry *parent, bool *value) | 把一个bool类型的内核变量转换为以name为名字的文件，存放在parent目录下。文件的访问模式是mode。true:"Y"，false:"N" |
| struct dentry *debugfs_create_blob(const char *name, umode_t mode, struct dentry *parent, struct debugfs_blob_wrapper *blob) | 参考源码                                                     |
| struct dentry *debugfs_create_u32_array(const char *name, umode_t mode, struct dentry *parent, u32 *array, u32 elements) | 把u32类型的数组转换为以name为名字的文件。文件内容："12 2345 3456789 0123456789" |
| struct dentry *debugfs_create_regset32(const char *name, umode_t mode, struct dentry *parent, struct debugfs_regset32 *regset) | 参考源码                                                     |

如果parent的值为NULL，则创建的文件在debugfs的挂载点目录下。

非以上类型的内核变量需要使用更原始的API：

| 函数                                                         | 描述                                   |
| ------------------------------------------------------------ | -------------------------------------- |
| struct dentry *debugfs_create_file(const char *name, umode_t mode, struct dentry *parent, void *data, const struct file_operations *fops) | 以fops来创建文件                       |
| struct dentry *debugfs_create_file_unsafe(const char *name, umode_t mode, struct dentry *parent, void *data, const struct file_operations *fops) | 以fops来创建文件                       |
| struct dentry *debugfs_create_file_size(const char *name, umode_t mode, struct dentry *parent, void *data, const struct file_operations *fops, loff_t file_size) | 以fops来创建文件，指定文件大小为size。 |

## 目录API

| 函数                                                         | 描述                                        |
| ------------------------------------------------------------ | ------------------------------------------- |
| struct dentry *debugfs_create_dir(const char *name, struct dentry *parent) | 在debugfs的parent目录下创建name名字的目录。 |

`debugfs_create_dir`的返回值可以传递给文件API或另一个目录API。

| 函数                                                         | 描述                                                         |
| ------------------------------------------------------------ | ------------------------------------------------------------ |
| struct dentry *debugfs_create_automount(const char *name, struct dentry *parent, debugfs_automount_t f, void *data) | 在debugfs的parent目录下创建name名字的目录，之后调用f指定的函数在该目录上挂载新的文件系统。 |

如果parent的值为NULL，则在debugfs的挂载点目录下创建目录。

## 移除API

| 函数                                                 | 描述                                                     |
| ---------------------------------------------------- | -------------------------------------------------------- |
| void debugfs_remove(struct dentry *dentry)           | 传递文件API和目录API的返回值到dentry，以删除文件或目录。 |
| void debugfs_remove_recursive(struct dentry *dentry) | 递归删除目录。                                           |

## 参考文档

### 内核文档

Documentation/filesystems/debugfs.txt