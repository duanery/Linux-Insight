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

打印内核命令行参数。在x86_64_start_kernel()函数中，把内核命令行参数，从boot_params中拷贝到*boot_command_line*数组中，数组2048字节。

从内核日志可以分析到内核启动参数。

## 3* 加载IDT

```c
	early_trap_init();
```

注册Debug和Breakpoint2个异常。把***idt_descr***加载到idt寄存器。之后只需要通过注册`中断门，陷阱门，系统中断门，系统陷阱门`就可以注册异常处理程序。

中断描述符初始化完成。

## 4

```c
	early_cpu_init();
```

早期cpu的初始化：cpu检测，判断cpu制造商，赋值this_cpu设备，读取cpu能力(cpuid)，初始化*boot_cpu_data*，this_cpu->c_early_init(c)，this_cpu->c_bsp_init(c)，fpu初始化等。

## 5

```c
	early_ioremap_init();
```

早期ioremap初始化。参考`mm/eayly_ioremap.c`文件。

定义了`early_ioremap,early_memremap,early_memremap_ro,early_iounmap`这样的四个函数来操作。

主要用于映射boot_params中指向的物理地址以访问数据结构。早期的物理地址的对等映射在x86_64_start_kernel()函数中被清除了，所以不能直接访问了。

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

调用default_machine_specific_memory_setup()函数把boot_params中的e820数组拷贝到***e820***全局变量中。在内核日志中打印e820信息。

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

处理boot_params.hdr.setup_data数据，会调用early_memremap()及early_memunmap()来访问物理地址的数据。

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

获取bios配置的x2APIC的情况。设置`x2apic_mode = 1; x2apic_state = X2APIC_ON;`

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

### 31.1 函数分析

```c
void __init init_mem_mapping(void)
{
	unsigned long end;

	probe_page_size_mask();
```

probe_page_size_mask()探测cpu是否支持2M/1G的大页，以及Global TLB标志。

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

建立直接映射时，一定会分配4K字节的pud、pmd、pte来存放页表项。那么操作这些页表项使用的虚拟地址(通过alloc_low_page()函数分配)也是通过__va得到的，访问这个虚拟地址会发生什么？

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

如果配置了*log_buf_len*这个内核参数，就从memblock中分配一个缓冲区，存放printk打印的日志。

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

### 37.1 函数分析

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

NUMA的初始化：在CONFIG_NUMA=y的情况下，会在这里初始化numa。

### 39.1 启动参数

```
NUMA

  numa=off	Only set up a single NUMA node spanning all memory.

  numa=noacpi   Don't parse the SRAT table for NUMA setup

  numa=fake=<size>[MG]
		If given as a memory unit, fills all system RAM with nodes of
		size interleaved over physical nodes.

  numa=fake=<N>
		If given as an integer, fills all system RAM with N fake nodes
		interleaved over physical nodes.
```

### 39.2 简要分析

```c
void __init initmem_init(void)
{
	x86_numa_init();
}
void __init x86_numa_init(void)
{
	if (!numa_off) {
#ifdef CONFIG_ACPI_NUMA
		if (!numa_init(x86_acpi_numa_init))
			return;
#endif
    ...
    }
}
```

最终会调用到x86_numa_init中。先分析`x86_acpi_numa_init`，然后再分析`numa_init`。

x86_acpi_numa_init()会调用acpi_numa_init，从acpi驱动中来获取numa的信息。numa的信息包含两部分：

```
SRAT: Static Resource Affinity Table
SLIT: System Locality Information Table
```

SRAT中包含cpu和内存资源与NUMA节点的亲和性信息。

numa_init()的核心函数是numa_register_memblks()，这个函数会把内存亲和性信息转移到memblock上，并设置`node_possible_map,node_online_map`，并从每个NUMA节点的物理内存中分配类型为`pg_data_t`的对象，这个对象唯一的表示一个numa节点。

所有的pg_data_t类型的对象，存放在一个***node_data***的数组之中。`struct pglist_data *node_data[MAX_NUMNODES] __read_mostly;`通过`NODE_DATA(nid)`宏来访问每个numa节点对象。

### 39.3 详细分析

[numa初始化](../mm/numa初始化.md)

### 39.4 总结

