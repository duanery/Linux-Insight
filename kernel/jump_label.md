# jump label support

> [![40](https://github.com/duanery/picture/blob/master/github/github_black_40px.png)](https://duanery.github.io)
> &nbsp;&nbsp;
> ***duanery*** &nbsp;
> 2020年1月18日
>
> Linux爱好者

```
 * type\branch|    likely (1)         |    unlikely (0)
 * -----------+-----------------------+------------------
 *            |                       |
 *  true (1)  |       ...             |       ...
 *            |    NOP                |       JMP L
 *            |    <br-stmts>         |    1: ...
 *            |    L: ...             |
 *            |                       |
 *            |                       |    L: <br-stmts>
 *            |                       |       jmp 1b
 *            |                       |
 * -----------+-----------------------+------------------
 *            |                       |
 *  false (0) |       ...             |       ...
 *            |    JMP L              |       NOP
 *            |    <br-stmts>         |    1: ...
 *            |    L: ...             |
 *            |                       |
 *            |                       |    L: <br-stmts>
 *            |                       |       jmp 1b
 *            |                       |
 * -----------+-----------------------+------------------
```

jump label 主要目标是替换分支指令，对于那种持续为真值、持续为假值的条件语句，可以暂时替换为nop指令，当条件变为假、或者变为真时，再把nop指令替换成jmp指令，跳转到分支对应的代码处，分支执行完再跳转回去正常执行。

```c
bool _false=0;  //持续为假值,在极少情况下会被切换成1.
if(_false) {
    br_stmts;
}
normal_code...

=>

_false = 0                  | _false = 1
----------------------------|------------------------------
  NOP                       |  jmp L
N:                          |N:
  normal_code...            |  normal_code...
  ret                       |  ret
L:                          |L:
  br_stmts                  |  br_stmts
  jmp N                     |  jmp N
```

要实现这个目标，就得知道`br_stmts`到底被编译器放到什么位置。这个是核心问题。在知道了br_stmts被放的位置之后，才能决定把分支指令替换成nop指令还是jmp指令。最后把`br_stmts的位置`和`NOP/jmp指令`对应到条件语句。

## 用法

```c
//定义一个值为真、假的变量
DEFINE_STATIC_KEY_TRUE(key);   //key的默认值为1
DEFINE_STATIC_KEY_FALSE(key);  //key的默认值为0

//决定br_stmts的位置
if(static_branch_likely(&key)) br_stmts;   //br_stmts紧跟着nop/jmp指令
if(static_branch_unlikely(&key)) br_stmts; //br_stmts放到函数末尾

//key的值变换
static_key_enable(&key)   //把默认为0的key改成1，nop和jmp指令互换
static_key_disable(&key)  //把默认为1的key改成0，nop和jmp指令互换
```

### 四种情况

##### 第1种情况

```c
DEFINE_STATIC_KEY_TRUE(key);
if(static_branch_likely(&key))
    br_stmts;
normal_code...
return;
```

likely决定br_stmts紧跟着分支指令。所以，第一步可以确定br_stmts的位置。

```asm
br_stmts;
normal_code...
ret
```

第二步，key的值持久为真值TRUE，可以确定分支指令替换为nop即可，执行完nop，执行br_stmts，之后执行normal_code，指令逻辑同if条件语句。可以认为分支指令被替换成无分支的指令。

```asm
nop
br_stmts
normal_code...
ret
```

第三步，使用`static_key_disable(&key)`把key的值改为0。此时nop指令需要替换为jmp指令。`if(key) br_stmts`key的值为0，br_stmts语句不会执行，直接执行normal_code...，于是代码被替换成：

```asm
jmp L
br_stmts
L:
normal_code...
ret
```

##### 第2种情况

```c
DEFINE_STATIC_KEY_FALSE(key);
if(static_branch_likely(&key))
    br_stmts;
normal_code...
return;
```

同样的分析，初始汇编指令：

```asm
jmp L
br_stmts
L:
normal_code...
ret
```

key值被切换为1后，jmp指令被替换为nop指令

```asm
nop
br_stmts
L:
normal_code...
ret
```

##### 第3种情况

```c
DEFINE_STATIC_KEY_TRUE(key);
if(static_branch_unlikely(&key))
    br_stmts;
normal_code...
return;
```

unlikely决定br_stmts分支在函数末尾。那么初始汇编指令。

```asm
  jmp L  #key的初始值为真，需要执行br_stmts分支，这里存jmp指令
N:
  normal_code...
  ret
L:
  br_stmts    #unlikely决定br_stmts在函数末尾
  jmp N
```

把key的值切换为0后，jmp指令替换为nop指令。

```asm
  nop  #key的值为加，不需要执行br_stmts分支
N:
  normal_code...
  ret
L:
  br_stmts    #unlikely决定br_stmts在函数末尾
  jmp N
```

##### 第4种情况

```c
DEFINE_STATIC_KEY_FALSE(key);
if(static_branch_unlikely(&key))
    br_stmts;
normal_code...
return;
```

不再具体分析。

### 总结归类

1. likely和unlikely是在编译阶段被去掉的，因为其只影响br_stmts的位置，代码块的位置是编译器决定的。

2. br_stmts的位置决定着，初始的指令是nop指令还是jmp指令。

3. key值反转后，nop指令要切换成jmp，jmp要切换成nop。因此，可以得出结论：

   1. br_stmts的位置和key的值，共同决定了指令是nop还是jmp。
   2. 初始指令是nop还是jmp也由br_stmts的位置和初始的key值决定。

4. 把br_stmts的位置看做是一个二值的值，1：likely，对应的br_stmts紧跟着nop/jmp指令；0：unlikely，对应的br_stmts放在函数末尾；把这个二值的值记为***type***。结合key的值，就可以得出表

   ```c
   type        key
   1(likely)   1     nop(0)
   1(likely)   0     jmp(1)
   0(unlikely) 1     jmp(0)
   0(unlikely) 0     nop(1)
   
   type ^ key 异或操作可以决定指令到底是nop还是jmp。
   ```

5. 需要知道br_stmts的位置才能把nop切换成jmp指令，jmp指令需要指定跳转的目标地址。

## 源码解析

### 只分析一个static_branch_likely

```c
static __always_inline bool arch_static_branch(struct static_key *key, bool branch)
{
	asm_volatile_goto("1:"
		".byte " __stringify(STATIC_KEY_INIT_NOP) "\n\t"
		".pushsection __jump_table,  \"aw\" \n\t"
		_ASM_ALIGN "\n\t"
		_ASM_PTR "1b, %l[l_yes], %c0 + %c1 \n\t"
		".popsection \n\t"
		: :  "i" (key), "i" (branch) : : l_yes);

	return false;
l_yes:
	return true;
}

static __always_inline bool arch_static_branch_jump(struct static_key *key, bool branch)
{
	asm_volatile_goto("1:"
		".byte 0xe9\n\t .long %l[l_yes] - 2f\n\t"
		"2:\n\t"
		".pushsection __jump_table,  \"aw\" \n\t"
		_ASM_ALIGN "\n\t"
		_ASM_PTR "1b, %l[l_yes], %c0 + %c1 \n\t"
		".popsection \n\t"
		: :  "i" (key), "i" (branch) : : l_yes);

	return false;
l_yes:
	return true;
}

#define static_branch_likely(x)							\
({										\
	bool branch;								\
	if (__builtin_types_compatible_p(typeof(*x), struct static_key_true))	\
		branch = !arch_static_branch(&(x)->key, true);			\
	else if (__builtin_types_compatible_p(typeof(*x), struct static_key_false)) \
		branch = !arch_static_branch_jump(&(x)->key, true);		\
	else									\
		branch = ____wrong_branch_error();				\
	branch;									\
})
```

static_branch_likely()根据初始值决定调用arch_static_branch还是arch_static_branch_jump。无论选择哪个，这两个inline函数返回值都是false，取非之后，得到的branch值都是TRUE，于是if(static_branch_likely(key))等价于if(true)，于是条件分支无条件执行，br_stmts就排放在normal_code...之前。

arch_static_branch和arch_static_branch_jump函数的第二个参数，记录的是1(likely)值，表示目前分支是likely。

在arch_static_branch函数内部是nop指令，同时会记录nop指令地址和br_stmts位置的对应关系，具体的是由asm goto内联汇编指定的。这个对应关系用于把nop切换成jmp指令时，指定jmp的目标地址。

arch_static_branch_jump类似。

### key值的转换

```c
static_key_enable() ->
    static_key_slow_inc ->
    jump_label_update ->
    __jump_label_update ->
    arch_jump_label_transform(entry, jump_label_type(entry))
```

jump_label_type()执行前面提到的**异或**操作，得到的值为0，即为nop指令；得到的值为1，即为jmp指令。

由arch_jump_label_transform()把指令转换成nop或jmp指令。

问题：

1. 需要先找到nop或jmp的指令地址，才能执行指令转换，这个地址如何保存？
2. jmp指令的偏移量如何计算？

## 初始化

jump_label_init()  内核初始化调用

jump_label_init_module()  模块加载时调用

具体参考代码。

## 参考文档

### 内核文档

Documentation\static-keys.txt