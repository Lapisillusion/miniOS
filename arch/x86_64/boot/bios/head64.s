# arch/x86_64/boot/bios/head64.s
#
# 64-bit kernel entry for the BIOS boot path.
# Linked at KERNEL_PHYS (0x100000) and placed first in the flat binary.
# Stage2 jumps directly to this address.
#
# rdi = E820 buffer pointer  (from stage2)
# rsi = E820 entry count      (from stage2)

        .section .text.head, "ax"
        .code64
        .intel_syntax noprefix
        .globl bios64_entry

bios64_entry:
        # Preserve stage2 arguments in callee-saved registers
        mov     r12, rdi                # E820 buffer
        mov     r13, rsi                # E820 count

        # Zero the BSS
        lea     rdi, [rip + _bss_start]
        lea     rcx, [rip + _bss_end]
        sub     rcx, rdi
        shr     rcx, 3                  # qword count
        xor     eax, eax
        cld
        rep stosq

        # Set up the kernel stack
        lea     rsp, [rip + stack_top]
        lea     rbp, [rip + stack_top]

        # Call C entry: bios_init(e820_buf, e820_count)
        mov     rdi, r12
        mov     rsi, r13
        call    bios_init

        # bios_init calls kmain() and should never return
.hang:
        hlt
        jmp     .hang

# ---------------------------------------------------------------------------
# Kernel stack
# ---------------------------------------------------------------------------
        .section .bss.stack, "aw", @nobits
        .align 16
stack_bottom:
        .skip   65536                    # 64 KiB
stack_top:
