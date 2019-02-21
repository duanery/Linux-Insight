# params参数系统

> [![40](https://github.com/duanery/picture/blob/master/github/github_black_40px.png)](https://duanery.github.io)
> &nbsp;&nbsp;
> ***duanery*** &nbsp;
> 2019年2月21日
>
> Linux爱好者

内核params处理分为2类：内核参数，模块参数。

## 内核参数

### early_param,__setup

```c
static int __init loglevel(char *str)
{
	int newlevel;
	if (get_option(&str, &newlevel)) {
		console_loglevel = newlevel;
		return 0;
	}
	return -EINVAL;
}
early_param("loglevel", loglevel);
```

early_param通过定义参数名及处理函数来标识一个参数。

```c
static int __init init_setup(char *str)
{
	unsigned int i;
	execute_command = str;
	for (i = 1; i < MAX_INIT_ARGS; i++)
		argv_init[i] = NULL;
	return 1;
}
__setup("init=", init_setup);
```

__setup定义带值的参数时，需要加上'='字符，如“init=”。这种形式已经废弃不用了。

处理函数内部一般都是对一个全局变量赋值。

### 内部

参考在`include/linux/init.h`中对`early_param`及`__setup`的定义：

```c
struct obs_kernel_param {
	const char *str;
	int (*setup_func)(char *);
	int early;
};
#define __setup_param(str, unique_id, fn, early)			\
	static const char __setup_str_##unique_id[] __initconst		\
		__aligned(1) = str; 					\
	static struct obs_kernel_param __setup_##unique_id		\
		__used __section(.init.setup)				\
		__attribute__((aligned((sizeof(long)))))		\
		= { __setup_str_##unique_id, fn, early }
#define __setup(str, fn)						\
	__setup_param(str, fn, fn, 0)
#define early_param(str, fn)						\
	__setup_param(str, fn, fn, 1)
```

可以看到参数最终转换成`obs_kernel_param`类型存放在`.init.setup`section中了，在vmlinux.lds.h中可以看到

```c
#define INIT_SETUP(initsetup_align)					\
		. = ALIGN(initsetup_align);				\
		VMLINUX_SYMBOL(__setup_start) = .;			\
		*(.init.setup)						\
		VMLINUX_SYMBOL(__setup_end) = .;
```

.init.setup主要是由\__setup_start和__setup_end这两个符号标记。

early_param定义的参数主要是通过`parse_early_param`函数来解析，对每个参数都在\__setup_start和__setup_end区域内搜索，然后调用setup_func来处理参数。

## 模块参数

### module_param,core_param...

```c
#define module_param(name, type, perm)					...
#define core_param(name, var, type, perm)				...
#define module_param_string(name, string, len, perm)	...
#define module_param_array(name, type, nump, perm)	 	...
```

通过`module_param`这个宏，可以把一个type类型的变量(变量名为name），转换为一个模块参数。参数名就是变量名，参数解析完直接会对type类型的变量赋值。

总结：直接把一个变量转换为参数。比early_param及__setup解析完调用函数对变量赋值要简单的多。

一个例子：

```c
bool initcall_debug;
core_param(initcall_debug, initcall_debug, bool, 0644);
```

支持的type有：

```c
 * Standard types are:
 *	byte, short, ushort, int, uint, long, ulong
 *	charp: a character pointer
 *	bool: a bool, values 0/1, y/n, Y/N.
 *	invbool: the above, only sense-reversed (N = true).
 *  array: a parameter which is an array of some type
 *  string: a char array parameter
```

### TODO

- 每种类型的详细解析。
- modinfo命令看到的parm是如何解析的。
- sysfs
- 链接到内核的模块，及编译成ko文件的模块，在传递参数上的差异？链接到内核的模块的参数只能通过内核参数来指定且参数前要加上"模块名."，ko文件通过modprobe加载时指定参数，不需要带"模块名."。

### 内部

```c
#define core_param(name, var, type, perm)				\
	param_check_##type(name, &(var));				\
	__module_param_call("", name, &param_ops_##type, &var, perm, -1, 0)
#define __module_param_call(prefix, name, ops, arg, perm, level, flags)	\
	/* Default value instead of permissions? */			\
	static const char __param_str_##name[] = prefix #name;		\
	static struct kernel_param __moduleparam_const __param_##name	\
	__used								\
    __attribute__ ((unused,__section__ ("__param"),aligned(sizeof(void *)))) \
	= { __param_str_##name, THIS_MODULE, ops,			\
	    VERIFY_OCTAL_PERMISSIONS(perm), level, flags, { arg } }

```

仅仅分析core_param这个宏，这个宏转换成`kernel_param`类型的结构体存储在`__param`section中。

```c
struct kernel_param_ops {
	/* How the ops should behave */
	unsigned int flags;
	/* Returns 0, or -errno.  arg is in kp->arg. */
	int (*set)(const char *val, const struct kernel_param *kp);
	/* Returns length written or -errno.  Buffer is 4k (ie. be short!) */
	int (*get)(char *buffer, const struct kernel_param *kp);
	/* Optional function to free kp->arg when module unloaded. */
	void (*free)(void *arg);
};
struct kernel_param {
	const char *name;
	struct module *mod;
	const struct kernel_param_ops *ops;
	const u16 perm;
	s8 level;
	u8 flags;
	union {
		void *arg;
		const struct kparam_string *str;
		const struct kparam_array *arr;
	};
};
```

name存放变量名，也是模块参数名。

ops参数由type类型转换得到，每个支持的类型都定义一个kernel_param_ops类型的结构param_ops_##type。

arg参数存放变量的指针。

所有的`module_param`宏定义出来的`kernel_param`结构体都存放在`__param`section中，从vmlinux.lds.h中可以看到：

```c
	/* Built-in module parameters. */				\
	__param : AT(ADDR(__param) - LOAD_OFFSET) {			\
		VMLINUX_SYMBOL(__start___param) = .;			\
		*(__param)						\
		VMLINUX_SYMBOL(__stop___param) = .;			\
	}
```

`__param`section由\_\_start\_\_\_param和\_\_stop\_\_\_param两个符号标记。

## 解析参数

early_param通过`parse_early_param()`函数解析，最终是调用`parse_args`函数解析的。

模块参数则直接调用`parse_args`函数解析。

```c
	parse_early_param();
	after_dashes = parse_args("Booting kernel",
				  static_command_line, __start___param,
				  __stop___param - __start___param,
				  -1, -1, NULL, &unknown_bootoption);
```

`static_command_line`存放从boot传递过来的内核参数，类似"foo=bar,bar2 baz=fuz wiz"这样的格式。

parse_args循环解析每一个参数，得到参数名和参数值，然后在\_\_start\_\_\_param到\_\_stop\_\_\_param之间搜索该参数的名字，如果搜到则调用该参数的kernel_param 结构中ops->set函数设置参数值。如果未搜到则调用`unknown_bootoption`函数处理。

在`unknown_bootoption`函数中会解析用`__setup`宏定义的内核参数，如果在\__setup_start和__setup_end之间也未搜索到这个参数，那么这个参数将作为init进程的参数或者环境变量。

## 解析时机

内核参数在内核初始化函数`start_kernel`内部解析。

静态链接到内核中的模块，其带的参数也是在内核初始化时解析的。内核模块的参数指定要带上模块名，如：usbcore.blinkenlights=1。

外部模块的参数，是在模块加载时解析的。

## 参考文档

### 内核文档

Documentation/x86/x86_64/boot-options.txt

Documentation/kernel-parameters.txt