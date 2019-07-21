# 模块分析

> [![40](https://github.com/duanery/picture/blob/master/github/github_black_40px.png)](https://duanery.github.io)
> &nbsp;&nbsp;
> ***duanery*** &nbsp;
> 2019年7月20日
>
> Linux爱好者

从模块编译、安装到模块加载。

## 一、使用方法

### 模块命令

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

### 模块用法

一个简单的模块：包含了模块的各种用途。

#### hello模块

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

#### hello_dep模块

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

#### 编译脚本

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

## 二、模块编译

### 配置项

- CONFIG_MODULES

  打开模块功能

- CONFIG_MODVERSIONS

  对模块内导出的符号进行crc校验。未导出的符号无crc。

### section

在模块加载时会查看模块内所有特殊功能的section，并与内核建立联系。例如：

1. 模块内定义的percpu变量，会在模块加载分配内存空间。
2. 模块内使用copy_from_user等类似的宏，会建立异常跳转表，也需要特殊处理，在内核收到异常时能判断在哪个模块的上下文中。
3. 模块内使用的static_key，也需要特殊处理。

分析哪些宏会向模块内引入哪些section会很有意义。

#### 1. 模块宏定义引入的section

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

#### 2.模块调用内核接口引入的section

TODO

#### 3.".mod.c"中引入的section

- .gnu.linkonce.this_module：这个section内唯一存放了一个struct module _\_this\_module变量。
- __versions：存放由当前模块调用的别的模块导出函数的版本信息。
- .modinfo：存放"depends=..."这样的依赖关系字符串，存放"vermagic=VERMAGIC_STRING"这样的版本字符串，存放"alias=..."模块别名字符串，存放"srcversion=..."这样的源码版本字符串。

### 编译第一阶段

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

### 编译第二阶段：modpost

无论是外部模块编译，还是内核编译，都会进入到第二阶段：modpost。

这一阶段主要是计算模块的依赖关系。生成.mod.c文件。链接生成.ko模块文件。

#### step1

从MODVERDIR中提取总共有多少模块需要在第二阶段处理。全部模块的路径名都找出来。

#### step2

调用`scripts/mod/modpost`命令，并为每个模块输出.mod.c文件。mod.c文件包含以下信息：

1. vermagic，由宏`MODULE_INFO(vermagic, VERMAGIC_STRING);`生成。
2. THIS_MODULE，定义了一个`struct module __this_module`变量，并存放到section。
3. \_\_\_\_versions[]，`struct modversion_info`结构体数组，存放由该模块引用的其他模块的函数的版本信息，本模块调用其他模块导出的函数(EXPORT_SYMBOL)，会记录其引用的其他模块的函数的crc值及name。这个结构体数组存放在`__versions`section中。
4. depends，模块间的依赖关系。本模块调用了其他模块导出的函数，就意味着本模块依赖其他模块。modpost能找出本模块依赖的所有模块。
5. device_table，模块内使用MODULE_DEVICE_TABLE宏定义一个表后，会产生这样的信息。一般驱动模块会定义这样的表，以示该模块能驱动是硬件设备。modpost会利用这个表生产模块别名。由MODULE_ALIAS引用。
6. srcversion，由宏`MODULE_INFO(srcversion, ...）`生成，定义模块的版本信息，详细生产方法：TODO

**模块依赖关系**

​	由模块导出的符号，以及模块调用其他模块导出的符号，这样2个信息来取得模块的依赖关系。

​	模块导出的符号会放到__ksymtab这样的section中，遍历模块.o文件的符号表，可以直接获取到。调用其他模块的符号，在模块.o文件的符号表中是UNDEF的。

​	因此，读取内核的全部模块，对这些模块导出符号建立全局哈希表。然后对每一个模块，遍历其UNDEF的符号，在哈希表内查找该符号在哪个模块定义，由此可以建立这个模块依赖的所有模块。

**example**

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

**外部模块**

内核模块编译完后，会把vmlinux及内核模块导出的全部符号写入Module.symvers文件。外部模块在编译时，先从这个文件加载全部导出的符号，然后根据外部模块自己的导出符号和UNDEF符号，来确定模块依赖关系，生成.mod.c文件。

Module.symvers文件在`/lib/modules/$(uname -r)/build/`目录。

#### step3

编译.mod.c文件。

#### step4

链接.mod.o和\<file>.o文件为\<file>.ko文件。

#### step5

模块签名：TODO

## 三、模块安装

### modules_install

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

### 总结

1. 模块安装主要是知道模块安装的目录：`/lib/modules/$(uname -r)/kernel/`或`/lib/modules/$(uname -r)/extra/`
2. 模块安装后会调用depmod进行模块依赖处理。使用modprobe就可以直接加载有多重依赖关系的模块。

## 四、模块加载

运行`insmod hello`或`modprobe hello_dep`就可以直接加载模块。