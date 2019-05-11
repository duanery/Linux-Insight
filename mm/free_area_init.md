# free_area_init

> [![40](https://github.com/duanery/picture/blob/master/github/github_black_40px.png)](https://duanery.github.io)
> &nbsp;&nbsp;
> ***duanery*** &nbsp;
> 2019年5月11日
>
> Linux爱好者

free_area_init统一指Linux内核中pg_data_t(标识一个numa节点)初始化，及其中的zone的初始化，zone中伙伴系统的初始化。

## 初始化

这一切都要从paging_init()函数开始。

```c
void __init paging_init(void)
{
	...
	zone_sizes_init();
}
```

```c
void __init zone_sizes_init(void)
{
	unsigned long max_zone_pfns[MAX_NR_ZONES];

	memset(max_zone_pfns, 0, sizeof(max_zone_pfns));

#ifdef CONFIG_ZONE_DMA
	max_zone_pfns[ZONE_DMA]		= min(MAX_DMA_PFN, max_low_pfn);
#endif
#ifdef CONFIG_ZONE_DMA32
	max_zone_pfns[ZONE_DMA32]	= min(MAX_DMA32_PFN, max_low_pfn);
#endif
	max_zone_pfns[ZONE_NORMAL]	= max_low_pfn;
#ifdef CONFIG_HIGHMEM
	max_zone_pfns[ZONE_HIGHMEM]	= max_pfn;
#endif

	free_area_init_nodes(max_zone_pfns);
}
```

pagint_init()最终会调用到zone_size_init()函数。zone_size_init()函数先初始化各个zone的边界，ZONE_DMA的最大边界为16M，ZONE_DMA32的最大边界为4G，ZONE_NORMAL的最大边界为最大物理内存(x86_64体系结构没有定义CONFIG_HIGHMEM，且max_low_pfn=max_pfn)，这里的ZONE的边界未区分numa节点；之后调用free_area_init_nodes()真正进入初始化之中。

## free_area_init_nodes()

该函数主要执行：

1. 确定每一个zone的lowest和highest的页框号，并存放在*arch_zone_lowest_possible_pfn[]*和*arch_zone_highest_possible_pfn[]*全局变量中。

2. 根据"movable_node"，"kernelcore"，"movablecore"这三个内核参数的配置，确定ZONE_MOVABLE的边界。并把边界存放在*zone_movable_pfn[nid]*全局变量中。

3. ***nr_node_ids***赋值为系统numa节点个数。

4. 对每个numa节点初始化。

   ```c
   for_each_online_node(nid) {
       pg_data_t *pgdat = NODE_DATA(nid);
       free_area_init_node(nid, NULL,
                           find_min_pfn_for_node(nid), NULL);
   
       /* Any memory on that node */
       if (pgdat->node_present_pages)
           node_set_state(nid, N_MEMORY);
       check_for_memory(pgdat, nid);
   }
   ```

   核心的函数是free_area_init_node();

## free_area_init_node()

该函数对一个numa节点(pg_data_t)进行初始化。

```c
void __paginginit free_area_init_node(int nid, unsigned long *zones_size,
		unsigned long node_start_pfn, unsigned long *zholes_size)
{
    pg_data_t *pgdat = NODE_DATA(nid);
	unsigned long start_pfn = 0;
	unsigned long end_pfn = 0;
    
    pgdat->node_id = nid;
	pgdat->node_start_pfn = node_start_pfn;
	pgdat->per_cpu_nodestats = NULL;
```

对*pgdat*进行基本初始化。node_id，起始页框等。

```c
#ifdef CONFIG_HAVE_MEMBLOCK_NODE_MAP
	get_pfn_range_for_nid(nid, &start_pfn, &end_pfn);
	pr_info("Initmem setup node %d [mem %#018Lx-%#018Lx]\n", nid,
		(u64)start_pfn << PAGE_SHIFT,
		end_pfn ? ((u64)end_pfn << PAGE_SHIFT) - 1 : 0);
#else
	start_pfn = node_start_pfn;
#endif
```

get_pfn_range_for_nid()函数获取nid节点对应的起始页框和终止页框，遍历memblock中与nid节点相关的内存得到的。

```c
	calculate_node_totalpages(pgdat, start_pfn, end_pfn,
				  zones_size, zholes_size);
```

计算每一个zone中横跨的页数(spanned_pages)和存在的页数(present_pages)，最后累加得到nid节点横跨的页数(node_spanned_pages)和存在的页数(node_present_pages)。

一些特殊情况：

1. ZONE_MOVABLE可能会存在，因此这个zone也会有横跨的页数和存在的页数。
2. nid=0节点才有ZONE_DMA,ZONE_DMA32
3. nid>=1的节点只有ZONE_NORMAL和ZONE_MOVABLE.

```c
	alloc_node_mem_map(pgdat);
```

在非稀疏内存模型下，该函数才有内容。

1. 分配struct page内存，赋值给pgdat->node_mem_map。
2. 在平坦内存模型下，`mem_map = NODE_DATA(0)->node_mem_map;`把numa节点0的node_mem_map赋值给mem_map。

在这之后，各种内存模型，都可以通过pfn_to_page宏把页框转换为struct page结构。

```c
	reset_deferred_meminit(pgdat);
	free_area_init_core(pgdat);
}
```

free_area_init_core()函数进一步初始化pgdata，初始化zone，伙伴系统。

## free_area_init_core()

```c
static void __paginginit free_area_init_core(struct pglist_data *pgdat)
{
	enum zone_type j;
	int nid = pgdat->node_id;
	int ret;
    
    pgdat_resize_init(pgdat);
#ifdef CONFIG_NUMA_BALANCING
	spin_lock_init(&pgdat->numabalancing_migrate_lock);
	pgdat->numabalancing_migrate_nr_pages = 0;
	pgdat->numabalancing_migrate_next_window = jiffies;
#endif
#ifdef CONFIG_TRANSPARENT_HUGEPAGE
	spin_lock_init(&pgdat->split_queue_lock);
	INIT_LIST_HEAD(&pgdat->split_queue);
	pgdat->split_queue_len = 0;
#endif
	init_waitqueue_head(&pgdat->kswapd_wait);
	init_waitqueue_head(&pgdat->pfmemalloc_wait);
#ifdef CONFIG_COMPACTION
	init_waitqueue_head(&pgdat->kcompactd_wait);
#endif
	pgdat_page_ext_init(pgdat);
	spin_lock_init(&pgdat->lru_lock);
	lruvec_init(node_lruvec(pgdat));
```

基本数据结构初始化。自旋锁，等待队列，lruvec等。

```c
	for (j = 0; j < MAX_NR_ZONES; j++) {
		struct zone *zone = pgdat->node_zones + j;
```

对节点中的每一个zone：

```c
		zone->managed_pages = is_highmem_idx(j) ? realsize : freesize;
#ifdef CONFIG_NUMA
		zone->node = nid;
#endif
		zone->name = zone_names[j];
		zone->zone_pgdat = pgdat;
		spin_lock_init(&zone->lock);
		zone_seqlock_init(zone);
		zone_pcp_init(zone);
```

初始化zone的数据。node，name，zone_pgdata.

```c
		if (!size)
			continue;
```

空zone，不再执行后续代码。为什么会有空zone？nid>=1的numa节点是没有ZONE_DMA和ZONE_DMA32的。

```c
		set_pageblock_order();
		setup_usemap(pgdat, zone, zone_start_pfn, size);
```

针对非稀疏内存模型：setup_usemap()会初始化zone的pageblock_flags。

```c
		ret = init_currently_empty_zone(zone, zone_start_pfn, size);
```

把zone中的伙伴系统初始化为空。伙伴系统中没有任何一个页。

- ```c
  static void __meminit zone_init_free_lists(struct zone *zone)
  {
  	unsigned int order, t;
  	for_each_migratetype_order(order, t) {
  		INIT_LIST_HEAD(&zone->free_area[order].free_list[t]);
  		zone->free_area[order].nr_free = 0;
  	}
  }
  ```

  伙伴系统相关链表初始化。

```c
		memmap_init(size, nid, j, zone_start_pfn);
```

初始化zone中每一个页框的struct page描述符。以及pageblock的migratetype。

- - ```
    page->flags : zone node section
    page->_refcount : 1
    page->_mapcount : -1
    page->lru : 链表初始化
    ```

```c
	}
}
```

结束。

