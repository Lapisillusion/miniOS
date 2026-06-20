/*
 * lib/printf.c — Kernel printf that fans out to serial + VGA.
 */

#include <miniOS/types.h>
#include <lib/printf.h>
#include <drivers/vga.h>
#include <drivers/serial.h>

/* ---- kputs — plain string to both consoles --------------------------- */

void kputs(const char *str)
{
    serial_puts(str);
    vga_puts(str);
}

/* ---- kvprintf — va_list → static buffer → both consoles -------------- */

void kvprintf(const char *fmt, __builtin_va_list args)
{
    static char buf[1024];
    unsigned long flags;

    /* Protect the static buffer against concurrent ISR access */
    __asm__ volatile (
        "pushfq\n\t"
        "popq %0\n\t"
        "cli"
        : "=m"(flags) :: "memory"
    );
    vsnprintf(buf, sizeof(buf), fmt, args);
    serial_puts(buf);
    vga_puts(buf);
    __asm__ volatile (
        "pushq %0\n\t"
        "popfq"
        :: "m"(flags) : "memory"
    );
}

/* ---- kprintf — the one you actually call ------------------------------ */

void kprintf(const char *fmt, ...)
{
    __builtin_va_list args;
    __builtin_va_start(args, fmt);
    kvprintf(fmt, args);
    __builtin_va_end(args);
}
