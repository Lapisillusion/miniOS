# 第一处问题：BOOTX64.EFI 很可能根本没有成功生成

你的 Makefile：

```make
UEFI_CC := x86_64-w64-mingw32-gcc
```

然后：

```make
BOOTX64.EFI:
    $(UEFI_CC) -shared -e efi_main \
        -Wl,--subsystem,10 \
        ...
```

但你的代码引用：

```c
#include <efi.h>
#include <efiprot.h>
```

这是 GNU-EFI 头文件。

然而你的链接方式却是：

```text
MinGW PE Loader
```

而不是：

```text
GNU-EFI
```

标准 GNU-EFI 构建通常需要：

```bash
crt0-efi-x86_64.o
-lgnuefi
-lefi
```

例如：

```bash
ld \
  crt0-efi-x86_64.o \
  efi_main.o \
  -lefi -lgnuefi
```

否则很多固件上：

```text
BOOTX64.EFI
加载后立即返回
```

或者：

```text
Unsupported
```

---

# 第二处问题（最可疑）

## 你加载 ELF 时使用的是：

```c
EFI_PHYSICAL_ADDRESS addr = ph.pa;
```

然后：

```c
AllocatePages(
    AllocateAddress,
    ...
    &addr
);
```

---

但你的 Kernel Linker：

```ld
. = 1M;
```

只有：

```ld
VMA
```

并没有指定：

```ld
AT(...)
PHDRS
```

---

因此 ELF Program Header 里的：

```text
p_paddr
```

极有可能是：

```text
0
```

或者：

```text
未定义
```

而不是：

```text
0x100000
```

---

这会导致：

```c
AllocatePages(..., &addr);
```

实际上尝试分配：

```text
0x00000000
```

附近的页面。

OVMF 通常直接拒绝。

于是：

```text
ERR: alloc pages
```

或者：

```text
直接退出
```

---

## 请立即打印

在：

```c
for(E64W i=0;i<eh.pn;i++)
```

里面加：

```c
CHAR16 buf[128];

SPrint(
    buf,
    sizeof(buf),
    L"PH %d pa=%lx va=%lx\r\n",
    i,
    ph.pa,
    ph.va
);

Print(buf);
```

看看：

```text
ph.pa
```

到底是多少。

---

# 第三处问题：Kernel Entry 与 Loader Entry 不统一

你的 Loader：

```c
void (*ke)(boot_info_t*) =
    (void(*)(boot_info_t*))(UINTN)eh.e;
```

然后：

```c
jmp *ke
```

---

而 Kernel：

```ld
ENTRY(_start)
```

---

即：

```text
eh.e
=
_start
```

---

而 `_start` 在：

```asm
entry.s
```

里面。

---

这里有一个隐藏风险：

```asm
lea rsp, [rip + stack_top]
```

重新切换栈。

没有问题。

但是：

```asm
call kmain
```

依赖：

```text
RDI
```

还保持着 boot_info 指针。

---

如果：

```asm
mov ax,0x10
mov ds,ax
...
```

过程中某个编译器或汇编器生成额外代码，

RDI 被破坏，

Kernel 就会：

```text
boot_info = NULL
```

直接崩。

---

建议改成：

```asm
mov rbx, rdi

...

mov rdi, rbx
call kmain
```

---

# 第四处问题（非常严重）

你创建了新的页表：

```c
mov %0, %%cr3
```

切换到：

```text
你自己构造的 CR3
```

---

但你只映射了：

```text
0~4GB
```

---

然而：

```text
BOOTX64.EFI
boot_info
memory map
page tables
```

这些对象都是：

```c
AllocatePool()
```

得到的。

---

在 OVMF 中：

它们经常位于：

```text
0x00000007xxxxxxx
```

甚至：

```text
0x00000008xxxxxxx
```

以上。

---

切换 CR3 后：

```text
RDI=&bi
```

可能已经指向未映射地址。

Kernel 第一次访问：

```c
info->...
```

立刻：

```text
Page Fault
Triple Fault
```

---

这是我目前认为最可能的问题。

---

# 如何验证

在：

```c
__asm__ volatile("mov %0,%%rdi\n\tjmp *%1"
```

之前打印：

```c
bi addr
pml4 addr
mp addr
```

例如：

```c
SPrint(buf,sizeof(buf),
    L"bi=%lx pml4=%lx\r\n",
    &bi,
    pml4);
Print(buf);
```

---

如果看到：

```text
bi = 00000007Fxxxxx
```

那么答案就找到了。

因为：

```text
你的页表只映射到 4GB
```

而：

```text
7Fxxxxxh
```

已经超出映射范围。

---

# 我最怀疑的两个点

按概率排序：

### 1

切换 CR3 后：

```text
boot_info
page tables
memory map
```

地址丢失

概率：

```text
≈ 60%
```

---

### 2

ELF 的：

```text
p_paddr
```

不正确

导致：

```text
AllocatePages失败
```

概率：

```text
≈ 30%
```
