# miniOS Development Guide

## Project Overview

miniOS 是一个从零开始实现的小型操作系统内核，目标是在虚拟机（QEMU）中完成自举引导，加载内核，初始化核心子系统，并最终支持进程/线程调度、文件系统和一个可交互的 Shell。

**核心特点**: 手写所有引导代码，覆盖 3 种引导方式（BIOS / Multiboot2 / UEFI），不依赖任何现成的 bootloader。目录结构模仿 Linux 内核的组织方式。

## Architecture Decisions

| 决策项 | 选择 | 原因 |
|--------|------|------|
| 目标架构 | x86_64 (AMD64) | 文档丰富，QEMU 支持完善，是现代 PC 的标准架构 |
| 编程语言 | C (GNU99) + AT&T 汇编 | C 是内核开发的事实标准；AT&T 语法与 GCC 内联汇编一致 |
| 工具链 | GCC 交叉编译器 (x86_64-elf) | 避免依赖宿主系统的编译设置 |
| 构建系统 | GNU Make | 简单直接，适合内核项目规模 |
| 文件系统 | 自定义简单 FS（类 ext2 简化版） | 比 FAT 更简洁，便于学习核心概念 |
| 可执行格式 | ELF64 | x86_64 原生格式 |

## Three Boot Methods

本项目手写全部 3 种引导方式，通过 Makefile 变量切换：

### 1. Legacy BIOS（手写完整 bootloader）
- 第一阶段: 手写 512 字节 MBR 引导扇区（实模式，BIOS 加载到 0x7C00）
- 第二阶段: 手写 Stage2 loader（实模式 → 读取磁盘扇区 → 加载内核到内存）
- 模式切换: 实模式 → 32 位保护模式（设置 GDT）→ 64 位长模式
- 优点: 理解整个启动流程的每一个细节

### 2. UEFI（手写 UEFI 应用）
- 使用 POSIX-UEFI 或纯 EFI 头编写 UEFI 应用
- 使用 GOP 获取帧缓冲
- 利用 UEFI 的 `LoadImage` / `StartImage` 或直接加载内核
- ExitBootServices → 接管控制权进入内核
- 优点: 了解现代固件接口

### 3. Multiboot2（GRUB 作为载体，我们手写协议适配层）
- 手写 Multiboot2 头部结构体
- GRUB 负责加载，但内核需要解析 Multiboot2 信息结构体获取内存映射等
- 优点: 最简单的实现路径，快速验证功能

**构建时**通过 `BOOT_METHOD=bios|uefi|multiboot2` 切换。

## Development Environment

### 必需工具
```bash
# Ubuntu/Debian
sudo apt install build-essential bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo \
                 qemu-system-x86 xorriso nasm mtools gdisk

# 交叉编译器 (tools/build-toolchain.sh)
```

### 测试运行
```bash
make BOOT_METHOD=multiboot2 run   # 最快验证路径
make BOOT_METHOD=bios run
make BOOT_METHOD=uefi run
make debug                         # GDB 调试
```

## Directory Structure

