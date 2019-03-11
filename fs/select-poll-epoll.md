# EPOLL分析

> [![40](https://github.com/duanery/picture/blob/master/github/github_black_40px.png)](https://duanery.github.io)
> &nbsp;&nbsp;
> ***duanery*** &nbsp;
> 2019年3月11日
>
> Linux爱好者

分析一下`select, poll, epoll`这三者的共通点，如何获取文件描述符是否可读可写呢？

## 共通点

都是通过文件描述符fd的`struct file`结构的`f_op->poll`操作来获取文件可读可写信息的。下文把这个`f_op->poll`称为poll接口。

```c
struct file {
	...
	const struct file_operations	*f_op;
    ...
}
struct file_operations {
    ...
    unsigned int (*poll) (struct file *, struct poll_table_struct *);
    ...
}
```

既然通过poll接口获取信息的，就有必要分析一下这个接口了。poll的返回值就是可读可写的位掩码。

```c
#define POLLIN		0x0001
#define POLLPRI		0x0002
#define POLLOUT		0x0004
#define POLLERR		0x0008
#define POLLHUP		0x0010
#define POLLNVAL	0x0020
```

返回值是由上面这些位掩码组成。

再看看poll_table_struct的定义：

```c
typedef struct poll_table_struct {
	poll_queue_proc _qproc;
	unsigned long _key;
} poll_table;

static inline void poll_wait(struct file * filp, wait_queue_head_t * wait_address, poll_table *p)
{
	if (p && p->_qproc && wait_address)
		p->_qproc(filp, wait_address, p);
}
```

`poll_table`直译为轮询表，每个文件描述符的会定义一个等待队列（`wait_queue_head_t`），在poll接口内会调用poll_wait函数，把（filp，wait_address，p）都传递进去，主要是调用`_qproc`函数，对于`select, poll, epoll`会有不同的_qproc函数。

经过对`select, poll, epoll`的分析：

- `select, poll`的poll_wait主要是把`poll_table_entry`这个结构加入filp对应的等待队列中，当filp有数据时就调用唤醒回调函数`pollwake`，然后唤醒调用select和poll系统调用的进程P，P就继续再次读取filp文件的可读可写信息。可以看到select和poll系统调用内传递的fd越多，进程P就会加入到越多的等待队列上，任意一个filp有数据都会唤醒进程P。
- `epoll`的poll_wait主要把`eppoll_entry`这个结构加入filp对应的等待队列中，当filp有数据时就会唤醒回调函数`ep_poll_callback`，进而把filp加入到epoll的ready list中了，然后唤醒调用epoll系统调用的进程P，P然后读取ready list来获取可用的filp。任意一个filp有数据也会唤醒进程P。