numa初始化之后，就可以知道有哪些possible的numa节点，有哪些online的numa节点。以及为每个numa节点分配描述符。

## 40

```c
	reserve_crashkernel();
```

解析"crashkernel="内核参数，并在memblock中分配相应的物理内存。

```
	crashkernel=size[KMG][@offset[KMG]]
			[KNL] Using kexec, Linux can switch to a 'crash kernel'
			upon panic. This parameter reserves the physical
			memory region [offset, offset + size] for that kernel
			image. If '@offset' is omitted, then a suitable offset
			is selected automatically. Check
			Documentation/kdump/kdump.txt for further details.
```

## 41

```c
	memblock_find_dma_reserve();
```

从memblock中查找小于16M的物理内存中有多少页框是已经分配的，并赋值给*dma_reserve*变量。

## 42

```c
#ifdef CONFIG_KVM_GUEST
	kvmclock_init();
#endif
```

TODO

## 43* page初始化

```c
	x86_init.paging.pagetable_init();
```

x86_64系统结构下，会执行paging_init()函数。

1. 执行稀疏内存初始化。
2. zone和伙伴系统初始化。前面[39]已经执行过numa节点对象的初始化了，zone和伙伴系统初始化在每个numa节点下都会执行。

### 43.1 简要分析

```c
void __init paging_init(void)
{
	sparse_memory_present_with_active_regions(MAX_NUMNODES);
	sparse_init();

	/*
	 * clear the default setting with node 0
	 * note: don't use nodes_clear here, that is really clearing when
	 *	 numa support is not compiled in, and later node_set_state
	 *	 will not set it back.
	 */
	node_clear_state(0, N_MEMORY);
	if (N_MEMORY != N_NORMAL_MEMORY)
		node_clear_state(0, N_NORMAL_MEMORY);

	zone_sizes_init();
}
```

sparse_init()执行稀疏内存初始化。

zone_sizes_init()执行pg_data_t初始化、zone初始化、伙伴系统初始化。

### 43.2 详细分析

[稀疏内存初始化](../../../mm/sparse.md)

[zone及伙伴系统初始化](../../../mm/free_area_init.md)

### 43.3 总结

这之后，所有的numa节点、zone、伙伴系统、struct page都初始化完成了。

## 44

```c
	kasan_init();
```

TODO

## 45

```c
	map_vsyscall();
```

根据"vsyscall="内核参数，把vsyscall页固定映射到VSYSCALL_PAGE(0xffffffffff600000)。-10M。

```
	vsyscall=	[X86-64]
			Controls the behavior of vsyscalls (i.e. calls to
			fixed addresses of 0xffffffffff600x00 from legacy
			code).  Most statically-linked binaries and older
			versions of glibc use these calls.  Because these
			functions are at fixed addresses, they make nice
			targets for exploits that can control RIP.

			emulate     [default] Vsyscalls turn into traps and are
			            emulated reasonably safely.

			native      Vsyscalls are native syscall instructions.
			            This is a little bit faster than trapping
			            and makes a few dynamic recompilers work
			            better than they would in emulation mode.
			            It also makes exploits much easier to write.

			none        Vsyscalls don't work at all.  This makes
			            them quite hard to use for exploits but
			            might break your system.
```

## 46

```c
	early_quirks();
```

修复有bug的pci设备。

## 47* BOOT FACP APIC HPET

```c
	acpi_boot_init();
```

解析ACPI表中的：

```
"BOOT"	/* Simple Boot Flag Table */
"FACP"	/* Fixed ACPI Description Table */
"APIC"	/* Multiple APIC Description Table */
"HPET"	/* High Precision Event Timer table */
```

### 47.1 APIC解析的函数调用链