```
miniOS/
├── CLAUDE.md
├── README.md
├── Makefile                    # 顶层构建
├── Kconfig                     # (未来) 配置系统
├── linker.ld                   # 内核链接脚本
│
├── arch/                       # 架构相关代码
│   └── x86_64/
│       ├── Makefile            # 架构级构建规则
│       ├── boot/               # 架构特定引导代码
│       │   ├── bios/           # Legacy BIOS 引导
│       │   │   ├── mbr.s       #   第一阶段: 512B MBR
│       │   │   ├── stage2.s    #   第二阶段: 实模式 loader
│       │   │   ├── a20.s       #   A20 地址线
│       │   │   └── mode_switch.s # 实模式→保护→长模式
│       │   ├── uefi/           # UEFI 引导
│       │   │   ├── efi_main.c  #   UEFI 入口点
│       │   │   ├── efi.h       #   UEFI 类型/协议定义
│       │   │   └── loader.c    #   内核 ELF 加载
│       │   ├── multiboot2/     # Multiboot2 引导
│       │   │   ├── header.s    #   Multiboot2 头部
│       │   │   └── parse.c     #   mb2 信息结构解析
│       │   └── Makefile
│       ├── kernel/             # 架构特定内核代码
│       │   ├── gdt.c           #   全局描述符表
│       │   ├── idt.c           #   中断描述符表
│       │   ├── isr.c           #   异常处理入口
│       │   ├── isr_handlers.s  #   ISR 汇编存根
│       │   ├── irq.c           #   IRQ + 8259 PIC
│       │   ├── syscall.c       #   syscall/sysret 框架
│       │   ├── switch.s        #   上下文切换
│       │   └── head.s          #   早期启动汇编（长模式入口）
│       ├── mm/                 # 架构特定内存管理
│       │   ├── paging.c        #   页表操作 (4-level)
│       │   └── tlb.c           #   TLB 刷新
│       └── include/            # 架构特定头文件
│           └── asm/            #   <asm/xxx.h> → arch/x86_64/include/asm/
│               ├── gdt.h
│               ├── idt.h
│               ├── paging.h
│               └── io.h        #   inb/outb 等
│
├── init/                       # 内核初始化
│   ├── main.c                  #   kmain() — 内核入口
│   └── start_kernel.c          #   子系统初始化序列
│
├── kernel/                     # 核心内核（架构无关）
│   ├── sched/                  #   调度器
│   │   └── sched.c
│   ├── proc/                   #   进程/线程管理
│   │   ├── process.c
│   │   └── thread.c
│   └── sys/                    #   系统调用实现
│       └── syscall.c
│
├── mm/                         # 核心内存管理
│   ├── pmm.c                   #   物理内存管理器
│   ├── vmm.c                   #   虚拟内存管理器
│   └── heap.c                  #   内核堆 (kmalloc/kfree)
│
├── fs/                         # 文件系统
│   ├── vfs.c                   #   VFS 层
│   └── minifs/                 #   minifs 实现
│       ├── super.c             #     超级块
│       ├── inode.c             #     inode 操作
│       └── dir.c               #     目录操作
│
├── drivers/                    # 设备驱动
│   ├── vga/                    #   VGA 文本模式
│   │   └── vga.c
│   ├── serial/                 #   串口输出
│   │   └── serial.c
│   ├── keyboard/               #   PS/2 键盘
│   │   └── keyboard.c
│   ├── timer/                  #   PIT 定时器
│   │   └── pit.c
│   └── ata/                    #   ATA PIO 磁盘驱动
│       └── ata.c
│
├── include/                    # 内核公共头文件
│   ├── miniOS/                 #   核心类型与宏
│   │   ├── types.h             #     u8, u16, u32, u64, size_t ...
│   │   ├── kernel.h            #     KERN_xxx 宏, container_of ...
│   │   ├── boot_info.h         #     统一引导信息结构
│   │   └── compiler.h          #     __packed, __aligned, likely/unlikely
│   ├── kernel/                 #   内核子系统头文件
│   │   ├── sched.h
│   │   └── proc.h
│   ├── mm/                     #   内存管理头文件
│   │   ├── pmm.h
│   │   ├── vmm.h
│   │   └── heap.h
│   ├── fs/                     #   文件系统头文件
│   │   └── vfs.h
│   └── drivers/                #   驱动头文件
│       ├── vga.h
│       ├── serial.h
│       └── keyboard.h
│
├── lib/                        # 内核库
│   ├── string.c                #   memcpy, memset, strlen ...
│   ├── printf.c                #   格式化输出
│   └── vsprintf.c              #   格式化核心
│
├── usr/                        # 用户空间（对应 Linux usr/）
│   ├── init/                   #   init 进程
│   ├── shell/                  #   miniOS Shell
│   ├── programs/               #   用户程序 (cat, ls, echo)
│   └── libc/                   #   简易 C 库
│       ├── syscall.s           #     syscall 指令封装
│       ├── stdio.c
│       ├── stdlib.c
│       └── string.c
│
├── scripts/                    # 辅助脚本
│   ├── build-disk.sh           #   磁盘镜像构建
│   └── grub.cfg                #   Multiboot2 模式 GRUB 配置
│
└── tools/                      # 工具
    └── build-toolchain.sh      #   交叉编译器构建脚本
```

## Development Phases

### Phase 0: 开发环境搭建
- [ ] 工具链确认（x86_64-elf 交叉编译器 或 系统 GCC + freestanding）
- [ ] 目录结构搭建
- [ ] 链接脚本 linker.ld
- [ ] 最小 Multiboot2 内核（验证构建链 + QEMU 启动）
- [ ] 顶层 Makefile
- [ ] `include/miniOS/types.h` — 基础类型定义
- [ ] `include/miniOS/boot_info.h` — 统一引导信息结构体
- [ ] **里程碑: QEMU 启动，VGA 打印 "miniOS booting..."**

