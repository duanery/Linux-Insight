# 内核模块分析框架

> [![40](https://github.com/duanery/picture/blob/master/github/github_black_40px.png)](https://duanery.github.io)
> &nbsp;&nbsp;
> ***duanery*** &nbsp;
> 2019年1月23日
> 
> Linux爱好者

*核心原理简介*

## 1 使用方法

*用户态的用法*

*内核态的用法*

## 2 编译
*讲述模块是如何编译的(如果有的话)*

## 3 初始化
*模块的初始化入口，以及初始化阶段做了什么*

## 4 源码简析
*简要介绍核心源码，或者详解核心源码*

### 4.1 数据结构

*介绍核心数据结构*

### 4.2 内核线程

*介绍内核线程的作用与交互（如果存在内核线程的话）*

## 5 接口
*proc文件系统接口，sys文件系统接口，系统调用接口，debugfs文件系统接口，
tracepoint跟踪接口，导出符号，内核如何使用接口*

### 5.1 proc
*/proc/kallsyms*

### 5.2 sys
*/sys/kernel/...*

### 5.3 syscall
*read(...)*

### 5.4 debugfs
*/sys/kernel/debug/*

### 5.5 EXPORT_SYMBOL
*导出的内核符号*

### 5.6 tracepoint
*/sys/kernel/debug/tracing/*

### 5.7 config
*.config 配置*

### 5.8 其他模块如何使用本接口
*导出的符号*

### 5.9 boot参数
*Documention/kernel_parameters.txt*

## 6 统计信息
*主要是proc文件系统内导出的内核参数*

## 7 性能调节
*调节性能，模块参数调节，/proc参数调节，/sys参数调节*

## 8 参考文档

### 8.1 内核文档
*Documentation/*

### 8.2 man手册

### 8.3 外网链接
*\[1\] [linux-insides][1]*

*\[2\] [kernel_exploring][2]*

[1]: https://0xax.gitbooks.io/linux-insides/ "linux-insides"
[2]: https://richardweiyang.gitbooks.io/kernel-exploring/	"kernel_exploring"

# 排版约束

1. 文章中提到的源码变量，统一使用*斜体*。全局变量**加粗**显示，局部变量不加粗。不重要的全局变量也可以不加粗。首次引用使用斜体，非首次引用可以不加粗不使用斜体。
2. 文章中提到的函数名，统一在函数名后加'()'来标识。括号内不加函数的参数。
3. 文章中提到的超过1行的源码，使用代码块引用。不超过一行的代码，使用行内引用。