```
acpi_boot_init() ->
	acpi_process_madt() ->
		acpi_parse_madt_lapic_entries() ->
			acpi_parse_lapic()/acpi_parse_x2apic() ->
				acpi_register_lapic() ->
					generic_processor_info() {
						cpu = allocate_logical_cpuid(apicid);  //--1--
						early_per_cpu(x86_cpu_to_apicid, cpu) = apicid;  //--2--
						set_cpu_possible(cpu, true);  //--3--
						set_cpu_present(cpu, true);  //--4--
					}
		acpi_parse_madt_ioapic_entries() ->
			acpi_parse_ioapic() ->
				mp_register_ioapic() {
					idx = find_free_ioapic_entry();  //--5--
					ioapics[idx].mp_config.apicaddr = address;  //--6--
					set_fixmap_nocache(FIX_IO_APIC_BASE_0 + idx, address);  //--7--
					ioapics[idx].mp_config.apicid = io_apic_unique_id(idx, id);
					gsi_cfg->gsi_base = gsi_base;  //--8--
                    gsi_cfg->gsi_end = gsi_end;
				}
```

1. 根据apicid分配逻辑cpuid。boot cpu的cpuid始终是0，其余逻辑cpu从*cpuid_to_apicid[]*数组中分配。

2. 设置x86_cpu_to_apicid这个每cpu变量。可以根据cpuid转换为apicid，进一步可以有apicid再转换为nid(numa节点)，以完成CPU到numa节点的转换。

3. 设置cpu可能存在标志(possible)。

4. 设置cpu存在标志(present)。在smp_init()中会根据已存在的cpu来逐个开启。

5. 在*ioapic[]*数组中分配空闲的ioapic项。

6. 设置ioapic的物理地址。IOAPIC手册：[82093AA IOAPIC](apic/ioapic.pdf)

   - ```
     [    0.000000] IOAPIC[0]: apic_id 1, version 32, address 0xfec00000, GSI 0-23
     [    0.000000] IOAPIC[1]: apic_id 2, version 32, address 0xfec01000, GSI 24-47
     [    0.000000] IOAPIC[2]: apic_id 3, version 32, address 0xfec40000, GSI 48-71
     ```

   - 从内核日志可以看到有3个IOAPIC，地址分别在0xfec00000，0xfec01000，0xfec40000

7. 设置ioapic的固定映射。通过固定映射的虚拟地址来访问每一个ioapic。
8. Global System Interrupt的起始地址。

IOAPIC寄存器的访问：通过IOREGSEL和IOWIN这两个寄存器来间接访问，这两个寄存器映射到特定的物理地址上(memory address specified by the APICBASE Register located in the PIIX3)。参考：<https://wiki.osdev.org/IOAPIC>

### 47.2 HPET解析的函数调用链

```
acpi_boot_init() ->
	acpi_parse_hpet() {
        hpet_address = hpet_tbl->address.address;  //-1-
        hpet_blockid = hpet_tbl->sequence;  //-2-
        hpet_res = alloc_bootmem(sizeof(*hpet_res) + HPET_RESOURCE_NAME_SIZE);  //-3-
        hpet_res->start = hpet_address;
		hpet_res->end = hpet_address + (1 * 1024) - 1;
	}
```

1. 获取HPET寄存器组的物理地址。
2. TODO
3. 申请*hpet_res*资源(struct resource)，并初始化。通过`cat /proc/iomem | grep HPET`可以看到。

HPET寄存器的访问：通过***hpet_address***这个物理地址直接访问。内核在访问时需要通过`ioremap_nocache`把物理地址映射为虚拟地址才能访问。参考：<https://wiki.osdev.org/HPET>

### 47.3 总结

通过ACPI查找并初始化lapic、ioapic、hpet。查找lapic的主要作用是为了完成lapic到cpuid的相互转换。

## 48

```c
	init_apic_mappings();
```

初始化*boot_cpu_physical_apicid*变量。

## 49

```c
	prefill_possible_map();
```

设置可能存在的cpu图。如果允许热插拔cpu，意味着有些cpu在启动时被禁用了，但是可能存在的。这些cpu也设置成possible的`set_cpu_possible`。

## 50

```c
	init_cpu_to_node();
```

对每个可能存在的CPU，都通过cpu--[47.1]-->lapic--[39]-->node转换成numa节点。并设置x86_cpu_to_node_map这个早期的percpu变量。最终等待percpu内存区域初始化完成后(setup_per_cpu_areas()函数)，可以通过numa_node这个percpu变量之间读取cpu的numa节点。

第[47.1]步完成了cpu跟lapic的关联，第[39]步完成了lapic和numa节点的关联。

这之后，CPU和numa节点的对应关系已经完成早期的初始化。

## 51