### Phase 1A: Legacy BIOS 引导
- [x] `arch/x86_64/boot/bios/mbr.s` — 512B MBR
- [x] `arch/x86_64/boot/bios/stage2.s` — 实模式加载器
- [x] A20 地址线开启
- [x] BIOS int 0x15 E820 内存检测
- [x] 临时 GDT 构建
- [x] 实模式 → 32 位保护模式
- [x] 4 级页表（恒等映射）→ 进入 64 位长模式
- [x] 调用 kmain，传递 boot_info_t
- [x] **里程碑: 纯手写 BIOS 引导进入 64 位 C 内核**

### Phase 1B: UEFI 引导
- [x] `arch/x86_64/boot/uefi/efi_main.c` — UEFI bootloader (自包含，仅依赖 efi.h)
- [x] `arch/x86_64/boot/uefi/entry.s` — 64 位内核入口 stub
- [x] `linker_uefi.ld` — UEFI 内核链接脚本
- [x] GOP 帧缓冲初始化
- [x] 从 ESP 加载 kernel.elf (ELF64 解析 + 段加载 + BSS 清零)
- [x] GetMemoryMap → 转换到 boot_info_t
- [x] 4 级页表（含 PML4[511] 高半区映射）→ ExitBootServices → CR3 切换 → kmain
- [x] **里程碑: UEFI 原生引导** — bootloader 完成, 内核正确加载执行.
  - **已知问题**: 内核串口无输出. 根因是 UEFI 固件退出后 UART 状态与 serial_init() 不兼容, TX 空闲位永不置位导致死循环. 这是 Phase 2 串口驱动的兼容性 bug, 非引导问题.
  - **构建方法**: 需要 `gcc-mingw-w64-x86-64` + `binutils-mingw-w64-x86-64`. MinGW GCC 直接编译 PE, 不再依赖 gnu-efi 库或 objcopy 转换.

### Phase 1C: Multiboot2 引导完善
- [x] 手写 Multiboot2 头部（magic, architecture, tags）
- [x] 解析 Multiboot2 信息结构体
- [x] 提取内存映射 / 帧缓冲 / 模块信息
- [x] 填充 boot_info_t → kmain
- [x] **里程碑: Multiboot2 引导信息完整解析**

### Phase 2: 终端与调试输出
- [x] `drivers/vga/vga.c` — VGA 文本模式 (80x25) + 硬件光标
- [x] `drivers/serial/serial.c` — 串口 COM1 (0x3F8)
- [x] `lib/vsprintf.c` — 格式化核心 (%s %d %u %x %p %lld %llu %llx)
- [x] `lib/printf.c` — kprintf / kputs（双路输出到 VGA + Serial）
- [x] `lib/assert.c` — kpanic / kassert（带源码位置）
- [x] `init/start_kernel.c` — 子系统初始化序列
- [x] **里程碑: 完整的终端输出和调试能力**

### Phase 3: 中断系统
- [x] `arch/x86_64/kernel/idt.c` — IDT 初始化 (256 门描述符, 64-bit 格式)
- [x] `arch/x86_64/kernel/isr_handlers.s` — 32 异常 + 16 IRQ 汇编存根（宏生成）
- [x] `arch/x86_64/kernel/isr.c` — 异常分发器（打印寄存器/错误码/CR2）
- [x] `arch/x86_64/kernel/irq.c` — 8259 PIC 重映射 (master 0x20, slave 0x28) + IRQ 注册框架
- [x] `drivers/timer/pit.c` — PIT 8253 定时器 100Hz + tick 计数器
- [x] `drivers/keyboard/keyboard.c` — PS/2 键盘 IRQ1, scancode set-1 → ASCII
- [x] **里程碑: 可捕获异常，定时器滴答，按键响应**

### Phase 4: 物理内存管理
- [ ] `mm/pmm.c` — 位图物理页分配器
- [ ] 从 boot_info 获取内存布局
- [ ] pmm_alloc_page / pmm_free_page
- [ ] **里程碑: 可分配和释放物理页**

### Phase 5: 虚拟内存与分页
- [ ] `arch/x86_64/mm/paging.c` — 4 级页表操作
- [ ] `mm/vmm.c` — 虚拟地址空间管理
- [ ] 恒等映射 → 高阶内核映射
- [ ] **里程碑: 分页启用，内核工作在高半区虚拟地址**

### Phase 6: 内核堆
- [ ] `mm/heap.c` — kmalloc / kfree
- [ ] **里程碑: 内核动态内存分配**

