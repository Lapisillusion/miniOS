# arch/x86_64/boot/multiboot2/header.s
#
# Multiboot2 header + 32-bit entry point + 64-bit long mode transition.
# This is loaded by GRUB2 and handles the full 32-bit → 64-bit switch.

.set MB2_MAGIC,     0xE85250D6
.set MB2_ARCH,      0               # i386 (32-bit protected mode)
.set MB2_HEADER_LEN,(header_end - header_start)
.set MB2_CHECKSUM,  -(MB2_MAGIC + MB2_ARCH + MB2_HEADER_LEN)

.set SEG_CODE64,    0x08
.set SEG_DATA,      0x10

# ---------------------------------------------------------------------------
# Multiboot2 header — must be in the first 32768 bytes of the kernel image
# ---------------------------------------------------------------------------
.section .multiboot, "a"
.align 8
header_start:
    .long MB2_MAGIC
    .long MB2_ARCH
    .long MB2_HEADER_LEN
    .long MB2_CHECKSUM

    # Framebuffer tag: request a linear framebuffer
    .align 8
    .word 5                     # type = 5 (framebuffer)
    .word 0                     # flags
    .long 20                    # size
    .long 0                     # width  (0 = don't care)
    .long 0                     # height (0 = don't care)
    .long 32                    # bpp    (32-bit preferred)

    # End tag
    .align 8
    .word 0                     # type = 0 (end)
    .word 0                     # flags
    .long 8                     # size
header_end:

# ---------------------------------------------------------------------------
# 32-bit entry point — GRUB drops us here in 32-bit protected mode
# ---------------------------------------------------------------------------
.section .text32, "ax"
.code32
.globl _start
_start:
    cli
    cld

    # Set up a temporary 32-bit stack
    leal stack_top, %esp

    # Validate multiboot2 magic in eax
    cmpl $0x36D76289, %eax
    jne  1f

    # Save the multiboot2 info pointer (ebx)
    movl %ebx, mb2_info_ptr

    # --------------------------------------------------
    # Build identity-mapped page tables
    #
    # Map first 1 GB using 2 MiB huge pages:
    #   PML4[0] → PDPT
    #   PDPT[0] → PD
    #   PD[0..511]: each maps a 2 MiB region
    # --------------------------------------------------
    call setup_paging_identity

    # --------------------------------------------------
    # Load PML4 physical address into CR3
    # --------------------------------------------------
    movl $pml4, %eax
    movl %eax, %cr3

    # --------------------------------------------------
    # Enable PAE (CR4.PAE = bit 5)
    # --------------------------------------------------
    movl %cr4, %eax
    orl  $0x00000020, %eax
    movl %eax, %cr4

    # --------------------------------------------------
    # Enable Long Mode (EFER.LME = bit 8)
    # --------------------------------------------------
    movl $0xC0000080, %ecx
    rdmsr
    orl  $0x00000100, %eax
    wrmsr

    # --------------------------------------------------
    # Enable Paging (CR0.PG = bit 31)
    # At this point we enter "compatibility mode"
    # --------------------------------------------------
    movl %cr0, %eax
    orl  $0x80000000, %eax
    movl %eax, %cr0

    # --------------------------------------------------
    # Load the 64-bit GDT and far-jump into 64-bit long mode
    # --------------------------------------------------
    lgdt gdt64_ptr
    ljmp $SEG_CODE64, $_start64

1:  # Error: no multiboot2 magic
    movw $0x0F52, (0xB8000)     # red 'R' on black at top-left
    hlt
    jmp 1b

# ---------------------------------------------------------------------------
# setup_paging_identity
#
# Builds page tables that identity-map the first 512 * 2 MiB = 1 GiB of RAM.
#  - PML4, PDPT, PD are pre-allocated in .bss (4 KiB each, 4096-byte aligned)
#  - Uses PD-level 2 MiB huge pages (PS=1 in PD entries)
# ---------------------------------------------------------------------------
setup_paging_identity:
    pushl %edi
    pushl %ecx
    pushl %eax
    pushl %edx

    # --- Clear PML4 (4096 bytes) ---
    movl $pml4, %edi
    xorl %eax, %eax
    movl $1024, %ecx            # 4096 / 4 = 1024 dwords
    rep stosl

    # --- Clear PDPT ---
    movl $pdpt, %edi
    movl $1024, %ecx
    rep stosl

    # --- Clear PD ---
    movl $pd, %edi
    movl $1024, %ecx
    rep stosl

    # --- PML4[0] → PDPT (present | writable) ---
    movl $pdpt, %eax
    orl  $0x3, %eax
    movl %eax, (pml4)

    # --- PDPT[0] → PD (present | writable) ---
    movl $pd, %eax
    orl  $0x3, %eax
    movl %eax, (pdpt)

    # --- Fill PD[0..511] — each entry maps a 2 MiB region ---
    movl $pd, %edi
    movl $512, %ecx             # 512 entries
    xorl %edx, %edx             # edx = 2 MiB index
.fill_pd:
    movl %edx, %eax
    shll $21, %eax              # addr = index << 21 (2 MiB)
    orl  $0x83, %eax            # present | writable | huge (PS=1)
    movl %eax, (%edi)           # low  32 bits of entry
    movl $0, 4(%edi)            # high 32 bits = 0
    addl $8, %edi               # next entry
    incl %edx
    decl %ecx
    jnz  .fill_pd

    popl %edx
    popl %eax
    popl %ecx
    popl %edi
    ret

# ---------------------------------------------------------------------------
# 64-bit entry point
# ---------------------------------------------------------------------------
.code64
_start64:
    # Reload data segment registers
    movw $SEG_DATA, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    # Set up the 64-bit kernel stack
    movq $stack_top, %rsp
    movq $stack_top, %rbp

    # Prepare boot_info pointer (rdi = first argument to kmain)
    # We expose the full mb2 info pointer — later phases will parse it
    movq $mb2_info_ptr, %rdi

    # Call the C kernel entry
    call kmain

.hang64:
    hlt
    jmp .hang64

# ---------------------------------------------------------------------------
# 64-bit Global Descriptor Table
# ---------------------------------------------------------------------------
.align 16
gdt64:
    .quad 0x0000000000000000    # 0x00: null descriptor
    .quad 0x00AF98000000FFFF    # 0x08: 64-bit code (D=0, L=1, P=1, DPL=0)
    .quad 0x00CF92000000FFFF    # 0x10: 64-bit data (P=1, DPL=0, W=1)
gdt64_end:

.align 8
gdt64_ptr:
    .word gdt64_end - gdt64 - 1
    .quad gdt64

# ---------------------------------------------------------------------------
# Page tables — must be 4 KiB aligned and zeroed at boot
# ---------------------------------------------------------------------------
.section .bss.page_tables, "aw", @nobits
.align 4096
.globl pml4
pml4:
    .skip 4096
.globl pdpt
pdpt:
    .skip 4096
.globl pd
pd:
    .skip 4096

# ---------------------------------------------------------------------------
# Kernel stack
# ---------------------------------------------------------------------------
.section .bss.stack, "aw", @nobits
.align 16
stack_bottom:
    .skip 65536                  # 64 KiB kernel stack
stack_top:

# ---------------------------------------------------------------------------
# Multiboot2 info pointer (written by 32-bit entry code)
# ---------------------------------------------------------------------------
.section .data
.align 8
.globl mb2_info_ptr
mb2_info_ptr:
    .quad 0
