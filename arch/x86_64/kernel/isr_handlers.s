# arch/x86_64/kernel/isr_handlers.s
#
# Exception ISR stubs for vectors 0 – 31.
# Each stub pushes a dummy error code (when the CPU does not), then the
# vector number, and jumps into the common save / restore trampoline.
#
# IRQ stubs for vectors 32 – 47 are also defined here so that the IDT
# can be populated in one pass.

        .intel_syntax noprefix
        .code64

# ------------------------------------------------------------------------
# Exception names (OSDev canonical)
# ------------------------------------------------------------------------
        .extern isr_handler
        .extern irq_handler

# ------------------------------------------------------------------------
# isr_common — save GPRs → call isr_handler → restore → iretq
# ------------------------------------------------------------------------
isr_common:
        push    r15
        push    r14
        push    r13
        push    r12
        push    r11
        push    r10
        push    r9
        push    r8
        push    rbp
        push    rdi
        push    rsi
        push    rdx
        push    rcx
        push    rbx
        push    rax

        mov     rdi, rsp                # rdi = pointer to the saved frame
        call    isr_handler

        pop     rax
        pop     rbx
        pop     rcx
        pop     rdx
        pop     rsi
        pop     rdi
        pop     rbp
        pop     r8
        pop     r9
        pop     r10
        pop     r11
        pop     r12
        pop     r13
        pop     r14
        pop     r15

        add     rsp, 16                 # drop vector + error code
        iretq

# ------------------------------------------------------------------------
# irq_common — same as isr_common, but calls irq_handler + sends EOI
# ------------------------------------------------------------------------
irq_common:
        push    r15
        push    r14
        push    r13
        push    r12
        push    r11
        push    r10
        push    r9
        push    r8
        push    rbp
        push    rdi
        push    rsi
        push    rdx
        push    rcx
        push    rbx
        push    rax

        mov     rdi, rsp
        call    irq_handler

        pop     rax
        pop     rbx
        pop     rcx
        pop     rdx
        pop     rsi
        pop     rdi
        pop     rbp
        pop     r8
        pop     r9
        pop     r10
        pop     r11
        pop     r12
        pop     r13
        pop     r14
        pop     r15

        add     rsp, 16
        iretq

# ========================================================================
# Exception stubs  (vectors 0 – 31)
# ========================================================================

.macro isr_no_err vector
        .global isr\vector
isr\vector:
        push    0                       # dummy error code
        push    \vector                 # vector number
        jmp     isr_common
.endm

.macro isr_err vector
        .global isr\vector
isr\vector:
        push    \vector                 # CPU already pushed error code
        jmp     isr_common
.endm

# 0  — Divide Error
isr_no_err 0
# 1  — Debug
isr_no_err 1
# 2  — NMI
isr_no_err 2
# 3  — Breakpoint
isr_no_err 3
# 4  — Overflow
isr_no_err 4
# 5  — Bound Range Exceeded
isr_no_err 5
# 6  — Invalid Opcode
isr_no_err 6
# 7  — Device Not Available
isr_no_err 7
# 8  — Double Fault              (error code)
isr_err 8
# 9  — Coprocessor Segment Overrun
isr_no_err 9
# 10 — Invalid TSS               (error code)
isr_err 10
# 11 — Segment Not Present       (error code)
isr_err 11
# 12 — Stack-Segment Fault       (error code)
isr_err 12
# 13 — General Protection Fault  (error code)
isr_err 13
# 14 — Page Fault                (error code)
isr_err 14
# 15 — Reserved
isr_no_err 15
# 16 — x87 FPU Error
isr_no_err 16
# 17 — Alignment Check           (error code)
isr_err 17
# 18 — Machine Check
isr_no_err 18
# 19 — SIMD Floating-Point
isr_no_err 19
# 20 — Virtualisation
isr_no_err 20
# 21 — Control Protection        (error code)
isr_err 21
# 22-27 — Reserved
isr_no_err 22
isr_no_err 23
isr_no_err 24
isr_no_err 25
isr_no_err 26
isr_no_err 27
# 28 — Hypervisor Injection
isr_no_err 28
# 29 — VMM Communication         (error code)
isr_err 29
# 30 — Security Exception        (error code)
isr_err 30
# 31 — Reserved
isr_no_err 31

# ========================================================================
# IRQ stubs  (vectors 32 – 47)
# ========================================================================

.macro irq_stub vector irq
        .global irq\irq
irq\irq:
        push    0
        push    \vector
        jmp     irq_common
.endm

irq_stub 32  0
irq_stub 33  1
irq_stub 34  2
irq_stub 35  3
irq_stub 36  4
irq_stub 37  5
irq_stub 38  6
irq_stub 39  7
irq_stub 40  8
irq_stub 41  9
irq_stub 42 10
irq_stub 43 11
irq_stub 44 12
irq_stub 45 13
irq_stub 46 14
irq_stub 47 15
