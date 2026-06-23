# arch/x86_64/boot/uefi/entry.s
        .section .text.head, "ax"
        .code64
        .intel_syntax noprefix
        .globl _start
_start:
        # Save boot_info pointer before touching RDI
        mov     r12, rdi

        mov     ax, 0x10
        mov     ds, ax
        mov     es, ax
        mov     fs, ax
        mov     gs, ax
        mov     ss, ax
        lea     rsp, [rip + stack_top]
        lea     rbp, [rip + stack_top]

        # Zero the BSS — UEFI firmware does not guarantee zeroed memory
        lea     rdi, [rip + _bss_start]
        lea     rcx, [rip + _bss_end]
        sub     rcx, rdi
        shr     rcx, 3                  # qword count
        xor     eax, eax
        cld
        rep stosq

        # Signal kernel entry: write magic number to physical 0x500
        mov     dword ptr [0x500], 0xDEADBEEF

        # VGA canary: white 'K' 'K' at top-left
        mov     dword ptr [0xB8000], 0x0F4B0F4B

        # Restore boot_info pointer and call kmain
        mov     rdi, r12
        call    kmain
.hang:  hlt
        jmp     .hang

        .section .bss.stack, "aw", @nobits
        .align 16
stack_bottom: .skip 65536
stack_top:
