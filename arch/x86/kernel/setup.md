# setup_arch

> [![40](https://github.com/duanery/picture/blob/master/github/github_black_40px.png)](https://duanery.github.io)
> &nbsp;&nbsp;
> ***duanery*** &nbsp;
> 2019年4月30日
>
> Linux爱好者

该函数主要用于x86体系结构的初始化。

内容比较多。

```c
/*
 * setup_arch - architecture-specific boot-time initializations
 */
void __init setup_arch(char **cmdline_p)
{
```

## 1

```c
	memblock_reserve(__pa_symbol(_text),
			 (unsigned long)__bss_stop - (unsigned long)_text);

	/*
	 * Make sure page 0 is always reserved because on systems with
	 * L1TF its contents can be leaked to user processes.
	 */
	memblock_reserve(0, PAGE_SIZE);

	early_reserve_initrd();
```

在memblock中保留page 0、内核text,data,init,bss段、initrd所占用的物理地址。

## 2

```c
	printk(KERN_INFO "Command line: %s\n", boot_command_line);
```

打印内核命令行参数。在`x86_64_start_kernel`函数中，把内核命令行参数，从boot_params中拷贝到boot_command_line数组中，数组2048字节。

从内核日志可以分析到内核启动参数。

## 3

```c
	early_trap_init();
```

注册Debug和Breakpoint2个异常。

## 4

```c
	early_cpu_init();
```

早期cpu的初始化：cpu检测，判断cpu制造商，赋值this_cpu设备，读取cpu能力(cpuid)，初始化boot_cpu_data，this_cpu->c_early_init(c)，this_cpu->c_bsp_init(c)，fpu初始化等。

## 5

```c
	early_ioremap_init();
```

早期ioremap初始化。参考`mm/eayly_ioremap.c`文件。

定义了`early_ioremap,early_memremap,early_memremap_ro,early_iounmap`这样的四个函数来操作。

主要用于映射boot_params中指向的物理地址以访问数据结构。早期的物理地址的对等映射在`x86_64_start_kernel`函数中被清除了，所以不能直接访问了。

## 6

```c
	x86_init.oem.arch_setup();
```

空函数。

## 7

```c
	iomem_resource.end = (1ULL << boot_cpu_data.x86_phys_bits) - 1;
```

物理内存资源的最大值。`boot_cpu_data.x86_phys_bits`是从cpuid_eax(0x80000008)获取的，物理地址的最大位数。

## 8

```c
	setup_memory_map();
```

调用`default_machine_specific_memory_setup`函数把boot_params中的e820数组拷贝到e820全局变量中。在内核日志中打印e820信息。

```
[    0.000000] e820: BIOS-provided physical RAM map:
[    0.000000] BIOS-e820: [mem 0x0000000000000000-0x000000000009c7ff] usable
[    0.000000] BIOS-e820: [mem 0x000000000009c800-0x000000000009ffff] reserved
[    0.000000] BIOS-e820: [mem 0x00000000000e0000-0x00000000000fffff] reserved
[    0.000000] BIOS-e820: [mem 0x0000000000100000-0x0000000071392fff] usable
[    0.000000] BIOS-e820: [mem 0x0000000071393000-0x0000000071393fff] ACPI NVS
[    0.000000] BIOS-e820: [mem 0x0000000071394000-0x00000000713bdfff] reserved
[    0.000000] BIOS-e820: [mem 0x00000000713be000-0x0000000076f11fff] usable
[    0.000000] BIOS-e820: [mem 0x0000000076f12000-0x0000000077214fff] reserved
[    0.000000] BIOS-e820: [mem 0x0000000077215000-0x000000007726cfff] ACPI data
[    0.000000] BIOS-e820: [mem 0x000000007726d000-0x0000000077ba5fff] ACPI NVS
[    0.000000] BIOS-e820: [mem 0x0000000077ba6000-0x0000000077ffefff] reserved
[    0.000000] BIOS-e820: [mem 0x0000000077fff000-0x0000000077ffffff] usable
[    0.000000] BIOS-e820: [mem 0x0000000078000000-0x00000000780fffff] reserved
[    0.000000] BIOS-e820: [mem 0x00000000e0000000-0x00000000efffffff] reserved
[    0.000000] BIOS-e820: [mem 0x00000000fe000000-0x00000000fe010fff] reserved
[    0.000000] BIOS-e820: [mem 0x00000000fec00000-0x00000000fec00fff] reserved
[    0.000000] BIOS-e820: [mem 0x00000000fee00000-0x00000000fee00fff] reserved
[    0.000000] BIOS-e820: [mem 0x00000000ff000000-0x00000000ffffffff] reserved
[    0.000000] BIOS-e820: [mem 0x0000000100000000-0x00000002807fffff] usable
```

## 9

```c
	parse_setup_data();
```

处理boot_params.hdr.setup_data数据，会调用`early_memremap`及`early_memunmap`来访问物理地址的数据。

参考Boot Protocol中setup_data相关字段的介绍。

## 10

```c
	if (!boot_params.hdr.root_flags)
		root_mountflags &= ~MS_RDONLY;
	init_mm.start_code = (unsigned long) _text;
	init_mm.end_code = (unsigned long) _etext;
	init_mm.end_data = (unsigned long) _edata;
	init_mm.brk = _brk_end;
```

初始化init进程的mm_struct相关的字段。

## 11

```c
	code_resource.start = __pa_symbol(_text);
	code_resource.end = __pa_symbol(_etext)-1;
	data_resource.start = __pa_symbol(_etext);
	data_resource.end = __pa_symbol(_edata)-1;
	bss_resource.start = __pa_symbol(__bss_start);
	bss_resource.end = __pa_symbol(__bss_stop)-1;
```

初始化code,data,bss相关资源。

## 12

```c
#ifdef CONFIG_CMDLINE_BOOL
#ifdef CONFIG_CMDLINE_OVERRIDE
	strlcpy(boot_command_line, builtin_cmdline, COMMAND_LINE_SIZE);
#else
	if (builtin_cmdline[0]) {
		/* append boot loader cmdline to builtin */
		strlcat(builtin_cmdline, " ", COMMAND_LINE_SIZE);
		strlcat(builtin_cmdline, boot_command_line, COMMAND_LINE_SIZE);
		strlcpy(boot_command_line, builtin_cmdline, COMMAND_LINE_SIZE);
	}
#endif
#endif

	strlcpy(command_line, boot_command_line, COMMAND_LINE_SIZE);
	*cmdline_p = command_line;
```

内核编译时，可以通过CONFIG_CMDLINE指定内建的命令行参数，把这个参数跟boot_command_line连接起来。

并拷贝到command_line中。

## 13

```c
	parse_early_param();
```

解析早期的内核参数。早期的内核参数是通过early_param安装的参数。

## 14

```c
	/* after early param, so could get panic from serial */
	memblock_x86_reserve_range_setup_data();
```

在memblock中保留`boot_params.hdr.setup_data`物理地址数据。

## 15

```c
	/* update the e820_saved too */
	e820_reserve_setup_data();
	finish_e820_parsing();
```

在e820中保留`boot_params.hdr.setup_data`物理地址。finish e820数据。

## 16

```c
	/*
	 * VMware detection requires dmi to be available, so this
	 * needs to be done after dmi_scan_machine, for the BP.
	 */
	init_hypervisor_platform();
```

TODO：Linux虚拟机可以识别到是否是虚拟化环境。

## 17

```c
	kaiser_check_boottime_disable();
```

检查kaiser是否启用。`kernel address isolation to have side-channels efficiently removed`

内核参数：

```
	pti=		[X86_64] Control Page Table Isolation of user and
			kernel address spaces.  Disabling this feature
			removes hardening, but improves performance of
			system calls and interrupts.

			on   - unconditionally enable
			off  - unconditionally disable
			auto - kernel detects whether your CPU model is
			       vulnerable to issues that PTI mitigates

			Not specifying this option is equivalent to pti=auto.

	nopti		[X86_64]
			Equivalent to pti=off
```

## 18

```c
	/* after parse_early_param, so could debug it */
	insert_resource(&iomem_resource, &code_resource);
	insert_resource(&iomem_resource, &data_resource);
	insert_resource(&iomem_resource, &bss_resource);
```

资源插入

## 19

```c
	e820_add_kernel_range();
	trim_bios_range();
```

e820加入内核从\_text到\_end的物理地址范围。

去除bios范围。（0xa0000-0x100000）

## 20

```c
	/*
	 * partially used pages are not usable - thus
	 * we are rounding upwards:
	 */
	max_pfn = e820_end_of_ram_pfn();

	/* update e820 for memory not covered by WB MTRRs */
	mtrr_bp_init();
	if (mtrr_trim_uncached_memory(max_pfn))
		max_pfn = e820_end_of_ram_pfn();

	max_possible_pfn = max_pfn;
```

从e820计算最大页框号赋值给max_pfn及max_possible_pfn. 

MTRR: TODO

## 21

```c
init_cache_modes();
```

PAT支持

## 22*

```c
	/*
	 * Define random base addresses for memory sections after max_pfn is
	 * defined and before each memory section base is used.
	 */
	kernel_randomize_memory();
```

KASLR内核地址空间布局随机化。随机选择物理内存直接映射、vmalloc、vmemmap区域的线性地址。

先看看4级页表的地址空间分配：

```
0000000000000000 - 00007fffffffffff (=47 bits) user space, different per mm
hole caused by [48:63] sign extension
ffff800000000000 - ffff87ffffffffff (=43 bits) guard hole, reserved for hypervisor
ffff880000000000 - ffffc7ffffffffff (=64 TB) direct mapping of all phys. memory
ffffc80000000000 - ffffc8ffffffffff (=40 bits) hole
ffffc90000000000 - ffffe8ffffffffff (=45 bits) vmalloc/ioremap space
ffffe90000000000 - ffffe9ffffffffff (=40 bits) hole
ffffea0000000000 - ffffeaffffffffff (=40 bits) virtual memory map (1TB)
... unused hole ...
ffffec0000000000 - fffffbffffffffff (=44 bits) kasan shadow memory (16TB)
... unused hole ...
ffffff0000000000 - ffffff7fffffffff (=39 bits) %esp fixup stacks
... unused hole ...
ffffffef00000000 - fffffffeffffffff (=64 GB) EFI region mapping space
... unused hole ...
ffffffff80000000 - ffffffff9fffffff (=512 MB)  kernel text mapping, from phys 0
ffffffffa0000000 - ffffffffff5fffff (=1526 MB) module mapping space
ffffffffff600000 - ffffffffffdfffff (=8 MB) vsyscalls
ffffffffffe00000 - ffffffffffffffff (=2 MB) unused hole

Note that if CONFIG_RANDOMIZE_MEMORY is enabled, the direct mapping of all
physical memory, vmalloc/ioremap space and virtual memory map are randomized.
Their order is preserved but their base will be offset early at boot time.
```

其中，virtual memory map是struct page结构体数组占据的线性地址空间。参考：<http://www.wowotech.net/memory_management/memory_model.html>

这之后__va和\_\_pa宏就可以使用了。

## 23

```c
	check_x2apic();
```

获取bios配置的x2APIC的情况。

## 24

```c
	/* How many end-of-memory variables you have, grandma! */
	/* need this before calling reserve_initrd */
	if (max_pfn > (1UL<<(32 - PAGE_SHIFT)))
		max_low_pfn = e820_end_of_low_ram_pfn();
	else
		max_low_pfn = max_pfn;

	high_memory = (void *)__va(max_pfn * PAGE_SIZE - 1) + 1;
```

如果物理内存大于4G，则max_low_pfn则是小于4G的最大物理页框号。

high_memory是物理内存最大地址。是一个虚拟地址。__va返回的虚拟地址在`ffff880000000000 - ffffc7ffffffffff (=64 TB) direct mapping of all phys. memory`范围内，如果第22步启用了KASLR，则在随机选择的物理内存直接映射的范围内。

因为第22步已经初始化过了KASLR，所以__va宏可以把物理地址转换为虚拟地址了。

## 25

```c
	early_alloc_pgt_buf();
```

申请内核早期页表分配缓存区。大小为INIT_PGT_BUF_SIZE。

这一块缓冲区是通过`RESERVE_BRK(early_pgt_alloc, INIT_PGT_BUF_SIZE)`宏保留下来的，存放在内核的brk段中。

## 26

```c
	reserve_brk();
```

在memblock中保留第25步分配的页表缓冲区占用的物理内存。

## 27

```c
	cleanup_highmap();
```

在`ffffffff80000000 - ffffffff9fffffff (=512 MB)  kernel text mapping, from phys 0`内核text映射地址范围内，清除`_text到_brk_end`之外的多余的映射。步长PMD_SIZE. level2_kernel_pgt.

## 28*

```c
	memblock_set_current_limit(ISA_END_ADDRESS);
	memblock_x86_fill();
```

设置从memblock中分配物理内存的限度为1M。`#define ISA_END_ADDRESS 0x100000`

把e820表示的物理内存加入memblock中。打印当前memblock的配置。如下：

```
[    0.000000] MEMBLOCK configuration:
[    0.000000]  memory size = 0x1f7683800 reserved size = 0x2aec1ac
[    0.000000]  memory.cnt  = 0x5
[    0.000000]  memory[0x0]	[0x00000000001000-0x0000000009bfff], 0x9b000 bytes flags: 0x0
[    0.000000]  memory[0x1]	[0x00000000100000-0x00000071392fff], 0x71293000 bytes flags: 0x0
[    0.000000]  memory[0x2]	[0x000000713be000-0x00000076f11fff], 0x5b54000 bytes flags: 0x0
[    0.000000]  memory[0x3]	[0x00000077fff000-0x00000077ffffff], 0x1000 bytes flags: 0x0
[    0.000000]  memory[0x4]	[0x00000100000000-0x000002807fffff], 0x180800000 bytes flags: 0x0
[    0.000000]  reserved.cnt  = 0x4
[    0.000000]  reserved[0x0]	[0x000000000fca10-0x000000000fcbab], 0x19c bytes flags: 0x0
[    0.000000]  reserved[0x1]	[0x000000000fcc30-0x000000000fcc3f], 0x10 bytes flags: 0x0
[    0.000000]  reserved[0x2]	[0x00000001000000-0x0000000214afff], 0x114b000 bytes flags: 0x0
[    0.000000]  reserved[0x3]	[0x00000034cad000-0x0000003664dfff], 0x19a1000 bytes flags: 0x0
[    0.000000] memblock_reserve: [0x0000000009c800-0x000000000fffff] flags 0x0 reserve_bios_regions+0x5b/0x5d
[    0.000000] memblock_reserve: [0x00000000001000-0x0000000000ffff] flags 0x0 setup_bios_corruption_check+0x131/0x192
```

## 29

```c
	reserve_bios_regions();
```

在memblock中保留bios区域。

## 30

```c
	reserve_real_mode();
```

在内核中保留一块实模式代码。在0-1M范围内申请实模式代码占用的物理内存空间。并设置real_mode_header的值。

实模式代码的作用：TODO

## 31

```c
	init_mem_mapping();
```



## 32

## 33

## 34

## 35

## 36

## 37

## 38

## 39

## 40