/*
 * drivers/timer/pit.c — 8253/8254 Programmable Interval Timer
 */

#include <drivers/pit.h>
#include <asm/io.h>
#include <lib/printf.h>

#define PIT_CH0     0x40
#define PIT_CMD     0x43

#define BASE_CLOCK  1193180UL

volatile u64 g_tick = 0;

/* ---- Forward declaration from irq.c --------------------------------- */
extern void irq_register(u8 irq, void (*handler)(void));
extern void irq_unmask(u8 irq);

/* ---- tick_handler — called on every PIT fire (IRQ0) ---------------- */
static void tick_handler(void)
{
    g_tick++;
}

/* ---- pit_init ------------------------------------------------------- */

void pit_init(u32 hz)
{
    if (hz == 0)
        return;

    u32 divisor = BASE_CLOCK / hz;

    /* Cap to the hardware's 16-bit counter */
    if (divisor > 65535)
        divisor = 65535;
    if (divisor < 1)
        divisor = 1;

    /*
     * 0x36 = channel 0 | lobyte/hibyte | rate generator | binary
     */
    outb(PIT_CMD, 0x36);
    outb(PIT_CH0, (u8)(divisor & 0xFF));          /* low byte  */
    outb(PIT_CH0, (u8)((divisor >> 8) & 0xFF));    /* high byte */

    irq_register(0, tick_handler);
    irq_unmask(0);

    kprintf("  pit     : %u Hz (divisor %u)\n", BASE_CLOCK / divisor, divisor);
}
