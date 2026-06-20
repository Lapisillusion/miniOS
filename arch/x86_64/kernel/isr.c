/*
 * arch/x86_64/kernel/isr.c — Exception dispatcher
 *
 * Called from isr_common in isr_handlers.s with a pointer to the saved
 * register frame on the stack.
 */

#include <miniOS/types.h>
#include <miniOS/compiler.h>
#include <lib/printf.h>
#include <lib/assert.h>
#include <drivers/vga.h>

/*
 * Register snapshot pushed by isr_common.
 * Fields are listed in push order (last pushed = lowest offset).
 */
typedef struct {
    u64 rax, rbx, rcx, rdx, rsi, rdi, rbp;
    u64 r8,  r9,  r10, r11, r12, r13, r14, r15;
    u64 vector;    /* pushed by the stub */
    u64 err_code;  /* pushed by the stub (or CPU for some exceptions) */
    u64 rip;
    u64 cs;
    u64 rflags;
    u64 rsp;
    u64 ss;
} __packed isr_frame_t;

/* Human-readable exception names */
static const char *exception_names[32] = {
    [ 0] = "Divide Error",
    [ 1] = "Debug",
    [ 2] = "NMI",
    [ 3] = "Breakpoint",
    [ 4] = "Overflow",
    [ 5] = "Bound Range Exceeded",
    [ 6] = "Invalid Opcode",
    [ 7] = "Device Not Available",
    [ 8] = "Double Fault",
    [ 9] = "Coprocessor Segment Overrun",
    [10] = "Invalid TSS",
    [11] = "Segment Not Present",
    [12] = "Stack-Segment Fault",
    [13] = "General Protection Fault",
    [14] = "Page Fault",
    [15] = "Reserved",
    [16] = "x87 FPU Error",
    [17] = "Alignment Check",
    [18] = "Machine Check",
    [19] = "SIMD Floating-Point",
    [20] = "Virtualisation",
    [21] = "Control Protection",
    /* 22-27 reserved */
    [28] = "Hypervisor Injection",
    [29] = "VMM Communication",
    [30] = "Security Exception",
    [31] = "Reserved",
};

void isr_handler(isr_frame_t *frame)
{
    u64 vec = frame->vector;

    /* Page Fault — print CR2 (the faulting address) */
    if (vec == 14) {
        u64 cr2;
        __asm__ volatile ("mov %%cr2, %0" : "=r"(cr2));
        vga_set_color(vga_entry_color(VGA_WHITE, VGA_RED));
        kprintf("\n*** EXCEPTION: %s (vec=%llu)\n",
                exception_names[vec], vec);
        kprintf("    addr: 0x%016llx  err: 0x%llx\n",
                cr2, frame->err_code);
        kprintf("    rip : 0x%016llx  cs : 0x%llx\n",
                frame->rip, frame->cs);
        kprintf("    rsp : 0x%016llx  ss : 0x%llx\n",
                frame->rsp, frame->ss);
    } else {
        vga_set_color(vga_entry_color(VGA_WHITE, VGA_RED));
        kprintf("\n*** EXCEPTION: %s (vec=%llu)\n",
                exception_names[vec], vec);
        kprintf("    rip : 0x%016llx  err: 0x%llx\n",
                frame->rip, frame->err_code);
    }

    /* Fatal — do not return */
    kpanic("Unhandled exception");
}