### Phase 7: 进程与线程
- [ ] `kernel/proc/process.c` — PCB
- [ ] `kernel/proc/thread.c` — TCB
- [ ] `arch/x86_64/kernel/switch.s` — 上下文切换
- [ ] `kernel/sched/sched.c` — Round-Robin 调度器
- [ ] **里程碑: 多个内核线程并发执行**

### Phase 8: 用户模式
- [ ] `arch/x86_64/kernel/gdt.c` — Ring 3 段 + TSS
- [ ] `arch/x86_64/kernel/syscall.c` — syscall/sysret
- [ ] 用户进程创建与 Ring 3 切换
- [ ] 基础系统调用: write, read, exit
- [ ] **里程碑: 用户态进程运行，可发起系统调用**

### Phase 9: 磁盘与文件系统
- [ ] `drivers/ata/ata.c` — ATA PIO 磁盘驱动
- [ ] `fs/minifs/` — 自定义文件系统
- [ ] `fs/vfs.c` — VFS 层
- [ ] 文件描述符表
- [ ] **里程碑: 可读写文件**

### Phase 10: ELF 加载器
- [ ] ELF64 解析与加载
- [ ] 用户地址空间建立
- [ ] 跳转用户入口点
- [ ] **里程碑: 从文件系统加载并运行 ELF 程序**

### Phase 11: Shell & 用户程序
- [ ] `usr/shell/` — 简易 Shell
- [ ] `usr/programs/` — cat, ls, echo
- [ ] `usr/init/` — init 进程
- [ ] `usr/libc/` — 简易 libc
- [ ] **里程碑: 交互式 Shell，可执行命令**

### Phase 12: 完善
- [ ] fork (Copy-on-Write)
- [ ] 管道
- [ ] 更多用户程序
- [ ] README 文档

## Boot Flow

### BIOS Boot Flow
```
Power-on / BIOS
  → POST
  → BIOS 加载 MBR (512B) 到 0x7C00
  → [mbr.s] 设置段, 加载 stage2
  → [stage2.s] int 0x13 读磁盘 → 加载内核
  → [stage2.s] int 0x15 E820 → 内存映射
  → A20 enable
  → 加载 32-bit GDT → CR0.PE=1 → 保护模式
  → 构建 PML4 → EFER.LME=1 → CR0.PG=1 → 长模式
  → 加载 64-bit GDT → 远跳转 → kmain(boot_info_t*)
```

### UEFI Boot Flow
```
Power-on / UEFI
  → DXE → BDS → 选中 miniOS.efi
  → [efi_main] SystemTable, BootServices
  → GOP → 帧缓冲
  → 从 ESP 加载内核 ELF
  → GetMemoryMap
  → ExitBootServices
  → 页表 + 长模式 → kmain(boot_info_t*)
```

### Multiboot2 Boot Flow
```
Power-on / BIOS / UEFI
  → GRUB 解析 multiboot2 header
  → GRUB 加载内核到内存
  → [header.s] 从 GRUB 接管控制权
  → 解析 mb2 info struct
  → 填充 boot_info_t → kmain(boot_info_t*)
```

## Common Boot Info Structure

3 种引导路径向 kmain 传递统一的结构体：

```c
/* include/miniOS/boot_info.h */
typedef struct {
    void     *framebuffer_addr;
    uint32_t  framebuffer_width;
    uint32_t  framebuffer_height;
    uint32_t  framebuffer_pitch;
    uint8_t   framebuffer_bpp;

    struct {
        uint64_t base;
        uint64_t length;
        uint32_t type;   /* 1=usable, 2=reserved, 3=ACPI reclaim, 4=ACPI NVS, 5=bad */
    } memory_map[128];
    uint32_t  memory_map_count;

    uint64_t  kernel_phys_base;
    uint64_t  kernel_size;
} boot_info_t;
```

## Coding Conventions

### C 代码风格
- **标准**: C99 (GNU99)，`-ffreestanding -nostdlib`
- **缩进**: Tab（Linux 风格），8 字符宽
- **行宽**: 80 字符
- **命名**:
  - 函数: `snake_case` (`pmm_alloc_page`, `vfs_open`)
  - 全局变量: `g_` 前缀 (`g_kernel_pml4`, `g_tick_count`)
  - 常量/宏: `UPPER_SNAKE_CASE` (`PAGE_SIZE`, `KERNEL_VMA`)
  - 结构体: `_t` 后缀 (`process_t`, `inode_t`, `page_t`)
  - typedef: 对基本类型使用 (`typedef unsigned long size_t;`)
