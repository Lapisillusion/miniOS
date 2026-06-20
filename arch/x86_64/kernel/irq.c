/*
 * arch/x86_64/kernel/irq.c — 8259 PIC remapping + IRQ dispatch
 */

#include <miniOS/types.h>
#include <asm/io.h>
#include <lib/printf.h>

#define PIC1_CMD    0x20
#define PIC1_DATA   0x21
#define PIC2_CMD    0xA0
#define PIC2_DATA   0xA1

#define ICW1_ICW4   (1 << 0)    /* ICW4 needed */
#define ICW1_INIT   (1 << 4)    /* initialisation mode */
#define ICW4_8086   (1 << 0)    /* 8086/88 mode */

/*
 * Registered IRQ handlers.  NULL means "unused / spurious".
 */
typedef void (*irq_handler_t)(void);
static irq_handler_t g_irq_handlers[16];

/* ---- irq_register -------------------------------------------------- */

void irq_register(u8 irq, irq_handler_t handler)
{
    if (irq < 16)
        g_irq_handlers[irq] = handler;
}

/* ---- irq_handler — called from irq_common in isr_handlers.s -------- */

/*
 * Frame pushed by irq_common — same layout as isr_frame_t.
 * The 'vector' field tells us which IRQ fired.
 */
typedef struct {
    u64 rax, rbx, rcx, rdx, rsi, rdi, rbp;
    u64 r8,  r9,  r10, r11, r12, r13, r14, r15;
    u64 vector;
    u64 err_code;
    u64 rip;
    u64 cs;
    u64 rflags;
    u64 rsp;
    u64 ss;
} irq_frame_t;

void irq_handler(irq_frame_t *frame)
{
    u8 irq = (u8)(frame->vector - 32);

    /* Send EOI to the appropriate PIC */
    if (irq >= 8) {
        outb(PIC2_CMD, 0x20);          /* EOI to slave */
    }
    outb(PIC1_CMD, 0x20);              /* EOI to master */

    /* Dispatch to the registered handler */
    if (g_irq_handlers[irq] != NULL)
        g_irq_handlers[irq]();
}

/* ---- irq_init — remap 8259 PICs, mask all IRQs --------------------- */

void irq_init(void)
{
    /* Save existing masks */
    u8 mask1 = inb(PIC1_DATA);
    u8 mask2 = inb(PIC2_DATA);

    /* ICW1 — start initialisation (edge-triggered, cascaded) */
    outb(PIC1_CMD, ICW1_INIT | ICW1_ICW4);
    outb(PIC2_CMD, ICW1_INIT | ICW1_ICW4);

    /* ICW2 — base vectors */
    outb(PIC1_DATA, 0x20);             /* master: IRQ 0-7  → vectors 0x20-0x27 */
    outb(PIC2_DATA, 0x28);             /* slave:  IRQ 8-15 → vectors 0x28-0x2F */

    /* ICW3 — cascade wiring */
    outb(PIC1_DATA, 0x04);             /* master IRQ2 connected to slave */
    outb(PIC2_DATA, 0x02);             /* slave  connected to master IRQ2 */

    /* ICW4 — 8086 mode */
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);

    /* Restore masks — everything masked except cascade line */
    outb(PIC1_DATA, mask1 & ~(1 << 2));    /* unmask IRQ2 for cascading */
    outb(PIC2_DATA, mask2);

    /* Zero the handler table */
    for (int i = 0; i < 16; i++)
        g_irq_handlers[i] = NULL;

    kprintf("  pic     : remapped (master 0x20, slave 0x28)\n");
}

/* ---- irq_unmask / irq_mask — enable / disable a single IRQ line ---- */

void irq_unmask(u8 irq)
{
    u16 port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    u8  mask = inb(port) & ~(u8)(1 << (irq & 7));
    outb(port, mask);
}

void irq_mask(u8 irq)
{
    u16 port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    u8  mask = inb(port) | (u8)(1 << (irq & 7));
    outb(port, mask);
}
