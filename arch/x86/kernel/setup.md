# setup_arch

> [![40](https://github.com/duanery/picture/blob/master/github/github_black_40px.png)](https://duanery.github.io)
> &nbsp;&nbsp;
> ***duanery*** &nbsp;
> 2019年4月30日
>
> Linux爱好者

该函数主要用于x86体系结构的初始化。

内容比较多。加*的是重点关注内容。

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

## 3* 加载IDT

```c
	early_trap_init();
```

注册Debug和Breakpoint2个异常。把idt_descr加载到idt寄存器。之后只需要通过注册`中断门，陷阱门，系统中断门，系统陷阱门`就可以注册异常处理程序。

中断描述符初始化完成。

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

## 13* 解析早期内核参数

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

## 22* KASLR

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

## 28* memblock填充

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

在这之后就可以使用memblock来分配物理内存。

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

## 31* 物理内存直接映射

```c
	init_mem_mapping();
```

这个函数建立物理内存的直接映射。把物理地址[0 - max]映射到虚拟地址[ffff880000000000 - ffffc7ffffffffff].

会剔除掉物理内存之间的空洞。0-1M之间的空洞不会剔除掉。

```
ffff880000000000 - ffffc7ffffffffff (=64 TB) direct mapping of all phys. memory
```

物理内存的直接映射是从`ffff880000000000`地址开始的，最大64TB。如果开启KASLR，则起始地址存放在page_offset_base变量中。这是随机选择的一块虚拟地址。

### 31.1 函数内部

```c
void __init init_mem_mapping(void)
{
	unsigned long end;

	probe_page_size_mask();
```

`probe_page_size_mask()`探测cpu是否支持2M/1G的大页，以及Global TLB标志。

```c
#ifdef CONFIG_X86_64
	end = max_pfn << PAGE_SHIFT;
#else
	end = max_low_pfn << PAGE_SHIFT;
#endif
```

确定物理地址的最后一个地址。

```c
	/* the ISA range is always mapped regardless of memory holes */
	init_memory_mapping(0, ISA_END_ADDRESS);
```

映射[0-1M]之间的物理地址到[ffff880000000000 + 1M].

```c
	/* Init the trampoline, possibly with KASLR memory offset */
	init_trampoline();
```

把[ffff880000000000 + 1M]页表保存下来。

```c
	if (memblock_bottom_up()) {
		...
	} else {
		memory_map_top_down(ISA_END_ADDRESS, end);
	}
```

映射[1M - end]之间的物理地址到[ffff880000000000+1M - ffff880000000000+end].

[1M - end]之间的物理地址会存在空洞，空洞不会进行映射。可以参考内核代码注释。

```c
#ifdef CONFIG_X86_64
	if (max_pfn > max_low_pfn) {
		/* can we preseve max_low_pfn ?*/
		max_low_pfn = max_pfn;
	}
#else
	...
#endif
```

max_low_pfn = max_pfn;

```c
	load_cr3(swapper_pg_dir);
	__flush_tlb_all();

	early_memtest(0, max_pfn_mapped << PAGE_SHIFT);
}
```

把init进程页表加载到cr3.  刷新tlb。

前面建立的直接映射都是修改的init进程页表，这里加载到cr3之后，通过__va得到的虚拟地址就可以直接访问了。不会再产生缺页异常。

### 31.2 疑问

建立直接映射时，一定会分配4K字节的pud、pmd、pte来存放页表项。那么操作这些页表项使用的虚拟地址(通过alloc_low_page函数分配)也是通过__va得到的，访问这个虚拟地址会发生什么？

在最后load_cr3之前，使用的是early_level4_pgt这个早期的临时页表。访问未映射的虚拟地址时，会发生缺页异常，在缺页异常处理程序内部会在early_level4_pgt上建立临时页表。所以可以访问__va得到的虚拟地址。

### 31.3 PAGE_OFFSET

物理地址加上PAGE_OFFSET得到在内核空间的虚拟地址。根据这个条件，PAGE_OFFSET就等于`ffff880000000000`或page_offset_base的值。

## 32

```c
	early_trap_pf_init();
```

注册缺页异常处理程序。`set_intr_gate(X86_TRAP_PF, page_fault);`

前面已经加载过IDT寄存器了，这里只需要注册中断门，缺页异常就直接可以处理了。

因为31步中物理内存的直接映射已经建立好了，不再需要早期的缺页异常处理程序了，就把缺页异常初始化为最终的page_fault函数，最终会调用do_page_fault.

## 33

```c
	memblock_set_current_limit(get_max_mapped());
```

设置memblock的限制为最大内存。

之后从memblock分配物理内存，就能分配到高地址的物理内存。

## 34

```c
	/* Allocate bigger log buffer */
	setup_log_buf(1);
```

如果配置了log_buf_len这个内核参数，就从memblock中分配一个缓冲区，存放printk打印的日志。

```
	log_buf_len=n[KMG]	Sets the size of the printk ring buffer,
			in bytes.  n must be a power of two and greater
			than the minimal size. The minimal size is defined
			by LOG_BUF_SHIFT kernel config parameter. There is
			also CONFIG_LOG_CPU_MAX_BUF_SHIFT config parameter
			that allows to increase the default size depending on
			the number of CPUs. See init/Kconfig for more details.
```

前面已经完全初始化好了物理内存映射，及memblock物理内存分配器。这一步可以从memblock中分配物理内存，并通过__va转换为虚拟地址。

## 35

```c
	reserve_initrd();
```

判断是否需要把initrd搬移到别的地方。在memblock中分配物理内存。

## 36

```c
	acpi_table_upgrade();
```

在initrd中搜索`kernel/firmware/acpi/`路径下的文件，这个路径下的文件可以用来修正bios提供的ACPI表。亦或是测试ACPI表。从memblock申请内存，并把ACPI表从initrd拷贝到申请的内存中。

后续会使用BIOS及这里提取到的ACPI表。

## 37* ACPI

```c
	/*
	 * Parse the ACPI tables for possible boot-time SMP configuration.
	 */
	acpi_boot_table_init();
```

初始化ACPI。

### 37.1 函数内部

```c
void __init acpi_boot_table_init(void)
{
    ...
	/*
	 * Initialize the ACPI boot-time table parser.
	 */
	if (acpi_table_init()) {
		disable_acpi();
		return;
	}
    acpi_table_parse(ACPI_SIG_BOOT, acpi_parse_sbf);
    ...
}
```

acpi_table_init函数会从bios或uefi中获取"RSDP"根，并解析根表，解析完会存放在一个数组里。并打印ACPI表信息。内核日志如下：

```
[    0.000000] ACPI: Early table checksum verification disabled
[    0.000000] ACPI: RSDP 0x00000000000F05B0 000024 (v02 HPQOEM)
[    0.000000] ACPI: XSDT 0x00000000772370A0 0000C4 (v01 HPQOEM SLIC-CPC 01072009 AMI  00010013)
[    0.000000] ACPI: FACP 0x0000000077261638 00010C (v05 HPQOEM SLIC-CPC 01072009 AMI  00010013)
[    0.000000] ACPI: DSDT 0x00000000772371F8 02A43D (v02 HPQOEM SLIC-CPC 01072009 INTL 20120913)
[    0.000000] ACPI: FACS 0x0000000077BA3F80 000040
[    0.000000] ACPI: APIC 0x0000000077261748 000084 (v03 HPQOEM SLIC-CPC 01072009 AMI  00010013)
[    0.000000] ACPI: FPDT 0x00000000772617D0 000044 (v01 HPQOEM SLIC-CPC 01072009 AMI  00010013)
[    0.000000] ACPI: FIDT 0x0000000077261818 00009C (v01 HPQOEM SLIC-CPC 01072009 AMI  00010013)
[    0.000000] ACPI: MCFG 0x00000000772618B8 00003C (v01 HPQOEM SLIC-CPC 01072009 MSFT 00000097)
[    0.000000] ACPI: HPET 0x00000000772618F8 000038 (v01 HPQOEM SLIC-CPC 01072009 AMI. 0005000B)
[    0.000000] ACPI: SSDT 0x0000000077261930 00036D (v01 HPQOEM SLIC-CPC 00001000 INTL 20120913)
[    0.000000] ACPI: LPIT 0x0000000077261CA0 000094 (v01 HPQOEM SLIC-CPC 00000000 MSFT 0000005F)
[    0.000000] ACPI: SSDT 0x0000000077261D38 000248 (v02 HPQOEM SLIC-CPC 00000000 INTL 20120913)
[    0.000000] ACPI: SSDT 0x0000000077261F80 002BAE (v02 HPQOEM SLIC-CPC 00001000 INTL 20120913)
[    0.000000] ACPI: SSDT 0x0000000077264B30 000BE3 (v02 HPQOEM SLIC-CPC 00001000 INTL 20120913)
[    0.000000] ACPI: DBGP 0x0000000077265718 000034 (v01 HPQOEM SLIC-CPC 00000000 MSFT 0000005F)
[    0.000000] ACPI: DBG2 0x0000000077265750 000054 (v00 HPQOEM SLIC-CPC 00000000 MSFT 0000005F)
[    0.000000] ACPI: SSDT 0x00000000772657A8 000618 (v02 HPQOEM SLIC-CPC 00000000 INTL 20120913)
[    0.000000] ACPI: MSDM 0x0000000077265DC0 000055 (v03 HPQOEM SLIC-CPC 01072009 AMI  00010013)
[    0.000000] ACPI: SSDT 0x0000000077265E18 00546C (v02 HPQOEM SLIC-CPC 00003000 INTL 20120913)
[    0.000000] ACPI: UEFI 0x000000007726B288 000042 (v01 HPQOEM SLIC-CPC 00000000      00000000)
[    0.000000] ACPI: SSDT 0x000000007726B2D0 000E73 (v02 HPQOEM SLIC-CPC 00003000 INTL 20120913)
[    0.000000] ACPI: DMAR 0x000000007726C148 0000A8 (v01 HPQOEM SLIC-CPC 00000001 INTL 00000001)
[    0.000000] ACPI: DBGP 0x000000007726C1F0 000034 (v01 HPQOEM SLIC-CPC 01072009 AMI  00010013)
[    0.000000] ACPI: Local APIC address 0xfee00000
```

## 38* APIC

```c
	early_acpi_boot_init();
```

从ACPI表中获取"APIC"表，并取得lapic的地址，并检测不同oem的APIC驱动，注册lapic地址(设置固定映射)。

内核实现了多个oem版本的APIC驱动，在内核源码`arch/x86/kernel/apic/`目录下，通过`apic_driver`宏来注册不同的APIC驱动。根据"APIC"表来获取当前的APIC驱动，赋值给apic变量。

## 39* NUMA

```c
	initmem_init();
```

NUMA的初始化：

## 40