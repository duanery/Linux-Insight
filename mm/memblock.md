# 内核模块分析框架

> [![40](https://github.com/duanery/picture/blob/master/github/github_black_40px.png)](https://duanery.github.io)
> &nbsp;&nbsp;
> ***duanery*** &nbsp;
> 2019年2月17日
>
> Linux爱好者

## 初始化

## 接口

### debugfs

- /sys/kernel/debug/memblock/memory
- /sys/kernel/debug/memblock/reserved
- /sys/kernel/debug/memblock/physmem

dump相应的memblock的内容。

### 内核参数

```
memblock=debug	[KNL] Enable memblock debug messages.
```

## 参考文档

### 内核文档

Documention/kernel_parameters.txt  memblock

### 外网链接

https://0xax.gitbooks.io/linux-insides/content/MM/linux-mm-1.html