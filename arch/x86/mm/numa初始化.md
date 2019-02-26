# NUMA初始化

> [![40](https://github.com/duanery/picture/blob/master/github/github_black_40px.png)](https://duanery.github.io)
> &nbsp;&nbsp;
> ***duanery*** &nbsp;
> 2019年2月26日
>
> Linux爱好者

NUMA包含CPU和内存与numa节点的对应关系。

## 初始化

`arch/x86/kernel/setup.c`

```c
void __init setup_arch(char **cmdline_p)
{
    ...
    acpi_boot_table_init();   //acpi初始化
	early_acpi_boot_init();  
	initmem_init();    //1
    ...
}
```

NUMA需要通过acpi接口获取cpu,内存和numa节点的对应关系，所以numa初始化在acpi初始化之后。

`arch/x86/mm/numa_64.c`

```c
void __init initmem_init(void)
{
	x86_numa_init();
}
```

`arch/x86/mm/numa.c`

```c
static int __init numa_init(int (*init_func)(void))
{
    ...
    ret = init_func();           //调用x86_acpi_numa_init
	if (ret < 0)
		return ret;
    memblock_set_bottom_up(false);

	ret = numa_cleanup_meminfo(&numa_meminfo);
	if (ret < 0)
		return ret;
    ret = numa_register_memblks(&numa_meminfo);
	if (ret < 0)
		return ret;
}
void __init x86_numa_init(void)
{
	if (!numa_off) {
#ifdef CONFIG_ACPI_NUMA
		if (!numa_init(x86_acpi_numa_init))
			return;
#endif
#ifdef CONFIG_AMD_NUMA
		if (!numa_init(amd_numa_init))
			return;
#endif
	}
	numa_init(dummy_numa_init);
}
```

x86_numa_init通过调用numa_init来完成初始化。会首先调用x86_acpi_numa_init来进行acpi中numa的初始化。

`arch/x86/mm/sart.c`

```CQL
int __init x86_acpi_numa_init(void)
{
	int ret;

	ret = acpi_numa_init();
	if (ret < 0)
		return ret;
	return srat_disabled() ? -EINVAL : 0;
}
```

`drivers/acpi/numa.c`

```c
int __init acpi_numa_init(void)
{
    	/* SRAT: Static Resource Affinity Table */
	if (!acpi_table_parse(ACPI_SIG_SRAT, acpi_parse_srat)) {
		struct acpi_subtable_proc srat_proc[3];

		memset(srat_proc, 0, sizeof(srat_proc));
		srat_proc[0].id = ACPI_SRAT_TYPE_CPU_AFFINITY;
		srat_proc[0].handler = acpi_parse_processor_affinity;
		srat_proc[1].id = ACPI_SRAT_TYPE_X2APIC_CPU_AFFINITY;
		srat_proc[1].handler = acpi_parse_x2apic_affinity;
		srat_proc[2].id = ACPI_SRAT_TYPE_GICC_AFFINITY;
		srat_proc[2].handler = acpi_parse_gicc_affinity;

		acpi_table_parse_entries_array(ACPI_SIG_SRAT,
					sizeof(struct acpi_table_srat),
					srat_proc, ARRAY_SIZE(srat_proc), 0);

		cnt = acpi_table_parse_srat(ACPI_SRAT_TYPE_MEMORY_AFFINITY,
					    acpi_parse_memory_affinity,
					    NR_NODE_MEMBLKS);
	}
	/* SLIT: System Locality Information Table */
	acpi_table_parse(ACPI_SIG_SLIT, acpi_parse_slit);
}
```

会在acpi中搜索**SRAT**结构，然后调用`acpi_parse_processor_affinity`解析cpu亲和性相关的表，调用`acpi_parse_x2apic_affinity`解析x2apic亲和性相关的表，以及调用`acpi_parse_memory_affinity`解析内存亲和性相关的表。我们重点关注内存亲和性相关的内容。

内存亲和性指定了（内存区域，numa节点）这样的关联信息。`acpi_parse_memory_affinity`会调用`acpi_parse_memory_affinity_init`，进而调用`numa_add_memblk`把内存亲和性信息传递出去。具体的内存亲和性信息被加入了numa_meminfo结构中。

我们回到`arch/x86/mm/numa.c`的`numa_init`函数中，看看`numa_register_memblks`

```c
static int __init numa_register_memblks(struct numa_meminfo *mi)
{
    ...
    /* Account for nodes with cpus and no memory */
	node_possible_map = numa_nodes_parsed;
	numa_nodemask_from_meminfo(&node_possible_map, mi);
	if (WARN_ON(nodes_empty(node_possible_map)))
		return -EINVAL;

	for (i = 0; i < mi->nr_blks; i++) {
		struct numa_memblk *mb = &mi->blk[i];
		memblock_set_node(mb->start, mb->end - mb->start,
				  &memblock.memory, mb->nid);
	}
    ...
}
```

通过`memblock_set_node`，最终numa_meminfo中的内存亲和性信息被转嫁到了memblock模块上。memblock是内核启动初期bootmem分配器，memblock也负责管理numa节点与内存的对应关系。

