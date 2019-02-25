# 资源模块

> [![40](https://github.com/duanery/picture/blob/master/github/github_black_40px.png)](https://duanery.github.io)
> &nbsp;&nbsp;
> ***duanery*** &nbsp;
> 2019年2月25日
>
> Linux爱好者

资源模块，初始定义了2个资源：ioport（0-0xffff），iomem（0-最大值）。内核的初始化会不断的插入新资源。一个资源以（start,end）这样的范围来表示，插入的小范围的资源会作为其他大范围资源的子节点，形成树形结构。

## 资源API

```c
//请求资源
//把new指向的资源加入root资源的第一层子节点，不会加入更深层的子节点。new执行的资源必须是已分配内存的。
int request_resource(struct resource *root, struct resource *new);
EXPORT_SYMBOL(request_resource);
```

```c
//释放资源
//把old资源及old资源的子节点资源全部释放，释放仅仅是把资源从树中移除，不会释放old及其子资源的结构体。
int release_resource(struct resource *old);
EXPORT_SYMBOL(release_resource);
```

```c
//判断pfn指定的页框是否是物理内存。
//内部，会在iomem资源树中搜索带IORESOURCE_SYSTEM_RAM标记的资源。
int __weak page_is_ram(unsigned long pfn);
EXPORT_SYMBOL_GPL(page_is_ram);
```

```c
/**
 * region_intersects() - determine intersection of region with known resources
 * @start: region start address
 * @size: size of region
 * @flags: flags of resource (in iomem_resource)
 * @desc: descriptor of resource (in iomem_resource) or IORES_DESC_NONE
 *
 * Check if the specified region partially overlaps or fully eclipses a
 * resource identified by @flags and @desc (optional with IORES_DESC_NONE).
 * Return REGION_DISJOINT if the region does not overlap @flags/@desc,
 * return REGION_MIXED if the region overlaps @flags/@desc and another
 * resource, and return REGION_INTERSECTS if the region overlaps @flags/@desc
 * and no other defined resource. Note that REGION_INTERSECTS is also
 * returned in the case when the specified region overlaps RAM and undefined
 * memory holes.
 *
 * region_intersect() is used by memory remapping functions to ensure
 * the user is not remapping RAM and is a vast speed up over walking
 * through the resource table page by page.
 */
int region_intersects(resource_size_t start, size_t size, unsigned long flags,
		      unsigned long desc);
EXPORT_SYMBOL_GPL(region_intersects);
```

```c
// allocate_resource - allocate empty slot in the resource tree given range & alignment. The resource will be reallocated with a new size if it was already allocated
int allocate_resource(struct resource *root, struct resource *new,
		      resource_size_t size, resource_size_t min,
		      resource_size_t max, resource_size_t align,
		      resource_size_t (*alignf)(void *,
						const struct resource *,
						resource_size_t,
						resource_size_t),
		      void *alignf_data);
EXPORT_SYMBOL(allocate_resource);
```

```c
//插入资源
//把new指向的资源插入parent之中，会遍历parent指向资源树的每个层次。如果new资源指向一个比较大的范围，会把该范围内的资源作为new的子节点。如果new资源指向一个比较小的范围，new会作为其他资源的子节点而插入。
int insert_resource(struct resource *parent, struct resource *new);
EXPORT_SYMBOL_GPL(insert_resource);
```

```c
//移除资源，作为insert_resource操作的逆操作
//只把old自身移除，如果old下有子资源，则子资源会提升一层，作为old父节点的子节点，子资源不会被移除。
int remove_resource(struct resource *old);
EXPORT_SYMBOL_GPL(remove_resource);
```

```c
//调整资源
//把res资源的范围调整为（start,start+size）
int adjust_resource(struct resource *res, resource_size_t start,
			resource_size_t size);
EXPORT_SYMBOL(adjust_resource);
```

### 其他API

TODO

## iomem资源初始化

内核初始化时，调用setup_arch函数，进而会把e820信息插入iomem资源树中。

```c
void __init setup_arch(char **cmdline_p)
{
    ...
    insert_resource(&iomem_resource, &code_resource);
	insert_resource(&iomem_resource, &data_resource);
	insert_resource(&iomem_resource, &bss_resource);
    ...
    e820_reserve_resources();  //把e820信息插入iomem资源树中
    ...
}
```

还有其他资源初始化。

## ioport资源初始化

```c
void __init setup_arch(char **cmdline_p)
{
    ...
	x86_init.resources.reserve_resources(); //指向reserve_standard_io_resources
    ...
}
void __init reserve_standard_io_resources(void)
{
	int i;

	/* request I/O space for devices used on all i[345]86 PCs */
	for (i = 0; i < ARRAY_SIZE(standard_io_resources); i++)
		request_resource(&ioport_resource, &standard_io_resources[i]);
}
```

会把x86架构定义的一些标准的io资源插入ioport资源树中。

## 接口

### proc

```bash
[root@localhost ~]# cat /proc/iomem 
00000000-00000fff : reserved
00001000-0009fbff : System RAM
...

[root@localhost ~]# cat /proc/ioports 
0000-0cf7 : PCI Bus 0000:00
  0000-001f : dma1
  0020-0021 : pic1
  0040-0043 : timer0
  0050-0053 : timer1
  ...
```

### boot参数

```
reserve=	[KNL,BUGS] Force the kernel to ignore some iomem area

iomem=		Disable strict checking of access to MMIO memory
		strict	regions from userspace.
		relaxed
```

## 参考文档

### 内核文档

Documention/kernel_parameters.txt