- **include guard**: `#ifndef _MINIOS_PMM_H` / `#define _MINIOS_PMM_H` / `#endif`
- **注释**: 英文，函数用 `/**` Doxygen 风格

### 汇编代码风格
- **语法**: AT&T (GAS) 为主，`.intel_syntax noprefix` 在必要时可用
- **缩进**: 标签顶格，指令缩进 Tab
- **注释**: `#` 开头

### Kernel Build Flags
```makefile
KBUILD_CFLAGS   := -std=gnu99 -ffreestanding -O2 -Wall -Wextra \
                    -nostdinc -nostdlib -fno-stack-protector \
                    -mno-red-zone -mcmodel=kernel -mno-mmx -mno-sse \
                    -mno-sse2 -mno-80387 -mgeneral-regs-only
KBUILD_AFLAGS   := -nostdinc -nostdlib
KBUILD_LDFLAGS  := -nostdlib -T linker.ld -z max-page-size=0x1000
```

## ⚠️ HARD-EARNED LESSONS — READ BEFORE WRITING BOOT CODE

> 以下每个坑都导致过实际的调试困局。违反即挂，没有警告。

### 🐞 BUG: 硬编码扇区数导致磁盘越界读

```
现象: int 0x13 返回后 CPU 挂起，不设 CF，不打印 err。
根因: KERNEL_SECTORS 写死 128，但 disk.img 只有 ~26 扇区。
      BIOS 试图读不存在的扇区，DMA 挂起永不返回。
```

**规则**: 扇区数必须由构建系统根据 `kernel.bin` 实际大小动态计算，**禁止硬编码**。

```makefile
# Makefile — 正确做法
nasm -dKERNEL_SECTORS=$(shell echo $$(( ($$(stat -c%s kernel.bin) + 511) / 512 )))
```

**同时** `disk.img` 中的 kernel 必须补齐到扇区边界，确保读请求不超磁盘物理大小。

---

### 🐞 BUG: A20 未开启就访问 1MB 以上地址

```
现象: 内核被静默写到地址 0 而不是 1MB，覆盖 IVT+BDA，CPU 崩溃。
根因: 实模式下 A20 关闭时 bit 20 强制为 0。
      0x100000 → 绕回 0x000000。
```

**规则**: 执行顺序必须是 **先开 A20，后访问 1MB+**。`enable_a20` 必须放在 `load_kernel` 之前，无一例外。

```
✅ 正确:    开 A20 → 读磁盘到 1MB
❌ 错误:    读磁盘到 1MB → 开 A20  (内核覆盖中断向量表)
```

---

### 🐞 BUG: BIOS int 0x13 的缓冲区用高段址

```
现象: int 0x13 不返回，单步调试走不过去。
根因: DAP 中 segment=0xFFF0 或 0xFFFF 这类极值。
      SeaBIOS 的地址检查逻辑对 HMA 边界处理有缺陷。
```

**规则**: **永远不要让 BIOS 中断的缓冲区落在 0x100000 以上**。
实模式 I/O 目标地址只允许常规内存（0x00000 - 0x9FFFF）。

```
✅ 正确:    DAP segment=0x07E0, offset=0    → 物理 0x7E00 (常规内存)
❌ 错误:    DAP segment=0xFFF0, offset=0x1000 → 物理 0x100000 (HMA)
```

**正确流程**: 实模式读到低地址 → 保护模式 `rep movsd` 搬到 1MB+。

---

### 🐞 BUG: 注意互不兼容的汇编语法

```
GAS (.s 文件):
  注释 #      寄存器 %rax      源在前 mov $1, %rax    .intel_syntax 可切换

NASM (.s 文件, -f bin):
  注释 ;      寄存器 rax       目的在前 mov rax, 1    [ORG] 设定基址
```

**混用后果**: GAS 把 `;` 后面的所有内容当指令，抛出一堆 "no such instruction"。

---

### 🐞 BUG: objcopy 丢弃 BSS，必须手动清零

```
kernel.elf → objcopy -O binary → kernel.bin
BSS 不在 binary 里！全局未初始化变量是磁盘上的随机值。
```

**规则**: 任何引导路径的 64 位入口（head64.s / bios64_entry）必须在调用第一个 C 函数前用 `rep stosq` 清零 BSS。

---

### 🐞 BUG: 实模式栈与代码区重叠

```
[ORG 0x1000]        ← 代码从 0x1000 向上
mov sp, 0x1000      ← 栈从 0x1000 向下生长
```

