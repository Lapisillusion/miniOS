# arch/x86_64/boot/uefi/entry.s
        .section .text.head, "ax"
        .code64
        .intel_syntax noprefix
        .globl _start
_start:
        mov     ax, 0x10
        mov     ds, ax
        mov     es, ax
        mov     fs, ax
        mov     gs, ax
        mov     ss, ax
        lea     rsp, [rip + stack_top]
        # Write directly to VGA buffer as a canary
        mov     dword ptr [0xB8000], 0x0F4B0F4B   # white 'K' 'K' at top-left
        call    kmain
.hang:  hlt
        jmp     .hang

        .section .bss.stack, "aw", @nobits
        .align 16
stack_bottom: .skip 65536
stack_top:
