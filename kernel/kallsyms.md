# kallsyms

> [![40](https://github.com/duanery/picture/blob/master/github/github_black_40px.png)](https://duanery.github.io)
> &nbsp;&nbsp;
> ***duanery*** &nbsp;
> 2019年1月23日
> 
> Linux爱好者

在内核的各个核心目录编译完成后，链接生成vmlinux时，会通过`nm -n .tmp_vmlinux1`导出所有的内核符号，并由`scripts/kallsyms`生成一个.S汇编文件，编译该.S文件后再次链接到vmlinux内部，最终由内核的`kernel/kallsyms.c`源码文件处理所有的内核符号。

## 编译

```bash
# scripts/link-vmlinux.sh
kallsyms()
{
    local kallsymopt;
    local afile="`basename ${2} .o`.S"

    ${NM} -n ${1} | scripts/kallsyms ${kallsymopt} > ${afile}
    ${CC} ${aflags} -c -o ${2} ${afile}
}
if [ -n "${CONFIG_KALLSYMS}" ]; then
    # step 1
    vmlinux_link "" .tmp_vmlinux1
    kallsyms .tmp_vmlinux1 .tmp_kallsyms1.o

    # step 2
    vmlinux_link .tmp_kallsyms1.o .tmp_vmlinux2
    kallsyms .tmp_vmlinux2 .tmp_kallsyms2.o
fi
info LD vmlinux
vmlinux_link "${kallsymso}" vmlinux
```

step 1,先把内核核心目录链接成.tmp_vmlinux1文件，之后`kallsyms`函数会把内核符号编译.tmp_kallsyms1.o文件。

step 2,再次运行一遍这个过程。为什么要进行setp 2？把.tmp_kallsyms1.o是要链接到vmlinux的rodata段的，会导致其他段的符号地址有一定偏移。所以step2，先把.tmp_kallsyms1.o 链接到.tmp_vmlinux2，之后再生成.tmp_kallsyms2.o，最后再把.tmp_kallsyms2.o链接到vmlinux。

`scripts/kallsyms`把内核符号编译后的数据是怎样存放的？

| 数据结构                   | 描述                      |
| ---------------------- | ----------------------- |
| kallsyms_offsets       | 数组，存放每个符号的相对地址，有序       |
| kallsyms_relative_base | 所有符号的相对地址               |
| kallsyms_num_syms      | 内核符号的总数                 |
| kallsyms_names         | 内核符号名字，经过压缩的            |
| kallsyms_markers       | 每256个符号，存放一个标记，用于快速查找符号 |
| kallsyms_token_table   | 压缩内核符号的token            |
| kallsyms_token_index   | token的索引                |

详细的分析`scripts/kallsyms.c`源码文件，比较简单。

根据以上的数据结构，就可以查找任何一个内核符号的地址。

- 根据符号名查找符号地址

- 根据IP（指令地址）查找该地址属于哪个函数

## 初始化

```c
static int __init kallsyms_init(void)
{
    proc_create("kallsyms", 0444, NULL, &kallsyms_operations);
    return 0;
}
```

初始化创建/proc/kallsyms文件。

## 源码简析

看`kernel/kallsyms.c`源码文件，比较简单。

核心代码是根据任意的内核地址，查找该地址属于哪个符号。

```c
static unsigned long get_symbol_pos(unsigned long addr,
                    unsigned long *symbolsize,
                    unsigned long *offset)
{
    unsigned long symbol_start = 0, symbol_end = 0;
    unsigned long i, low, high, mid;

    /* Do a binary search on the sorted kallsyms_addresses array. */
    low = 0;
    high = kallsyms_num_syms;

    while (high - low > 1) {
        mid = low + (high - low) / 2;
        if (kallsyms_sym_address(mid) <= addr)
            low = mid;
        else
            high = mid;
    }
```

根据kallsyms_offsets给出的每个符号的地址，二分查找比addr小的最大地址。

```c
    /*
     * Search for the first aliased symbol. Aliased
     * symbols are symbols with the same address.
     */
    while (low && kallsyms_sym_address(low-1) == kallsyms_sym_address(low))
        --low;
```

过滤别名符号

```c
    symbol_start = kallsyms_sym_address(low);

    /* Search for next non-aliased symbol. */
    for (i = low + 1; i < kallsyms_num_syms; i++) {
        if (kallsyms_sym_address(i) > symbol_start) {
            symbol_end = kallsyms_sym_address(i);
            break;
        }
    }

    if (symbolsize)
        *symbolsize = symbol_end - symbol_start;
    if (offset)
        *offset = addr - symbol_start;

    return low;
}
```

确定符号的大小及偏移。

符号大小（如函数）指的是从符号的起始地址到下一个符号的起始地址对应的范围。

偏移指的的addr距离符号起始地址的位置。

### 符号名查找

`get_symbol_pos`获取符号的pos，`get_symbol_offset`取得偏移，`kallsyms_expand_symbol`解析符号名

## 接口

### proc

```bash
[root@localhost ~]# cat /proc/kallsyms | tail
ffffffffa0003630 t dm_set_mdptr    [dm_mod]
ffffffffa00059f0 t dm_table_get_immutable_target_type    [dm_mod]
ffffffffa0004090 t dm_suspended_md    [dm_mod]
ffffffffa00068b0 t dm_table_postsuspend_targets    [dm_mod]
ffffffffa0006e30 t dm_target_iterate    [dm_mod]
ffffffffa00003e0 t dm_get_reserved_bio_based_ios    [dm_mod]
ffffffffa0005a10 t dm_table_get_immutable_target    [dm_mod]
ffffffffa0004b40 T dm_consume_args    [dm_mod]
ffffffffa000a5d0 t dm_copy_name_and_uuid    [dm_mod]
ffffffffa000f5d0 t dm_request_based    [dm_mod]
```

| 符号地址 | 类型                  | 符号名  | 模块名          |
| ---- | ------------------- | ---- | ------------ |
| 有序地址 | 通过`man nm`可以查看所有的类型 | 符号名字 | 模块内的符号则显示模块名 |

### EXPORT_SYMBOL

| 函数                                       | 描述                                              |
| ---------------------------------------- | ----------------------------------------------- |
| kallsyms_lookup_name(name)               | 根据name查找符号地址                                    |
| kallsyms_on_each_symbol(fn, data)        | 遍历每个符号                                          |
| sprint_symbol(buffer, address)           | 打印address对应的符号到buffer `name +offset/size [mod]` |
| sprint_symbol_no_offset(buffer, address) | 打印address对应的符号到buffer 不包含`+offset/size`         |
| __print_symbol(fmt, address)             | 以fmt指定的格式打印address对应的符号                         |

## 参考文档

### man手册

man nm

### 外网链接

\[1\] [Linux kallsyms 机制分析](https://blog.csdn.net/kehyuanyu/article/details/46346321 "CSDN")
