/*
 * lib/assert.c — Kernel panic and assertion support.
 */

#include <miniOS/types.h>
#include <lib/printf.h>
#include <drivers/vga.h>

/* ---- kpanic — unrecoverable, print message and halt forever ---------- */

void kpanic(const char *msg)
{
    vga_set_color(vga_entry_color(VGA_WHITE, VGA_RED));

    kprintf("\n*** KERNEL PANIC ***\n");
    kprintf("%s\n", msg);
    kprintf("*** SYSTEM HALTED ***\n");

    /* Spin forever */
    while (1)
        __asm__ volatile ("cli; hlt");
}

/* ---- kassert_fail — called by the kassert() macro -------------------- */

void kassert_fail(const char *file, int line, const char *cond)
{
    kprintf("ASSERTION FAILED: %s\n", cond);
    kprintf("  at %s:%d\n", file, line);
    kpanic("kassert");
}
