# 内核模块分析框架
[![48](https://github.com/duanery/picture/blob/master/github/github_black_48px.png)](https://duanery.github.io)

作者简介

核心原理简介

## 编译
讲述模块是如何编译的(如果有的话)

## 初始化
模块的初始化入口，以及初始化阶段做了什么

## 源码简析
简要介绍核心源码，或者详解核心源码

## 接口
proc文件系统接口，sys文件系统接口，系统调用接口，debugfs文件系统接口，
tracepoint跟踪接口，导出符号，内核如何使用接口

### proc
> /proc/kallsyms

### sys
> /sys/kernel/...

### syscall
> read(...)

### debugfs

### EXPORT_SYMBOL

### 其他模块如何使用本接口

## 统计信息
主要是proc文件系统内导出的内核参数

## 参考文档

### 内核文档
Documentation/

### man手册

### 外网链接
