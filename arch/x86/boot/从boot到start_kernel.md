# 从boot到start_kernel

> [![40](https://github.com/duanery/picture/blob/master/github/github_black_40px.png)](https://duanery.github.io)
> &nbsp;&nbsp;
> ***duanery*** &nbsp;
> 2019年2月13日
>
> Linux爱好者

从实模式代码到内核初始化函数start_kernel中间会经历：从实模式切换到保护模板，再切换到IA-32e兼容模式，再切换到64位模式。以及页表建立。

## 实模式(setup.bin)

实模式代码从`arch/x86/boot/header.S`开始，跳入`arch/x86/boot/main.c`文件的main函数。

boot_params结构定义在setup.bin内部。会把header.S中的头拷贝到boot_params结构中。

检测内存（e820），存入到boot_params结构中。

安装gdt。

跳入保护模式代码。cs会设置为__BOOT_CS，ds设置为\_\_BOOT_DS.

## 保护模式

保护模式代码从`arch/x86/boot/compressed/head_64.S`文件的`startup_32`开始。

1. 重新加载gdt，使用64位segment。

2. 建立虚拟地址0-4G到物理地址0-4G的对等映射，使用的是4级页表。
3. 把4级页表写入cr3.
4. 开启long mode（64位模式），之后会进入兼容模式（因为目前的cs还是__BOOT_CS）。
5. 开启分页。
6. 跳入64位模式。cs会重新设置为__KERNEL_CS，指令IP会使用64位虚拟地址。

## 64位模式

64位模式代码从`arch/x86/boot/compressed/head_64.S`文件的`startup_64`开始。

1. 建立栈
2. 拷贝压缩的内核及解压代码到解压buffer末尾
3. 跳入解压代码（因为解压代码被拷贝到解压buffer末尾了）
4. 调用extract_kernel解压内核
   - 启用KASLR，会把内核解压到随机选择的物理地址上。（16M-64T(最大的物理地址)）
     - 会错开boot_params, cmdline, initrd, 及压缩的内核，占用的物理地址。
   - 解压vmlinux
   - 解析elf格式的vmlinux，把相应的segment拷贝到正确的加载地址
5. 跳入vmlinux之中

## 进入start_kernel

代码从`arch/x86/kernel/head_64.S`开始。

1. 默认的early_level4_pgt会自带一个0xffffffff8000000虚拟地址到0-2G物理地址的映射。

2. 建立vmlinux所占用物理内存块的对等映射

3. 修复0xffffffff8000000虚拟地址的映射，使其映射到vmlinux对应的物理内存上。

4. 把early_level4_pgt载入cr3，建立新的页表
   - 新页表包含vmlinux物理地址的映射（KASLR随机选择的物理地址，或者0x100000）
   - 新页表包含vmlinux虚拟地址的映射（0xffffffff8000000）

   ```asm
   	movq	$(early_level4_pgt - __START_KERNEL_map), %rax
   	...
   /* Setup early boot stage 4 level pagetables. */
   	addq	phys_base(%rip), %rax
   	movq	%rax, %cr3
   ```

5. 跳入vmlinux虚拟地址中

   ```asm
   	/* Ensure I am executing from virtual addresses */
   	movq	$1f, %rax
   	ANNOTATE_RETPOLINE_SAFE
   	jmp	*%rax
   1:
   ```

   从此时开始后续则全部进入内核虚拟地址空间中了。

6. 初始化栈，主要是把init_thread_union加载到rsp。

   ```asm
   /* Setup a boot time stack */
   movq initial_stack(%rip), %rsp
   
   GLOBAL(initial_stack)
   	.quad  init_thread_union+THREAD_SIZE-8
   ```

7. 重新加载新的gdt

   ```asm
   lgdt	early_gdt_descr(%rip)
   
   	.data
   	.align 16
   	.globl early_gdt_descr
   early_gdt_descr:
   	.word	GDT_ENTRIES*8-1
   early_gdt_descr_base:
   	.quad	INIT_PER_CPU_VAR(gdt_page)
   ```

   gdt_page地址存放的是GDT描述符表。

8. Set up %gs.

   ```asm
   	/* Set up %gs.
   	 *
   	 * The base of %gs always points to the bottom of the irqstack
   	 * union.  If the stack protector canary is enabled, it is
   	 * located at %gs:40.  Note that, on SMP, the boot cpu uses
   	 * init data section till per cpu areas are set up.
   	 */
   	movl	$MSR_GS_BASE,%ecx
   	movl	initial_gs(%rip),%eax
   	movl	initial_gs+4(%rip),%edx
   	wrmsr
   ```

   从注释可以看到设置gs就设置了percpu区域及stack protector。进一步分析：

   - initial_gs其实指向了irq_stack_union这块区域

     ```asm
     GLOBAL(initial_gs)
     .quad	INIT_PER_CPU_VAR(irq_stack_union)
     ```

   - x86/include/asm/percpu.h中定义INIT_PER_CPU_VAR

     ```c
     #define INIT_PER_CPU_VAR(var)  init_per_cpu__##var
     ```

     于是，INIT_PER_CPU_VAR(irq_stack_union)被定义为init_per_cpu__irq_stack_union

   - vmlinux.lds.S链接脚本中定义INIT_PER_CPU

     ```asm
     #define INIT_PER_CPU(x) init_per_cpu__##x = x + __per_cpu_load
     INIT_PER_CPU(gdt_page);
     INIT_PER_CPU(irq_stack_union);
     ```

     可以看到`init_per_cpu__irq_stack_union`等于`irq_stack_union + __per_cpu_load`

   - arch/x86/kernel/cpu/common.c中定义irq_stack_union

     ```c
     DEFINE_PER_CPU_FIRST(union irq_stack_union,
     		     irq_stack_union) __aligned(PAGE_SIZE) __visible;
     ```

     经过对DEFINE_PER_CPU_FIRST的分析可以知道irq_stack_union存放在`.data..percpu..first` section中

   - 看看vmlinux.lds.S中引用的PERCPU_VADDR的定义

     ```c
     #define PERCPU_INPUT(cacheline)						\
     	VMLINUX_SYMBOL(__per_cpu_start) = .;				\
     	VMLINUX_SYMBOL(__per_cpu_user_mapped_start) = .;		\
     	*(.data..percpu..first)						\
     	. = ALIGN(cacheline);						\
     	...											\
     	...											\
     	VMLINUX_SYMBOL(__per_cpu_end) = .;
     
     #define PERCPU_VADDR(cacheline, vaddr, phdr)				\
     	VMLINUX_SYMBOL(__per_cpu_load) = .;				\
     	.data..percpu vaddr : AT(VMLINUX_SYMBOL(__per_cpu_load)		\
     				- LOAD_OFFSET) {			\
     		PERCPU_INPUT(cacheline)					\
     	} phdr								\
     	. = VMLINUX_SYMBOL(__per_cpu_load) + SIZEOF(.data..percpu);
     
     ```

     可以看到__per_cpu_load的定义，及`.data..percpu..first` section放在percpu区域的最开始的位置。

   - 可以得出结论：gs的base值初始化为`irq_stack_union + __per_cpu_load`，就是初始化为percpu的起始区域。可以用%gs:的方式访问所有的percpu变量。

9. 跳入x86_64_start_kernel函数中

   - 清除vmlinux物理地址的映射

   - 建立早期的中断，主要处理缺页异常。

   - 拷贝boot_params及boot_command_line，`copy_bootdata(__va(real_mode_data));`

     - __va 先把物理地址转换为虚拟地址，但虚拟地址的页表是未建立的，所以访问该虚拟地址就会触发缺页异常，在缺页异常内建立虚拟地址到对应的物理地址的映射。参考early_make_pgtable函数。

       `arch/x86/include/asm/page_64_types.h`

       ```c
       #define __PAGE_OFFSET_BASE      _AC(0xffff880000000000, UL)
       #ifdef CONFIG_RANDOMIZE_MEMORY
       #define __PAGE_OFFSET           page_offset_base
       #else
       #define __PAGE_OFFSET           __PAGE_OFFSET_BASE
       #endif /* CONFIG_RANDOMIZE_MEMORY */
       ```

       `arch/x86/include/asm/page_types.h`

       ```c
       #define PAGE_OFFSET		((unsigned long)__PAGE_OFFSET)
       ```

       `arch/x86/include/asm/page_types.h`

       ```c
       #define __va(x)			((void *)((unsigned long)(x)+PAGE_OFFSET))
       ```

       __va 把x加上0xffff880000000000而转换为虚拟地址。

   - ```c 
     init_level4_pgt[511] = early_level4_pgt[511]; 
     ```
   - ```c
     start_kernel();
     ```

## 进入内核这一刻

1. GDT描述符已经建立好，并且不会再调用ldgt重新加载新的GDT了。

2. 页表(cr3寄存器)使用的是early_level4_pgt，这个结构存放在.init.data section中，最终会被释放掉，所以这不是最终的页表。只是临时页表。

3. 临时页表只映射了 ffffffff80000000 - ffffffff9fffffff 这块内核空间。

4. 建立了一个早期的中断，处理缺页异常，用来在 0xffff880000000000 地址处建立物理地址的直接映射。

5. boot cpu可以基于%gs:的方式访问任意的percpu变量。只是这时访问的percpu是初始的percpu区域。

6. current指向了init_task任务。

   ```c
   DEFINE_PER_CPU(struct task_struct *, current_task) ____cacheline_aligned =
   	&init_task;
   static __always_inline struct task_struct *get_current(void)
   {
   	return this_cpu_read_stable(current_task);
   }
   
   #define current get_current()
   ```

   current是读取current_task这个percpu变量的，而current_task的初始值就是init_task。


## 疑问

1. vmlinux是elf格式的

2. start_kernel是运行在虚拟地址空间中的。

   ```
   ffffffff80000000 - ffffffff9fffffff (=512 MB)  kernel text mapping
   ```

3. boot_params起初是在setup.bin的.bss段中，最后被拷贝到vmlinux的.bss段中。与setup.bin所在的物理地址无关了。cmdline也是一样的。

   ```bash
   [root@localhost boot]# nm setup.elf | grep boot_params$
   00004520 B boot_params
   [root@localhost boot]# nm ../../../vmlinux | grep boot_params$
   ffffffff8202c120 B boot_params
   ```

   

## 参考文档

### 内核文档

Documentation/x86/x86_64/mm.txt

### 外网链接
*\[1\] [Booting][1]*

[1]: https://0xax.gitbooks.io/linux-insides/content/Booting/ "linux-insides"