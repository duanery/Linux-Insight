# 稀疏内存模型

> [![40](https://github.com/duanery/picture/blob/master/github/github_black_40px.png)](https://duanery.github.io)
> &nbsp;&nbsp;
> ***duanery*** &nbsp;
> 2019年5月11日
>
> Linux爱好者

Linux通过配置可以分为三种内存模型：

1. CONFIG_FLATMEM

   平坦内存模型。主要用于非numa情况下。

2. CONFIG_DISCONTIGMEM

   不连续内存模型。主要用于numa。

3. CONFIG_SPARSEMEM

   稀疏内存模型。主要支持内存hotplug。又衍生出一种CONFIG_SPARSEMEM_VMEMMAP。

详细参考：<http://www.wowotech.net/memory_management/memory_model.html>

这里主要分析稀疏内存模型的初始化。

## 简介



```
x86_64体系物理地址划分：
|____________|_______________|___________________|__________________
0      |     12      |      27          |        46
       |             |                  `使用struct mem_section来描述一个section
       |             `使用struct page来描述一个页。
       `0-12位描述页框内偏移
一个物理地址右移12位，可以得到对应的页框，每个页框都由对应的struct page来描述。
一个物理地址右移27位，可以得到对应的section，每个section由对应的struct mem_section来描述。
x86_64定义一个section是128M。
```

稀疏内存模式则多增加了一个`struct mem_section`来描述每个可hotplug的内存块。所有的`struct mem_section`描述符存放在***mem_section***数组中。

```c
#ifdef CONFIG_SPARSEMEM_EXTREME
struct mem_section *mem_section[NR_SECTION_ROOTS]
	____cacheline_internodealigned_in_smp;
#else
struct mem_section mem_section[NR_SECTION_ROOTS][SECTIONS_PER_ROOT]
	____cacheline_internodealigned_in_smp;
#endif
EXPORT_SYMBOL(mem_section);
```

## 初始化

```c
void __init paging_init(void)
{
	sparse_memory_present_with_active_regions(MAX_NUMNODES);
	sparse_init();
    ...
}
```

稀疏内存主要在page_init()中开始初始化的。sparse_memory_present_with_active_regions()函数从memblock中获取每一个内存区域(this_nid，start_pfn，end_pfn)，调用memory_present()函数。memory_present()函数只是标记从start_pfn到end_pfn对应的section存在。

```c
void __init memory_present(int nid, unsigned long start, unsigned long end)
{
	unsigned long pfn;

	start &= PAGE_SECTION_MASK;
	mminit_validate_memmodel_limits(&start, &end);
	for (pfn = start; pfn < end; pfn += PAGES_PER_SECTION) {
		unsigned long section = pfn_to_section_nr(pfn);    //--1--
		struct mem_section *ms;

		sparse_index_init(section, nid);    //--2--
		set_section_nid(section, nid);

		ms = __nr_to_section(section);    //--3--
		if (!ms->section_mem_map)
			ms->section_mem_map = sparse_encode_early_nid(nid) |
							SECTION_MARKED_PRESENT;    //--4--
	}
}
```

1. pfn转换为section
2. 如果配置CONFIG_SPARSEMEM_EXTREME，就会分配一个4K大小的内存来存放多个连续的`struct mem_section`描述符，这样可以更节省内存。极度稀疏。分配4K内存会从对应的numa节点内划分。
3. 得到section描述符。
4. 把nid和PRESENT标记都编码在section_mem_map字段。后续会用到。

sparse_init()函数为PRESENT的section分配usemap_map和mem_map.

```c
void __init sparse_init(void)
{
    ...
	size = sizeof(unsigned long *) * NR_MEM_SECTIONS;
	usemap_map = memblock_virt_alloc(size, 0);
	alloc_usemap_and_memmap(sparse_early_usemaps_alloc_node,    //--1--
							(void *)usemap_map);

#ifdef CONFIG_SPARSEMEM_ALLOC_MEM_MAP_TOGETHER
	size2 = sizeof(struct page *) * NR_MEM_SECTIONS;
	map_map = memblock_virt_alloc(size2, 0);
	alloc_usemap_and_memmap(sparse_early_mem_maps_alloc_node,    //--2--
							(void *)map_map);
#endif
    
    for (pnum = 0; pnum < NR_MEM_SECTIONS; pnum++) {
		if (!present_section_nr(pnum))
			continue;

		usemap = usemap_map[pnum];
		if (!usemap)
			continue;

#ifdef CONFIG_SPARSEMEM_ALLOC_MEM_MAP_TOGETHER
		map = map_map[pnum];
#else
		map = sparse_early_mem_map_alloc(pnum);
#endif
		if (!map)
			continue;

		sparse_init_one_section(__nr_to_section(pnum), pnum, map,    //--3--
								usemap);
	}
```

1. 对所有PRESENT的section分配usemap。一个section内每2M字节会分配4个位来描述pageblock的标志，所以一个section内需要分配64*4个位来存放usemap。
2. 对所有PRESENT的section分配memmap。这个函数会在***vmemmap***处建立页表映射。后续可以通过vmemmap来访问struct page结构体。重点分析。
3. 把usemap和memmap安装到section中。

sparse_early_mem_maps_alloc_node()会调用sparse_mem_maps_populate_node()函数。

```c
void __init sparse_mem_maps_populate_node(struct page **map_map,
					  unsigned long pnum_begin,
					  unsigned long pnum_end,
					  unsigned long map_count, int nodeid)
{
	unsigned long pnum;
	unsigned long size = sizeof(struct page) * PAGES_PER_SECTION;
	void *vmemmap_buf_start;

	size = ALIGN(size, PMD_SIZE);
	vmemmap_buf_start = __earlyonly_bootmem_alloc(nodeid, size * map_count,
			 PMD_SIZE, __pa(MAX_DMA_ADDRESS));    //--1--

	if (vmemmap_buf_start) {
		vmemmap_buf = vmemmap_buf_start;    //--2--
		vmemmap_buf_end = vmemmap_buf_start + size * map_count;
	}

	for (pnum = pnum_begin; pnum < pnum_end; pnum++) {
		struct mem_section *ms;

		if (!present_section_nr(pnum))
			continue;

		map_map[pnum] = sparse_mem_map_populate(pnum, nodeid);    //--3--
		if (map_map[pnum])
			continue;
	}
	...
}

```

1. 从对应的numa节点分配物理内存，仍然是从memblock中分配的。每个section会有2^15个struct page结构体，总共有map_count个PRESENT的section，需要分配`size * map_count`字节。不同的numa节点的cpu在访问struct page结构体时，都会非常快速。
2. 缓冲区赋值。后续会用到。
3. sparse_mem_map_populate()函数建立从vmemmap到物理内存的映射。以后通过vmemmap+pfn就可以访问到pfn页框对应的struct page结构体。重点分析。

```c
struct page * __meminit sparse_mem_map_populate(unsigned long pnum, int nid)
{
	unsigned long start;
	unsigned long end;
	struct page *map;

	map = pfn_to_page(pnum * PAGES_PER_SECTION);    //--1--
	start = (unsigned long)map;
	end = (unsigned long)(map + PAGES_PER_SECTION);

	if (vmemmap_populate(start, end, nid))    //--2--
		return NULL;

	return map;
}
```

1. 计算得到section对应的struct page数组对应的虚拟地址。`pnum * PAGES_PER_SECTION`先转换成物理页框号。pfn_to_page再把物理页框号转换成struct page*虚拟地址。在启用配置CONFIG_SPARSEMEM_VMEMMAP之后，这个虚拟地址就是vmemmap+pfn得到的。
2. 会把从start到end对应的虚拟地址都映射到前面所分配的struct page物理内存。映射的中间页表(pud,pmd,pte)都会从对应的numa节点中分配。

## 总结

在初始化完成之后，mem_section中usemap和memmap都初始化好，之后pfn_to_page得到的struct page就能够正常读写。