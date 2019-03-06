# VDSO

> [![40](https://github.com/duanery/picture/blob/master/github/github_black_40px.png)](https://duanery.github.io)
> &nbsp;&nbsp;
> ***duanery*** &nbsp;
> 2019年3月6日
>
> Linux爱好者

VDSO是用来实现内核快速系统调用的。目前内部包含，`clock_gettime`,`gettimeofday`,`time`,`getcpu`这4个系统调用。

用户进行调用这4个系统调用，就像调用自己实现的函数一样，不会进入内核。内核把这4个系统调用用户态形式的函数封装到vdso这个库中，内部需要用到的数据保存到一个称为vvar的页上，用户调用这4个函数会从vvar页获取数据，而无需进入内核态。内核会不断的更新vvar页上的数据，保证vvar数据总是最新的。

VDSO源码目录：`arch/x86/entry/vdso`

## 编译

`arch/x86/entry/vdso/Makefile`

```makefile
# files to link into kernel
obj-y				+= vma.o

# vDSO images to build
vdso_img-$(VDSO64-y)		+= 64

# Build the vDSO image C files and link them in.
vdso_img_objs := $(vdso_img-y:%=vdso-image-%.o)
vdso_img_cfiles := $(vdso_img-y:%=vdso-image-%.c)
vdso_img_sodbg := $(vdso_img-y:%=vdso%.so.dbg)
obj-y += $(vdso_img_objs)
```

从Makefile文件中可以看到`obj-y`包含vma.o和vdso-image-64.o，这两个文件会被链接进入vmlinux。

继续分析。

```makefile
quiet_cmd_vdso2c = VDSO2C  $@
define cmd_vdso2c
	$(obj)/vdso2c $< $(<:%.dbg=%) $@
endef

$(obj)/vdso-image-%.c: $(obj)/vdso%.so.dbg $(obj)/vdso%.so $(obj)/vdso2c FORCE
	$(call if_changed,vdso2c)
```

vdso-image-64.o文件来自于vdso-image-64.c，这个c文件是使用vdso2c这个命令生成的，具体是`vdso2c vdso64.so.dbg vdso64.so vdso-image-64.c`，显然这个c文件依赖于vdso64.so.dbg和vdso64.so。

```makefile
# files to link into the vdso
vobjs-y := vdso-note.o vclock_gettime.o vgetcpu.o
vobjs := $(foreach F,$(vobjs-y),$(obj)/$F)

$(obj)/vdso64.so.dbg: $(src)/vdso.lds $(vobjs) FORCE
	$(call if_changed,vdso)
	
$(obj)/%.so: OBJCOPYFLAGS := -S
$(obj)/%.so: $(obj)/%.so.dbg
	$(call if_changed,objcopy)
#
# The DSO images are built using a special linker script.
#
quiet_cmd_vdso = VDSO    $@
      cmd_vdso = $(CC) -nostdlib -o $@ \
		       $(VDSO_LDFLAGS) $(VDSO_LDFLAGS_$(filter %.lds,$(^F))) \
		       -Wl,-T,$(filter %.lds,$^) $(filter %.o,$^) && \
		 sh $(srctree)/$(src)/checkundef.sh '$(NM)' '$@'
```

可以看到.so是通过objcopy自.so.dbg。vdso64.so.dbg是通过cmd_vdso这个命令编译来的，依赖于vdso.lds链接脚本和vobjs，vobjs包含vdso-note.o，vclock_gettime.o，vgetcpu.o这三个文件。

vdso64.so包含的就是一个动态库，内部包含了vclock_gettime.o，vgetcpu.o这两个核心源码文件，内部定义了`clock_gettime`,`gettimeofday`,`time`,`getcpu`这4个函数。具体见下文分析。

得到vdso64.so和vdso64.so.dbg之后，调用vdso2c命令把其转换成vdso-image-64.c文件再链接进入vmlinux。

看一下vdso-image-64.c文件的内容：

```c
/* AUTOMATICALLY GENERATED -- DO NOT EDIT */

#include <linux/linkage.h>
#include <asm/page_types.h>
#include <asm/vdso.h>

static unsigned char raw_data[8192] __ro_after_init __aligned(PAGE_SIZE) = {
        0x7F, 0x45, 0x4C, 0x46, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x3E, 0x00,
		...
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

const struct vdso_image vdso_image_64 = {
        .data = raw_data,
        .size = 8192,
        .alt = 3705,
        .alt_len = 156,
        .sym_vvar_start = -8192,
        .sym_vvar_page = -8192,
        .sym_pvclock_page = -4096,
};    
```

主要是定义了vdso_image_64这个结构体，把vdso64.so文件转换成raw_data这个数组，占用2个页，页对齐。

## VVAR

从上面的分析，vdso已经清晰了，是一个动态库，包含了4个函数，并且链接到内核中，占用2个页，页对齐。那vvar呢？vvar才是真正包含系统调用所需要的数据的页。内核会更新vvar页上的数据，vdso的4个库函数读取这个页上的数据。

首先，看一下vdso.so库中的vclock_gettime.c文件

```c
#define VVAR(name) (vvar_ ## name)

#define gtod (&VVAR(vsyscall_gtod_data))

notrace time_t __vdso_time(time_t *t)
{
	/* This is atomic on x86 so we don't need any locks. */
	time_t result = ACCESS_ONCE(gtod->wall_time_sec);

	if (t)
		*t = result;
	return result;
}
int time(time_t *t)
	__attribute__((weak, alias("__vdso_time")));
```

看看time系统调用的实现，别名为__vdso_time，通过`gtod->wall_time_sec`获取系统时间，gtod被展开为`vvar_vsyscall_gtod_data`，而这个符号是在vdso的链接脚本中定义的。

主要是vdso-layout.lds.S文件。

```
SECTIONS
{
	/*
	 * User/kernel shared data is before the vDSO.  This may be a little
	 * uglier than putting it after the vDSO, but it avoids issues with
	 * non-allocatable things that dangle past the end of the PT_LOAD
	 * segment.
	 */

	vvar_start = . - 2 * PAGE_SIZE;
	vvar_page = vvar_start;

	/* Place all vvars at the offsets in asm/vvar.h. */
#define EMIT_VVAR(name, offset) vvar_ ## name = vvar_page + offset;
#define __VVAR_KERNEL_LDS
#include <asm/vvar.h>
#undef __VVAR_KERNEL_LDS
#undef EMIT_VVAR

	pvclock_page = vvar_start + PAGE_SIZE;

...
...
```

链接脚本定义`EMIT_VVAR(name, offset)`宏和`__VVAR_KERNEL_LDS`宏，并包含`<asm/vvar.h>`文件，看看这个文件。

```c
#if defined(__VVAR_KERNEL_LDS)

/* The kernel linker script defines its own magic to put vvars in the
 * right place.
 */
#define DECLARE_VVAR(offset, type, name) \
	EMIT_VVAR(name, offset)

#else
	...
#fi
DECLARE_VVAR(128, struct vsyscall_gtod_data, vsyscall_gtod_data)
```

最终在链接脚本内会生成，`vvar_vsyscall_gtod_data = vvar_page + 128`，而vvar_page定义的地址在vdso加载地址的前面2个页内(`vvar_start = . - 2 * PAGE_SIZE;`)，见下文分析，vvar确实是在vdso的前面2个页内。

而且vvar页并不包含在vdso中，而是有Linux内核定义的vvar页。vvar页的真实物理地址是包含在vmlinux中的。我们需要分析vvar页+128字节地址处存放的到底是什么？具体在哪里定义的？

看看vsyscall_gtod.c文件

```c
#define DEFINE_VVAR(type, name)						\
	type name							\
	__attribute__((section(".vvar_" #name), aligned(16))) __visible

DEFINE_VVAR(struct vsyscall_gtod_data, vsyscall_gtod_data);
```

在该文件中定义vsyscall_gtod_data变量，并且存放在section`.vvar_vsyscall_gtod_data`中。再分析该section链接到了vmlinux的什么位置。

`vmlinux.lds.S`

```
	. = ALIGN(PAGE_SIZE);
	__vvar_page = .;

	.vvar : AT(ADDR(.vvar) - LOAD_OFFSET) {
		/* work around gold bug 13023 */
		__vvar_beginning_hack = .;

		/* Place all vvars at the offsets in asm/vvar.h. */
#define EMIT_VVAR(name, offset) 			\
		. = __vvar_beginning_hack + offset;	\
		*(.vvar_ ## name)
#define __VVAR_KERNEL_LDS
#include <asm/vvar.h>
#undef __VVAR_KERNEL_LDS
#undef EMIT_VVAR

		/*
		 * Pad the rest of the page with zeros.  Otherwise the loader
		 * can leave garbage here.
		 */
		. = __vvar_beginning_hack + PAGE_SIZE;
	} :data
```

可以看到，通过vvar.h文件，`.vvar_vsyscall_gtod_data`链接在了`__vvar_beginning_hack + 128`的位置，也就是__vvar_page + 128的位置。

### 总结

vvar页包含在vmlinux中，却在编译vdso时需要引用该页。而vdso内包含的系统调用，也需要根据vvar页的地址来读取数据。



## 初始化

```c
static int __init init_vdso(void)
{
	init_vdso_image(&vdso_image_64);

#ifdef CONFIG_X86_X32_ABI
	init_vdso_image(&vdso_image_x32);
#endif

	/* notifier priority > KVM */
	return cpuhp_setup_state(CPUHP_AP_X86_VDSO_VMA_ONLINE,
				 "AP_X86_VDSO_VMA_ONLINE", vgetcpu_online, NULL);
}
subsys_initcall(init_vdso);
```

调用`init_vdso_image(&vdso_image_64);`把vdso_image_64注册了。

## 进程执行

用execve系统调用执行程序时，会调用`arch_setup_additional_pages`函数，把[vvar]和[vdso]映射进用户态进程的地址空间中，通过`cat /proc/self/maps`可以看到。

```bash
[root@localhost ~]# cat /proc/self/maps
00400000-0040c000 r-xp 00000000 08:03 1310780                            /bin/cat
0060b000-0060c000 r--p 0000b000 08:03 1310780                            /bin/cat
0060c000-0060d000 rw-p 0000c000 08:03 1310780                            /bin/cat
01e05000-01e26000 rw-p 00000000 00:00 0                                  [heap]
...
7f1c09dbe000-7f1c09dbf000 rw-p 00000000 00:00 0 
7ffec58c5000-7ffec58e6000 rw-p 00000000 00:00 0                          [stack]
7ffec5995000-7ffec5997000 r--p 00000000 00:00 0                          [vvar]
7ffec5997000-7ffec5999000 r-xp 00000000 00:00 0                          [vdso]
ffffffffff600000-ffffffffff601000 r-xp 00000000 00:00 0                  [vsyscall]
```

看看`arch_setup_additional_pages`函数的实现。

```c
int arch_setup_additional_pages(struct linux_binprm *bprm, int uses_interp)
{
	if (!vdso64_enabled)
		return 0;
	return map_vdso_randomized(&vdso_image_64);
}
```

```c
static int map_vdso_randomized(const struct vdso_image *image)
{
	unsigned long addr = vdso_addr(current->mm->start_stack, image->size-image->sym_vvar_start);

	return map_vdso(image, addr);
}
```

`vdso_addr`函数会挑选vdso需要映射的地址，从栈顶部(`current->mm->start_stack`)随机选择`image->size-image->sym_vvar_start`这么大的一块地址，实际是16K（4个页，vvar占2个页，vdso占2个页）。

```c
static int map_vdso(const struct vdso_image *image, unsigned long addr)
{
	addr = get_unmapped_area(NULL, addr,
				 image->size - image->sym_vvar_start, 0, 0);
    text_start = addr - image->sym_vvar_start;
	current->mm->context.vdso = (void __user *)text_start;
	current->mm->context.vdso_image = image;
    /*
	 * MAYWRITE to allow gdb to COW and set breakpoints
	 */
	vma = _install_special_mapping(mm,
				       text_start,
				       image->size,
				       VM_READ|VM_EXEC|
				       VM_MAYREAD|VM_MAYWRITE|VM_MAYEXEC,
				       &vdso_mapping);
    vma = _install_special_mapping(mm,
				       addr,
				       -image->sym_vvar_start,
				       VM_READ|VM_MAYREAD|VM_IO|VM_DONTDUMP|
				       VM_PFNMAP,
				       &vvar_mapping);
}
```

`map_vdso`通过`get_unmapped_area`获取一个16K的未映射的区域，以addr指定的位置开始查找。然后计算得到vdso的地址(`text_start = addr - image->sym_vvar_start;`)，赋值给`current->mm->context.vdso`，通过`_install_special_mapping`把vdso和vvar对应的线性区安装。通过`cat /proc/self/maps`可以看到有[vvar]和[vdso]两个线性区。

**随机选择的vdso的地址如何告知给进程的用户态？**

```c
static int load_elf_binary(struct linux_binprm *bprm)
{
    ...
#ifdef ARCH_HAS_SETUP_ADDITIONAL_PAGES
	retval = arch_setup_additional_pages(bprm, !!elf_interpreter);
	if (retval < 0)
		goto out;
#endif /* ARCH_HAS_SETUP_ADDITIONAL_PAGES */
	retval = create_elf_tables(bprm, &loc->elf_ex,
			  load_addr, interp_load_addr);
    ...
}
```

execve系统调用在执行时会调用`load_elf_binary`来解析elf二进制程序来建立进程的地址空间，其中就会调用`arch_setup_additional_pages`建立[vvar]和[vdso]这两个线性区。

`create_elf_tables`会告知用户态进行一些核心信息。其中就包含vdso的线性地址

```c
static int
create_elf_tables(struct linux_binprm *bprm, struct elfhdr *exec,
		unsigned long load_addr, unsigned long interp_load_addr)
{
    ...
#ifdef ARCH_DLINFO
	/* 
	 * ARCH_DLINFO must come first so PPC can do its special alignment of
	 * AUXV.
	 * update AT_VECTOR_SIZE_ARCH if the number of NEW_AUX_ENT() in
	 * ARCH_DLINFO changes
	 */
	ARCH_DLINFO;
#endif
    ...
}
```

```c
#define ARCH_DLINFO							\
do {									\
	if (vdso64_enabled)						\
		NEW_AUX_ENT(AT_SYSINFO_EHDR,				\
			    (unsigned long __force)current->mm->context.vdso); \
} while (0)
```

可以看到`ARCH_DLINFO`宏把`current->mm->context.vdso`这个vdso的地址放到了`ELF Auxiliary Vectors`AT_SYSINFO_EHDR的位置。

### 总结

内核在程序执行期间会把[vdso]和[vvar]映射到用户控件，并通过`ELF Auxiliary Vectors`告知给用户态的elf解释器，解释器会把相关的系统调用链接到[vdso]中。



## 内核更新

只剩下内核如何往vvar页内填充数据了。是由vsyscall_gtod.c文件的`update_vsyscall`函数负责。

```c
void update_vsyscall(struct timekeeper *tk)
{
	int vclock_mode = tk->tkr_mono.clock->archdata.vclock_mode;
	struct vsyscall_gtod_data *vdata = &vsyscall_gtod_data;
	/* copy vsyscall data */
	vdata->vclock_mode	= vclock_mode;
	vdata->cycle_last	= tk->tkr_mono.cycle_last;
	vdata->mask		= tk->tkr_mono.mask;
	vdata->mult		= tk->tkr_mono.mult;
	vdata->shift		= tk->tkr_mono.shift;
    ...
}
```

内核的时钟框架，每次更新系统时间，都会调用该函数更新vvar页，保证用户态的系统调用都正常。



## 缺页异常

从前面的“进程执行”一节的分析看，内核只是给进程提供了[vvar]和[vdso]两个线性区，这两个线性区并未映射真正的页。只有用户进程真正需要访问这两个线性区时，才会给这两个线性区映射真正的页。

[vvar]对应的页在vmlinux中，__vvar_page符号，页对齐。

[vdso]对应的页在vmlinux中，vdso_image_64.data位置，页对齐。

用户态进行调用time这里系统调用时，会触发缺页异常，在进程页表中建立映射。

## 接口

### boot参数

	vdso=		[X86,SH]
			On X86_32, this is an alias for vdso32=.  Otherwise:
	
			vdso=1: enable VDSO (the default)
			vdso=0: disable VDSO mapping
			
	vdso32=		[X86] Control the 32-bit vDSO
				vdso32=1: enable 32-bit VDSO
				vdso32=0 or vdso32=2: disable 32-bit VDSO
	
				See the help text for CONFIG_COMPAT_VDSO for more
				details.  If CONFIG_COMPAT_VDSO is set, the default is
				vdso32=0; otherwise, the default is vdso32=1.
	
				For compatibility with older kernels, vdso32=2 is an
				alias for vdso32=0.
	
				Try vdso32=0 if you encounter an error that says:
				dl_main: Assertion `(void *) ph->p_vaddr == _rtld_local._dl_sysinfo_dso' failed!
## 参考文档

### 内核文档

Documention/kernel_parameters.txt

### 外网连接

http://articles.manugarg.com/aboutelfauxiliaryvectors.html

http://www.man7.org/linux/man-pages/man7/vdso.7.html

http://www.man7.org/linux/man-pages/man3/getauxval.3.html