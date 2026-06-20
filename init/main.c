/*
 * init/main.c — Kernel entry point
 *
 * Called from the boot-method-specific init code (mb2_init / bios_init)
 * with a fully populated boot_info_t.  Hands off to start_kernel() which
 * brings up all subsystems in order.
 */

#include <miniOS/boot_info.h>

/* Defined in init/start_kernel.c */
void start_kernel(boot_info_t *info);

void kmain(boot_info_t *info)
{
    start_kernel(info);

    /* start_kernel() never returns */
    while (1)
        __asm__ volatile ("hlt");
}
