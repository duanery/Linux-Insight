# 模块分析

> [![40](https://github.com/duanery/picture/blob/master/github/github_black_40px.png)](https://duanery.github.io)
> &nbsp;&nbsp;
> ***duanery*** &nbsp;
> 2019年7月20日
>
> Linux爱好者

从模块编译、安装到模块加载。

# 一、使用方法

## 模块命令

| 命令                   | 介绍                                       |
| ---------------------- | ------------------------------------------ |
| /usr/bin/kmod          | 模块管理命令，包含list,load,unload模块     |
| /usr/sbin/depmod       | 为modprobe输出一个模块依赖关系列表         |
| /usr/sbin/insmod       | 插入模块                                   |
| /usr/sbin/lsmod        | 列出模块                                   |
| /usr/sbin/modinfo      | 查看模块信息                               |
| /usr/sbin/modprobe     | 加载模块，会根据模块依赖关系加载一系列模块 |
| /usr/sbin/rmmod        | 移除模块                                   |
| /usr/sbin/weak-modules |                                            |

## 模块用法

一个简单的模块：包含了模块的各种用途。

### hello模块

''hello.c" 源码文件，编译后会得到hello.ko模块：

```c
//hello.c
#include<linux/init.h>
#include<linux/module.h>
#include <linux/pci.h>

static int hello; 
module_param(hello, int, 0644);
static struct module *this = THIS_MODULE;
static const struct pci_device_id hello_pci_tbl[] = {
	{ 0x8086, 0x7010, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 },
	{ },
};

void hello_export()
{
	printk("export by %s\n", this->name);
}
EXPORT_SYMBOL(hello_export);

static int hello_init(void)
{
	printk("hello world\n");
	return 0;
}
static void hello_exit(void)
{
	printk("goodbye\n");
}
module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("duanery");
MODULE_DEVICE_TABLE(pci, hello_pci_tbl);
MODULE_DESCRIPTION("Hello World!");
```

这是一个最简单的模块，包含了导出符号，device_table，模块参数等等。

### hello_dep模块

"hello_dep.c" 源码文件，编译后得到hello_dep.ko模块：

```c
//hello_dep.c
#include<linux/init.h>
#include<linux/module.h>
extern void hello_export();
static int hello_dep_init(void)
{
	hello_export();
	return 0;
}
static void hello_dep_exit(void)
{
	printk("goodbye\n");
}

module_init(hello_dep_init);
module_exit(hello_dep_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("duanery");
MODULE_DESCRIPTION("Hello World!");
```

调用hello模块中导出的函数hello_export()。

### 编译脚本

```makefile
#Makefile
src_c := $(filter-out %.mod.c,$(notdir $(wildcard $(src)/*.c)))

hostprogs-y := 
always := $(hostprogs-y)

hostprogs_c := $(patsubst %,%.c,$(hostprogs-y))

obj_c := $(filter-out $(hostprogs_c), $(src_c))
obj-m := $(patsubst %.c,%.o,$(obj_c))

PHONE += modules
modules:
modules clean modules_install help:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) $@
```

键入`make`编译。

键入`make clean`清理。

键入`make modules_install`安装模块。

键入`modprobe hello_dep`插入模块到内核。

后续会根据这个例子，讲解模块编译、安装、加载。

# 二、模块编译

## 配置项

- CONFIG_MODULES

  打开模块功能

- CONFIG_MODVERSIONS

  对模块内导出的符号进行crc校验。未导出的符号无crc。

## section

在模块加载时会查看模块内所有特殊功能的section，并与内核建立联系。例如：

1. 模块内定义的percpu变量，会在模块加载分配内存空间。
2. 模块内使用copy_from_user等类似的宏，会建立异常跳转表，也需要特殊处理，在内核收到异常时能判断在哪个模块的上下文中。
3. 模块内使用的static_key，也需要特殊处理。

分析哪些宏会向模块内引入哪些section会很有意义。

### 1. 模块宏定义引入的section

- EXPORT_SYMBOL

  - **__kcrctab**：unsigned long，符号的crc值。

  - __ksymtab_strings：string,符号名字，字符串文本

  - **__ksymtab**：struct kernel_symbol，符号地址和name结构体.

  - ```c
    struct kernel_symbol
    {
    	unsigned long value;
    	const char *name;
    };
    ```

- EXPORT_SYMBOL_GPL

  - **__kcrctab_gpl**：unsigned long，符号的crc值。
  - __ksymtab_strings：符号名字，字符串文本
  - **__ksymtab_gpl**：struct kernel_symbol，符号地址和name结构体

- module_param(name, type, perm)

  - **\_\_param**：struct kernel_param，模块参数。

  - **.modinfo**：string，参数的信息。由`paramtype=name:type`(name,参数名;type,参数类型)这样的字符串组成。

  - ```c
    module_param(test, int, 0644) =>
    "paramtype=test:int"
    ```

- MODULE_ALIAS(_alias)

  - .modinfo：string。在.modinfo中增加`alias=_alias`这样的字符串。

- MODULE_LICENSE(_license)

  - .modinfo：string。在.modinfo中增加`license=_license`这样的字符串。

- MODULE_AUTHOR(_author)

  - .modinfo：string。在.modinfo中增加`author=_author`这样的字符串。

- MODULE_DESCRIPTION(_description)

  - .modinfo：string。在.modinfo中增加`description=_description`这样的字符串。

- MODULE_PARM_DESC(_parm, desc)

  - .modinfo：string。在.modinfo中增加`parm=_parm:desc`这样的字符串。

  - ```c
    MODULE_PARM_DESC(test, this a test) =>
    "parm=test:this a test\0"
    ```

- MODULE_DEVICE_TABLE(type,name)

  - 不生成scetion，而是生成\`type\'\_device\_id类型的数组，数组名为\_\_mod\_\`type\'\_device\_table。
  - 一般用于定义驱动可以识别的设备表。

- MODULE_VERSION(_version)

  - .modinfo：string。在.modinfo中增加`version=_version`这样的字符串。

- module_init(initfn)

  - 把init_module函数设置别名"initfn"

- module_exit(exitfn)

  - 把cleanup_module函数设置别名"exitfn"

- 。。。随着内核版本更新会有更多。

- 看看hello模块的信息，大部分宏的信息可以通过modinfo查看。

  ```
  [root@localhost module]# modinfo hello
  filename:       /lib/modules/3.10.0-957.el7.x86_64/extra/hello.ko
  description:    Hello World!
  author:         duanery
  license:        GPL
  retpoline:      Y
  rhelversion:    7.6
  srcversion:     56C405DE508ED8E3C5E62DF
  alias:          pci:v00008086d00007010sv*sd*bc*sc*i*
  depends:        
  vermagic:       3.10.0-957.el7.x86_64 SMP mod_unload modversions
  ```

### 2.模块调用内核接口引入的section

TODO

### 3.".mod.c"中引入的section

- .gnu.linkonce.this_module：这个section内唯一存放了一个struct module _\_this\_module变量。
- __versions：存放由当前模块调用的别的模块导出函数的版本信息。
- .modinfo：存放"depends=..."这样的依赖关系字符串，存放"vermagic=VERMAGIC_STRING"这样的版本字符串，存放"alias=..."模块别名字符串，存放"srcversion=..."这样的源码版本字符串。

## 编译第一阶段

这一阶段把模块需要的.o文件编译出来。主要是用`scripts/Makefile.build`脚本来编译。

.o文件依赖于.c文件。

```makefile
$(single-used-m): $(obj)/%.o: $(src)/%.c FORCE
	$(call cmd,force_checksrc)
	$(call if_changed_rule,cc_o_c)
	@{ echo $(@:.o=.ko); echo $@; } > $(MODVERDIR)/$(@F:.o=.mod)
```

最终会调用`rule_cc_o_c`来编译每个.c文件。编译完成后，会向MODVERDIR指定的目录下创建一个.mod文件。

**MODVERDIR**：

```makefile
export MODVERDIR := $(if $(KBUILD_EXTMOD),$(firstword $(KBUILD_EXTMOD))/).tmp_versions
```

这是内核编译过程中记录最终需要进入第二阶段处理的模块。一般存放在内核源码树的根目录.tmp_versions文件夹下，或者外部模块的根目录.tmp_versions文件夹下。

```makefile
define rule_cc_o_c
	$(call echo-cmd,checksrc) $(cmd_checksrc)			  \
	$(call echo-cmd,cc_o_c) $(cmd_cc_o_c);				  \
	$(cmd_modversions)						  \
	$(call echo-cmd,record_mcount)					  \
	$(cmd_record_mcount)						  \
	scripts/basic/fixdep $(depfile) $@ '$(call make-cmd,cc_o_c)' >    \
	                                              $(dot-target).tmp;  \
	rm -f $(depfile);						  \
	mv -f $(dot-target).tmp $(dot-target).cmd
endef
```

又依赖于`cmd_cc_o_c及cmd_modversions`命令。

```makefile
ifndef CONFIG_MODVERSIONS
cmd_cc_o_c = $(CC) $(c_flags) -c -o $@ $<

else
# When module versioning is enabled the following steps are executed:
# o compile a .tmp_<file>.o from <file>.c
# o if .tmp_<file>.o doesn't contain a __ksymtab version, i.e. does
#   not export symbols, we just rename .tmp_<file>.o to <file>.o and
#   are done.
# o otherwise, we calculate symbol versions using the good old
#   genksyms on the preprocessed source and postprocess them in a way
#   that they are usable as a linker script
# o generate <file>.o from .tmp_<file>.o using the linker to
#   replace the unresolved symbols __crc_exported_symbol with
#   the actual value of the checksum generated by genksyms

cmd_cc_o_c = $(CC) $(c_flags) -c -o $(@D)/.tmp_$(@F) $<
cmd_modversions =								\
	if $(OBJDUMP) -h $(@D)/.tmp_$(@F) | grep -q __ksymtab; then		\
		$(call cmd_gensymtypes,$(KBUILD_SYMTYPES),$(@:.o=.symtypes))	\
		    > $(@D)/.tmp_$(@F:.o=.ver);					\
										\
		$(LD) $(LDFLAGS) -r -o $@ $(@D)/.tmp_$(@F) 			\
			-T $(@D)/.tmp_$(@F:.o=.ver);				\
		rm -f $(@D)/.tmp_$(@F) $(@D)/.tmp_$(@F:.o=.ver);		\
	else									\
		mv -f $(@D)/.tmp_$(@F) $@;					\
	fi;
endif
```

可以看到cmd_cc_o_c依赖于CONFIG_MODVERSIONS这个配置项。主要分析一下该配置项打开的情况：

1. \<file>.c文件先编译为.tmp_\<file>.o文件

2. 调用cmd_modversions命令，这个命令会过滤.tmp_\<file>.o文件中是否有调用`EXPORT_SYMBOL`等宏导出符号：

   1. 如果有导出符号，则调用cmd_gensymtypes命令计算所有导出符号的crc值，并输出到.tmp_\<file>.ver文件中，最终把该文件和.tmp\_\<file>.o一起链接，输出到\<file>.o文件中。

      ```makefile
      GENKSYMS	= scripts/genksyms/genksyms
      cmd_gensymtypes =                                                           \
          $(CPP) -D__GENKSYMS__ $(c_flags) $< |                                   \
          $(GENKSYMS) $(if $(1), -T $(2)) -a $(ARCH)                              \
           $(if $(KBUILD_PRESERVE),-p)                                            \
           -r $(firstword $(wildcard $(2:.symtypes=.symref) /dev/null))
      ```

      调用`scripts/genksyms/genksyms`命令来生成，具体生成比较复杂，详细分析：TODO。但可以看看生成的.tmp_\<file>.ver文件：

      ```
      [root@localhost module]# cat .tmp_hello.ver 
      __crc_hello_export = 0x33a9e426 ;
      ```

      经过推理：`scripts/genksyms/genksyms`这个命令应该是调用词法分析器对c语言源码进行分析，把导出的函数拆分为语法树，对语法树进行crc计算。因此在不改变函数的逻辑的情况下crc值是一直相同的。

   2. 如果无导出符号，则直接重名名.tmp_\<file>.o为\<file>.o

**总结**

这一阶段主要是.c编译为.o的过程，以及导出符号的crc计算。

## 编译第二阶段：modpost

无论是外部模块编译，还是内核编译，都会进入到第二阶段：modpost。

这一阶段主要是计算模块的依赖关系。生成.mod.c文件。链接生成.ko模块文件。

### step1

从MODVERDIR中提取总共有多少模块需要在第二阶段处理。全部模块的路径名都找出来。

### step2

调用`scripts/mod/modpost`命令，并为每个模块输出.mod.c文件。mod.c文件包含以下信息：

1. vermagic，由宏`MODULE_INFO(vermagic, VERMAGIC_STRING);`生成。
2. THIS_MODULE，定义了一个`struct module __this_module`变量，并存放到section。
3. \_\_\_\_versions[]，`struct modversion_info`结构体数组，存放由该模块引用的其他模块的函数的版本信息，本模块调用其他模块导出的函数(EXPORT_SYMBOL)，会记录其引用的其他模块的函数的crc值及name。这些函数在本模块中都是UNDEF的，因此分析UNDEF的符号并查找这些符号在哪个模块中定义，如果能查找到就记录crc值。这个结构体数组存放在`__versions`section中。
4. depends，模块间的依赖关系。本模块调用了其他模块导出的函数，就意味着本模块依赖其他模块。modpost能找出本模块依赖的所有模块。
5. device_table，模块内使用MODULE_DEVICE_TABLE宏定义一个表后，会产生这样的信息。一般驱动模块会定义这样的表，以示该模块能驱动是硬件设备。modpost会利用这个表生产模块别名。由MODULE_ALIAS引用。
6. srcversion，由宏`MODULE_INFO(srcversion, ...）`生成，定义模块的版本信息，详细生产方法：TODO

#### 模块依赖关系

​	由模块导出的符号，以及模块调用其他模块导出的符号，这样2个信息来取得模块的依赖关系。

​	模块导出的符号会放到__ksymtab这样的section中，遍历模块.o文件的符号表，可以直接获取到。调用其他模块的符号，在模块.o文件的符号表中是UNDEF的。

​	因此，读取内核的全部模块，对这些模块导出符号建立全局哈希表。然后对每一个模块，遍历其UNDEF的符号，在哈希表内查找该符号在哪个模块定义，由此可以建立这个模块依赖的所有模块。

#### example

```c
[root@localhost module]# cat hello.mod.c 
#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);  //1

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {  //2
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]  //3
__used
__attribute__((section("__versions"))) = {
	{ 0x28950ef1, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x15692c87, __VMLINUX_SYMBOL_STR(param_ops_int) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =  //4
"depends=";

MODULE_ALIAS("pci:v00008086d00007010sv*sd*bc*sc*i*");  //5

MODULE_INFO(srcversion, "9DDB9793EE1E6086188E19A");  //6
MODULE_INFO(rhelversion, "7.6");
#ifdef RETPOLINE
	MODULE_INFO(retpoline, "Y");
#endif
#ifdef CONFIG_MPROFILE_KERNEL
	MODULE_INFO(mprofile, "Y");
#endif
```

modpost为其中一个模块生成出.mod.c文件。

#### 外部模块

内核模块编译完后，会把vmlinux及内核模块导出的全部符号写入Module.symvers文件。外部模块在编译时，先从这个文件加载全部导出的符号，然后根据外部模块自己的导出符号和UNDEF符号，来确定模块依赖关系，生成.mod.c文件。

Module.symvers文件在`/lib/modules/$(uname -r)/build/`目录。

### step3

编译.mod.c文件。

### step4

链接.mod.o和\<file>.o文件为\<file>.ko文件。

链接脚本是"scripts/module-common.lds"，内核在3.0版本开始，会把"\_\_\_ksymtab+*"这类的符号链接到"\_\_ksymtab"中。

### step5

模块签名：TODO

# 三、模块安装

## modules_install

键入`make modules_install`进行模块安装：

主Makefile中的安装入口

```makefile
MODLIB	= $(INSTALL_MOD_PATH)/lib/modules/$(KERNELRELEASE)
modules_install: _emodinst_ _emodinst_post

install-dir := $(if $(INSTALL_MOD_DIR),$(INSTALL_MOD_DIR),extra)
PHONY += _emodinst_
_emodinst_:
	$(Q)mkdir -p $(MODLIB)/$(install-dir)
	$(Q)$(MAKE) -f $(srctree)/scripts/Makefile.modinst

PHONY += _emodinst_post
_emodinst_post: _emodinst_
	$(call cmd,depmod)
```

依赖于`_emodinst_及_emodinst_post`；

\_emodinst\_:

1. 创建/lib/modules/$(uname -r)/extra目录

2. 调用Makefile.modinst安装模块

   ```makefile
   __modules := $(sort $(shell grep -h '\.ko$$' /dev/null $(wildcard $(MODVERDIR)/*.mod)))
   modules := $(patsubst %.o,%.ko,$(wildcard $(__modules:.ko=.o)))
   PHONY += $(modules)
   __modinst: $(modules)
   	@:
   
   # Don't stop modules_install if we can't sign external modules.
   quiet_cmd_modules_install = INSTALL $@
         cmd_modules_install = \
       mkdir -p $(2) ; \
       cp $@ $(2) ; \
       $(mod_strip_cmd) $(2)/$(notdir $@) ; \
       $(mod_sign_cmd) $(2)/$(notdir $@) $(patsubst %,|| true,$(KBUILD_EXTMOD)) && \
       $(mod_compress_cmd) $(2)/$(notdir $@)
   
   # Modules built outside the kernel source tree go into extra by default
   INSTALL_MOD_DIR ?= extra
   ext-mod-dir = $(INSTALL_MOD_DIR)$(subst $(patsubst %/,%,$(KBUILD_EXTMOD)),,$(@D))
   
   modinst_dir = $(if $(KBUILD_EXTMOD),$(ext-mod-dir),kernel/$(@D))
   
   $(modules):
   	$(call cmd,modules_install,$(MODLIB)/$(modinst_dir))
   ```

   1. 先获取所有安装的模块名。
   2. 调用cmd_modules_install进行安装。主要是cp命令。分2种情况，如果安装的是内核源码树中的模块，会安装到`/lib/modules/$(uname -r)/kernel/`目录下。如果安装的是外部模块，会安装到`/lib/modules/$(uname -r)/extra/`目录下。

_emodinst_post:

1. 调用depmod。

## 总结

1. 模块安装主要是知道模块安装的目录：`/lib/modules/$(uname -r)/kernel/`或`/lib/modules/$(uname -r)/extra/`
2. 模块安装后会调用depmod进行模块依赖处理。使用modprobe就可以直接加载有多重依赖关系的模块。

# 四、模块加载

运行`insmod hello`或`modprobe hello_dep`就可以直接加载模块。

模块加载主要是利用`init_module或finit_module`这两个系统调用来载入内核。

有几点疑问：

1. 模块载入内核后.ko文件是否可以删除？
2. 模块内调用的其他模块的函数是否必须是EXPORT_SYMBOL导出的符号？
3. 模块加载失败是因为哪些原因？

随着分析这些疑问就会被解答。

## init_module

```c

SYSCALL_DEFINE3(init_module, void __user *, umod,
		unsigned long, len, const char __user *, uargs)
{
	int err;
	struct load_info info = { };

	err = may_init_module();
	if (err)
		return err;
	err = copy_module_from_user(umod, len, &info);
	if (err)
		return err;

	return load_module(&info, uargs, 0);
}
```

init_module系统调用需要用户程序先读出模块文件二进制数据，再传递给内核：

1. void __user umod，模块文件二进制数据。
2. unsigned long len，文件长度。
3. const char __user* uargs，用户传递过来的模块参数。

内核在may_init_module()函数内先判断用户进程是否有`CAP_SYS_MODULE`能力，之后把数据拷贝到内核态。

调用load_module()加载模块。

## finit_module

同init_module系统调用，只是模块的数据从用户态传递过来的文件描述符读取。多增加一个flags参数。

flags:

## load_module

完全的分析这个函数。

```c
static int load_module(struct load_info *info, const char __user *uargs,
		       int flags)
{
	struct module *mod;
	long err;
	char *after_dashes;
```

### 1. 检查模块签名

```c
	err = module_sig_check(info, flags);
	if (err)
		goto free_copy;
```

签名放在模块文件的末尾。分析：TODO。

### 2. 检查模块是否是elf格式

```c
	err = elf_header_check(info);
	if (err)
		goto free_copy;
```

### 3. 模块布局和分配内存

```c
	/* Figure out module layout, and allocate all the memory. */
	mod = layout_and_allocate(info, flags);
	if (IS_ERR(mod)) {
		err = PTR_ERR(mod);
		goto free_copy;
	}
```

这个函数会分析模块内的所有section，并为需要的section安排内存地址，然后为这些section申请内存，并拷贝到内存中。

```c
static struct module *layout_and_allocate(struct load_info *info, int flags)
{
    struct module *mod;
```

#### 3.1 设置info信息

```c
	mod = setup_load_info(info, flags);
	if (IS_ERR(mod))
		return mod;
```

在setup_load_info()函数内部：

1. 修改所有section hdr中sh_addr地址，模块编译成ko后并未真正的链接，所有sh_addr的值都是0，这里会把sh_addr的值修改为模块内核起始地址加上section在模块文件内的便宜量。

2. 查找"__versions" section。

3. 查找".modinfo" section。

4. 查找".gnu.linkonce.this_module" section。这个section存放了struct module结构体。

5. 查找".data..percpu" section。

6. 检查模块相关结构体的版本信息。利用导出函数会计算crc信息这点，module.c文件中定义一个module_layout的函数，把模块相关的结构体作为函数的参数，并导出这个函数。在编译module.c文件时就会计算module_layout函数的crc值。在编译模块文件生成.mod.c文件中"__version"section中会包含module_layout的crc信息。比对内核中module_layout函数的crc值，和模块中引用module_layout函数的crc信息就能知道模块相关的结构体是否匹配。

   crc不一致，模块加载失败。意味着模块是在其他内核源码树上编译的，不能加载到当前内核上，模块相关的结构体有变化。

7. 返回struct module结构体。

#### 3.2 模块黑名单

```c
	if (blacklisted(mod->name))
		return ERR_PTR(-EPERM);
```

模块在黑名单中，加载失败。

#### 3.3 检查vermagic及licence信息

```c
	err = check_modinfo(mod, info, flags);
	if (err)
		return ERR_PTR(err);
```

vermagic信息在.mod.c中通过`MODULE_INFO(vermagic, VERMAGIC_STRING);`宏生成。VERMAGIC_STRING，主要是内核的配置项组成的一个字符串，配置项打开对应字符串多一个值。vermagic主要是判别模块和内核的配置是否一致。

不一致，模块加载失败。意味着编译模块的机器的内核配置项和当前内核不一致，不能加载。

#### 3.4 

```c
	ndx = find_sec(info, ".data..ro_after_init");
	if (ndx)
		info->sechdrs[ndx].sh_flags |= SHF_RO_AFTER_INIT;
```

#### 3.5 section布局

```c
	layout_sections(mod, info);
```

section分两类：core，init。使用core_layout和init_layout结构表示。

core和init都按照`"| executable | RO | RO after init | WR | . | symtab | strtab |"`这样的顺序排列section，只是init都是以".init"开头的section。

布局完成后，模块内的每一个section都有一个相对位置。

#### 3.6 symtab布局

```c
	layout_symtab(mod, info);
```

在打开CONFIG_KALLSYMS配置的情况下，需要保存模块内的符号表。layout_symtab()函数识别出模块符号表中哪些符号需要保留，计算这些符号占用的内存大小，并布局符号的相对位置。

哪些符号不需要保留？

1. ".init"开头的section中的符号都不保留。
2. 不占内存的符号不保留。
3. 不是可执行代码段中的符号不保留。
4. UNDEF的符号不保留。

#### 3.7 移动模块

```c
	/* Allocate and move to the final place */
	err = move_module(mod, info);
	if (err)
		return ERR_PTR(err);
```

根据3.5和3.6中的布局信息，分配内存，并把需要的section移动到新分配的内存中。

#### 3.8 获取mod

```c
	/* Module has been copied to its final place now: return it. */
	mod = (void *)info->sechdrs[info->index.mod].sh_addr;
	kmemleak_load_module(mod, info);
	return mod;
```

".gnu.linkonce.this_module" section也被移动了，重新获得新的struct module结构体。

#### 3.9 返回

```c
}  //layout_and_allocate
```

layout_and_allocate()函数返回。

这个函数主要就是布局和移动，并返回struct module结构体。

### 4. 模块链表

```	/* Reserve our place in the list. */
	err = add_unformed_module(mod);
	if (err)
		goto free_module;
```

把模块插入modules链表。`static LIST_HEAD(modules);`modules是链表表头。

### 5. 分配percpu内存

```c
	/* To avoid stressing percpu allocator, do this once we're unique. */
	err = percpu_modalloc(mod, info);
	if (err)
		goto unlink_mod;
```

为模块中的percpu变量分配内存。如果模块没有定义percpu变量则不分配。

### 6. 模块卸载相关的结构体初始化

```c
	/* Now module is in final location, initialize linked lists, etc. */
	err = module_unload_init(mod);
	if (err)
		goto unlink_mod;
```

在CONFIG_MODULE_UNLOAD配置打开的情况下，模块可以卸载。module_unload_init()初始化卸载相关参数。

### 7. 查找模块各类section

```c
	/* Now we've got everything in the final locations, we can
	 * find optional sections. */
	err = find_module_sections(mod, info);
	if (err)
		goto free_unload;
```

查找各个section：

| section              | 描述                             |
| -------------------- | -------------------------------- |
| __param              | 模块参数，由module_param宏引入。 |
| __ksymtab            | EXPORT_SYMBOL导出的符号。        |
| __kcrctab            |                                  |
| __ksymtab_gpl        |                                  |
| __kcrctab_gpl        |                                  |
| __ksymtab_gpl_future |                                  |
| __kcrctab_gpl_future |                                  |
| __ksymtab_unused     |                                  |
| __kcrctab_unused     |                                  |
| __ksymtab_unused_gpl |                                  |
| __kcrctab_unused_gpl |                                  |
| __tracepoints_ptrs   |                                  |
| __jump_table         |                                  |
| _ftrace_events       |                                  |
| _ftrace_enum_map     |                                  |
| __trace_printk_fmt   |                                  |
| __mcount_loc         |                                  |
| __ex_table           |                                  |
| __obsparm            |                                  |
| __verbose            |                                  |

### 8. TODO

```c
	err = check_module_license_and_versions(mod);
	if (err)
		goto free_unload;
```

### 9. 

```c
	/* Set up MODINFO_ATTR fields */
	setup_modinfo(mod, info);
```

### 10. 修复符号

```c
	/* Fix up syms, so that st_value is a pointer to location. */
	err = simplify_symbols(mod, info);
	if (err < 0)
		goto free_modinfo;
```

1. 对于UNDEF的符号会使用resolve_symbol()函数解析符号，赋值给st_value。具体的：
   1. 使用find_symbol()查找符号；会查找内核中导出的符号，以及其他模块导出的符号。如果找不到，模块加载失败。
   2. 使用check_version()检查符号crc信息是否匹配。如果crc不匹配，模块加载失败。
   3. ref_module()相互引用。本模块依赖于符号定义的模块。会增加依赖模块的引用计数。在add_module_usage()函数中处理，源模块和目标模块的相互索引。
2. 对应已经定义的符号，则st_value会加上符号所在section的基地址。默认.ko中的符号值都是基于0地址的，模块被加载到内核，符号值需要相应的偏移。

### 11. 重定位

```c
	err = apply_relocations(mod, info);
	if (err < 0)
		goto free_modinfo;
```

### 12. 重定位后续处理

```c
	err = post_relocation(mod, info);
	if (err < 0)
		goto free_modinfo;
```

主要做几个事：

1. 排序异常表。"__ex_table"定义的section。
2. percpu变量的安装。把percpu变量的初始化拷贝到各个cpu上。
3. add_kallsyms。在打开CONFIG_KALLSYMS配置的情况下，把需要保留的符号拷贝到core_layout的末尾。
4. module_finalize。处理".altinstructions"，".smp_locks"，".parainstructions"，"__jump_table"这几个section。

### 13. 刷新指令cache

```c
	flush_module_icache(mod);
```

x86无操作。

### 14. 拷贝模块参数

```c
	/* Now copy in args */
	mod->args = strndup_user(uargs, ~0UL >> 1);
	if (IS_ERR(mod->args)) {
		err = PTR_ERR(mod->args);
		goto free_arch_cleanup;
	}
```

### 15. TODO

```c

	dynamic_debug_setup(info->debug, info->num_debug);

	/* Ftrace init must be called in the MODULE_STATE_UNFORMED state */
	ftrace_module_init(mod);
```

### 16. 

```c
	/* Finally it's fully formed, ready to start executing. */
	err = complete_formation(mod, info);
	if (err)
		goto ddebug_cleanup;
```

完成几个事情：

1. 检查本模块导出的符号，是否已经在其他模块导出过了。如是，模块加载失败。
2. 只读和进制执行的权限配置。例如：读写数据对应的区域是要进制执行的。
3. mod->state = MODULE_STATE_COMING;

### 17. 

```c
	err = prepare_coming_module(mod);
	if (err)
		goto bug_cleanup;
```

调用模块的通知链。

### 18. 解析模块参数

```c
	/* Module is ready to execute: parsing args may do that. */
	after_dashes = parse_args(mod->name, mod->args, mod->kp, mod->num_kp,
				  -32768, 32767, mod,
				  unknown_module_param_cb);
	if (IS_ERR(after_dashes)) {
		err = PTR_ERR(after_dashes);
		goto coming_cleanup;
	} else if (after_dashes) {
		pr_warn("%s: parameters '%s' after `--' ignored\n",
		       mod->name, after_dashes);
	}
```

处理模块参数，跟解析内核参数是一样的。

### 19. sysfs

```c
	/* Link in to syfs. */
	err = mod_sysfs_setup(mod, info, mod->kp, mod->num_kp);
	if (err < 0)
		goto coming_cleanup;
```

在/sys/module目录下建立模块对应的目录。

```bash
[root@localhost module]# ll /sys/module/kvm/
total 0
-r--r--r--. 1 root root 4096 Jul 21 16:01 coresize
drwxr-xr-x. 2 root root    0 Jul 21 12:11 holders
-r--r--r--. 1 root root 4096 Jul 21 16:01 initsize
-r--r--r--. 1 root root 4096 Jul 21 16:01 initstate
drwxr-xr-x. 2 root root    0 Jul 21 16:01 notes
drwxr-xr-x. 2 root root    0 Jul 21 16:01 parameters
-r--r--r--. 1 root root 4096 Jul 21 16:01 refcnt
-r--r--r--. 1 root root 4096 Jul 21 16:01 rhelversion
drwxr-xr-x. 2 root root    0 Jul 21 16:01 sections
-r--r--r--. 1 root root 4096 Jul 21 16:01 srcversion
-r--r--r--. 1 root root 4096 Jul 21 16:01 taint
--w-------. 1 root root 4096 Jul 21 16:01 uevent
```

coresize：core_layout的大小

initsize：init_layout的大小

initstate：模块状态，有`live,coming,going`这几个值。

parameters：目录，存放模块的各个参数。

refcnt：模块的引用计数

rhelversion：版本

sections：目录，存放模块的各个section。

srcversion：源码版本号

### 20. TODO

```c
	if (is_livepatch_module(mod)) {
		err = copy_module_elf(mod, info);
		if (err < 0)
			goto sysfs_cleanup;
	}
```

### 21. 最后的初始化

```c
return do_init_module(mod);
```

模块加载到最后，调用模块init函数，是否init_layout。

- ```c
  static noinline int do_init_module(struct module *mod)
  {
  	do_mod_ctors(mod);
  	/* Start the module */
  	if (mod->init != NULL)
  		ret = do_one_initcall(mod->init);
      if (ret < 0) {
  		goto fail_free_freeinit;
  	}
  ```

​	调用模块构造函数或init函数。如果init函数返回值小于0，则模块加载失败。

- ```c
  	/* Now it's a first class citizen! */
    	mod->state = MODULE_STATE_LIVE;
    	blocking_notifier_call_chain(&module_notify_list,
    				     MODULE_STATE_LIVE, mod);
  ```

  模块状态修改为live，调用通知链。

- ```c
  	trim_init_extable(mod);
  #ifdef CONFIG_KALLSYMS
  	/* Switch to core kallsyms now init is done: kallsyms may be walking! */
  	rcu_assign_pointer(mod->kallsyms, &mod->core_kallsyms);
  #endif
  ```

  kallsyms处理

- ```c
  	module_enable_ro(mod, true);
    	mod_tree_remove_init(mod);
    	disable_ro_nx(&mod->init_layout);
  ```

  只读和nx

- ```c
  	mod->init_layout.base = NULL;
    	mod->init_layout.size = 0;
    	mod->init_layout.ro_size = 0;
    	mod->init_layout.ro_after_init_size = 0;
    	mod->init_layout.text_size = 0;
    	/*
    	 * We want to free module_init, but be aware that kallsyms may be
    	 * walking this with preempt disabled.  In all the failure paths, we
    	 * call synchronize_sched(), but we don't want to slow down the success
    	 * path, so use actual RCU here.
    	 */
    	call_rcu_sched(&freeinit->rcu, do_free_init);
  ```

  释放init_layout.

- do_init_module()返回。

### 22. 错误返回

```c
	//处理各种错误，返回错误码。
	return err;
}
```

## 总结

1. 模块载入内核后.ko文件是否可以删除？

   可以删除。内核会分配内存，把模块数据拷贝到新内存上。

2. 模块内调用的其他模块的函数是否必须是EXPORT_SYMBOL导出的符号？

   必须是导出的符号。如果引用了未导出的符号，模块会加载失败。

3. 模块加载失败是因为哪些原因？

   在机器A上编译的模块，拷贝到机器B上加载。

   - 机器A和B，模块相关的数据结构发生变化，模块加载失败。 #3.1
   - 在模块黑名单上。  #3.2
   - 机器A和B的vermagic信息不一致。 #3.3
   - 使用了未导出的符号。 #10
   - 导出的符号已经被其他模块导出。 #16
   - 模块init函数执行失败。 #21

4. 加载模块是一个把外部模块和内核链接的过程。

   1. 外部模块引用的内核符号会自动链接。已导出的。

## 模块符号处理API

### EXPORT_SYMBOL_GPL

| 函数                 | 描述                                              |
| -------------------- | ------------------------------------------------- |
| find_symbol          | 在内核和所有模块导出(EXPORT_SYMBOL)的符号中查找。 |
| find_module          | 根据模块名字查找模块。                            |
| kallsyms_lookup_name | 可以查找模块内的符号。                            |



# 五、模块卸载

# 六、其他

## 1. 模块licence

1. 非GPL的模块，不能使用由GPL导出的符号。其一，编译不过。其二，即使编译过了，加载模块也会失败。