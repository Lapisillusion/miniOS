/*
 * init/main.c — Kernel entry point
 *
 * Called from arch/x86_64/boot/multiboot2/header.s after the 32→64 bit
 * transition.  Receives a pointer to the stored multiboot2 info pointer.
 */

#include <miniOS/types.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <miniOS/boot_info.h>

void kmain(boot_info_t *info)
{
    (void)info;  /* Not used yet — will parse mb2 info in Phase 1C */

    /* Serial is the first thing we bring up — it's the debug lifeline */
    serial_init();
    serial_puts("\n=== miniOS kernel started ===\n");

    vga_init();
    vga_set_color(vga_entry_color(VGA_LIGHT_GREEN, VGA_BLACK));

    serial_puts("miniOS booting...\n");
    vga_puts("miniOS booting...\n");

    /* Print early diagnostic info — both to VGA and serial */
    serial_puts("  arch    : x86_64\n");
    serial_puts("  boot    : multiboot2 (GRUB)\n");
    serial_puts("  display : 80x25 VGA text mode\n");

    vga_set_color(vga_entry_color(VGA_LIGHT_CYAN, VGA_BLACK));
    vga_puts("  arch    : x86_64\n");
    vga_puts("  boot    : multiboot2 (GRUB)\n");
    vga_puts("  display : 80x25 VGA text mode\n");

    vga_set_color(vga_entry_color(VGA_WHITE, VGA_BLACK));
    vga_puts("\n[ OK ] Kernel entered 64-bit long mode successfully.\n");
    serial_puts("[ OK ] Kernel entered 64-bit long mode successfully.\n");

    serial_puts("=== Kernel halted ===\n");

    /* Hang here — Phase 2+ will add the kernel start sequence */
    while (1)
        __asm__ volatile ("hlt");
}
