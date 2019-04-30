# 中断框架

> [![40](https://github.com/duanery/picture/blob/master/github/github_black_40px.png)](https://duanery.github.io)
> &nbsp;&nbsp;
> ***duanery*** &nbsp;
> 2019年4月30日
>
> Linux爱好者

介绍x86体系结构中断和陷阱的一些知识。

## 中断描述符

`arch/x86/kernel/cpu/common.c`

```c
#ifdef CONFIG_X86_64
struct desc_ptr idt_descr __ro_after_init = {
	.size = NR_VECTORS * 16 - 1,
	.address = (unsigned long) idt_table,
};
const struct desc_ptr debug_idt_descr = {
	.size = NR_VECTORS * 16 - 1,
	.address = (unsigned long) debug_idt_table,
};
...
```

`idt_descr`定义了中断描述符，包含256个中断向量，及`idt_table`中断描述符表。idt_descr是可以通过`lidt`指令加载到中断描述符寄存器中的。参见load_ldt宏。

而中断描述表定义在`arch/x86/kernel/traps.c`中

```c
/* Must be page-aligned because the real IDT is used in a fixmap. */
gate_desc idt_table[NR_VECTORS] __page_aligned_bss;

DECLARE_BITMAP(used_vectors, NR_VECTORS);
EXPORT_SYMBOL_GPL(used_vectors);
```

中断描述符表中的每一个向量，描述一个中断门或陷阱门。中断门在执行中断函数时会关中断。陷阱门在执行异常函数时不会关中断。

### 中断门和陷阱门

`arch\x86\include\asm\desc.h`

```c
static inline void pack_gate(gate_desc *gate, unsigned type, unsigned long func,
			     unsigned dpl, unsigned ist, unsigned seg)
{
	gate->offset_low	= PTR_LOW(func);
	gate->segment		= __KERNEL_CS;
	gate->ist		= ist;
	gate->p			= 1;
	gate->dpl		= dpl;
	gate->zero0		= 0;
	gate->zero1		= 0;
	gate->type		= type;
	gate->offset_middle	= PTR_MIDDLE(func);
	gate->offset_high	= PTR_HIGH(func);
}
static inline void _set_gate(int gate, unsigned type, void *addr,
			     unsigned dpl, unsigned ist, unsigned seg)
{
	gate_desc s;

	pack_gate(&s, type, (unsigned long)addr, dpl, ist, seg);
	/*
	 * does not need to be atomic because it is only done once at
	 * setup time
	 */
	write_idt_entry(idt_table, gate, &s);
	write_trace_idt_entry(gate, &s);
}
```

任何一个中断向量的设置都最终会调用到_set_gate函数。通过该函数的参数可以得知，需要指定中断向量(gate)，门的类型(type)，中断处理程序的地址(addr)，描述符特权级(dpl)，代码段(seg)等。

且中断向量最终都写入了`idt_table`这个中断向量表。

- 门类型(type)，区分是中断门还是陷阱门。中断门在执行中断函数时会关中断。陷阱门在执行异常函数时不会关中断。

  ```c
  enum {
  	GATE_INTERRUPT = 0xE, //中断门
  	GATE_TRAP = 0xF,      //陷阱门
  	GATE_CALL = 0xC,
  	GATE_TASK = 0x5,
  };
  ```

- 描述符特权级(dpl)是用于用户态是否可以触发中断，如dpl==3，用户态指令int 128，into等指令可以触发中断，进入内核态调用中断处理程序。

于是根据dpl的值，就分成了四类门（中断门，陷阱门，系统中断门，系统陷阱门）

```c
#define set_intr_gate_notrace(n, addr)					\
	do {								\
		BUG_ON((unsigned)n > 0xFF);				\
		_set_gate(n, GATE_INTERRUPT, (void *)addr, 0, 0,	\
			  __KERNEL_CS);					\
	} while (0)

#define set_intr_gate(n, addr)						\
	do {								\
		set_intr_gate_notrace(n, addr);				\
		_trace_set_gate(n, GATE_INTERRUPT, (void *)trace_##addr,\
				0, 0, __KERNEL_CS);			\
	} while (0)
static inline void set_system_intr_gate(unsigned int n, void *addr)
{
	BUG_ON((unsigned)n > 0xFF);
	_set_gate(n, GATE_INTERRUPT, addr, 0x3, 0, __KERNEL_CS);
}

static inline void set_system_trap_gate(unsigned int n, void *addr)
{
	BUG_ON((unsigned)n > 0xFF);
	_set_gate(n, GATE_TRAP, addr, 0x3, 0, __KERNEL_CS);
}

static inline void set_trap_gate(unsigned int n, void *addr)
{
	BUG_ON((unsigned)n > 0xFF);
	_set_gate(n, GATE_TRAP, addr, 0, 0, __KERNEL_CS);
}
```

中断门：`set_intr_gate`  type = GATE_INTERRUPT,	dpl = 0

陷阱门：`set_trap_gate`  type = GATE_TRAP, 		dpl = 0

系统中断门：`set_system_intr_gate`  type = GATE_INTERRUPT, 	dpl = 3

系统陷阱门：`set_system_trap_gate`  type = GATE_TRAP, 			dpl = 3