如果代码超过栈的初始偏移，`push` 会踩到代码。给代码留够空间，或把栈设到代码区之前的安全地址。

---

### 🐞 BUG: 加载内核到低地址时注意不与页表重叠

```
当前布局 (stage2):
  0x7E00  内核暂存区  (最大 ~560KB)
  0xB000  PML4        (4KB)
  0xC000  PDPT        (4KB)
  0xD000  PD          (4KB)
```

内核长度超过 `0xB000 - 0x7E00 = 0x3200 (12.5KB)` 就会覆盖页表。未来扩容时注意检查这个边界。

---

### 🐞 知识: 保护模式/长模式下不能调 BIOS 中断

BIOS = 16 位实模式代码。`CR0.PE=1` 之后 `int 0x10`/`int 0x13`/`int 0x15` 全部失效。
保护模式下读写磁盘必须用 ATA PIO 等硬件驱动（Phase 9）。

---

### 🐞 BUG: UEFI 必须用 MinGW 直接编译 PE，不能走 ELF→objcopy

```
现象: objcopy -O pei-x86-64 产出的 PE 不被 OVMF 接受 (Unsupported)
根因: 新版 binutils (2.44+) 移除了 efi-app-x86_64 目标。
      pei-x86-64 产出的 PE 缺少 IMAGE_FILE_DLL 标志, OVMF 拒绝加载。
```

**规则**: UEFI bootloader 必须用 MinGW GCC (`x86_64-w64-mingw32-gcc`) 直接编译链接为 PE/COFF。
安装 `gcc-mingw-w64-x86-64` + `binutils-mingw-w64-x86-64`。

```makefile
# 正确做法
x86_64-w64-mingw32-gcc -shared -nostdlib -e efi_main -Wl,--subsystem,10 \
    -I/usr/include/efi -I/usr/include/efi/x86_64 -Iinclude \
    -ffreestanding -fno-stack-protector -fshort-wchar -mno-red-zone \
    efi_main.c -o BOOTX64.EFI
```

`-nostdlib` 是必须的：不加则 MinGW 自动链接 `-lmsvcrt -lkernel32`，产出的 PE 导入 Windows DLL，UEFI 环境里不存在。

---

### 🐞 BUG: `-mcmodel=kernel` 需要 PML4[511] 高半区映射

```
现象: UEFI 引导下内核页错误, CR2 = 0xFFFFFFFF80XXXXXX
根因: 内核用 -mcmodel=kernel 编译, 符号地址在高半区 (0xFFFF_FFFF_8000_0000+).
      但 UEFI 的页表只有恒等映射 (PML4[0]).
```

**规则**: UEFI bootloader 构建页表时必须同时设 PML4[0]（恒等）和 PML4[511]（高半区），指向同一个 PDPT。

---

### 🐞 BUG: UEFI ExitBootServices 的 key 失效问题

```
现象: ExitBootServices 返回 EFI_INVALID_PARAMETER 或 Buffer Too Small
根因: 在 GetMemoryMap 和 ExitBootServices 之间做了任何 AllocatePool,
      内存布局改变, map key 失效.
```

**规则**: 所有 UEFI 内存分配（包括页表）必须在最后一次 GetMemoryMap 之前完成。
执行顺序：load kernel → 分配页表 → GetMemoryMap（最后一次）→ ExitBootServices（紧接）。

```c
build_pages();        // 先分配
GetMemoryMap(..., &key);  // 再取 key
ExitBootServices(key);    // 立即用, 中间不做任何分配
```

---

### 🐞 知识: QEMU debug port 0xE9 ≠ serial port

QEMU 的 `-serial stdio` 重定向的是 COM1 (0x3F8)。`outb $0xE9` 输出到 QEMU 的 stderr（调试控制台），与串口完全独立。用 `-serial file:xxx` 抓不到 port 0xE9 的输出。调试时两者不能混淆。

---

## Key Design Principles

1. **渐进式开发**: 每完成一个 Phase 立即在 QEMU 中验证
2. **简单优先**: 先用最简实现（bitmap PMM, Round-Robin 调度），后续迭代
3. **调试先行**: 优先打通串口/ VGA 输出，kpanic 走在崩溃前面
4. **模块化**: 子系统通过 include/ 中的头文件接口通信
5. **统一引导信息**: 3 种引导最终给 kmain 同一套 boot_info_t
6. **失败即停止**: `kpanic()` + `kassert()`，不静默吞错误
7. **Linux 风格目录**: arch/ kernel/ mm/ fs/ drivers/ include/ init/ lib/ usr/
