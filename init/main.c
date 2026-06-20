/*
 * init/main.c — Kernel entry point
 *
 * Called from the boot-method-specific init code (mb2_init / bios_init)
 * with a fully populated boot_info_t structure.
 */

#include <miniOS/types.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <miniOS/boot_info.h>

void kmain(boot_info_t *info)
{
    /* Serial is the first thing we bring up — it's the debug lifeline */
    serial_init();
    serial_puts("\n=== miniOS kernel started ===\n");

    vga_init();
    vga_set_color(vga_entry_color(VGA_LIGHT_GREEN, VGA_BLACK));

    serial_puts("miniOS booting...\n");
    vga_puts("miniOS booting...\n");

    /* ---- Framebuffer ---- */
    serial_puts("  arch    : x86_64\n");

    vga_set_color(vga_entry_color(VGA_LIGHT_CYAN, VGA_BLACK));
    vga_puts("  arch    : x86_64\n");

    if (info != NULL && info->framebuffer_bpp == 16) {
        serial_puts("  display : VGA text mode (80x25)\n");
        vga_puts("  display : VGA text mode (80x25)\n");
    } else if (info != NULL && info->framebuffer_addr != NULL) {
        /* GRUB gave us a linear framebuffer — later we'll use it */
        serial_puts("  display : linear framebuffer\n");
        vga_puts("  display : linear framebuffer\n");
    }

    /* ---- Memory map ---- */
    if (info != NULL && info->memory_map_count > 0) {
        u32 usable = 0;
        u32 total  = info->memory_map_count;

        for (u32 i = 0; i < total; i++) {
            if (info->memory_map[i].type == MEM_TYPE_USABLE)
                usable++;
        }

        serial_puts("  mem map : ");
        vga_puts("  mem map : ");

        /* Cheesy itoa — Phase 2 brings proper printf */
        if (usable >= 10) { serial_putchar('0' + (usable / 10)); vga_putchar('0' + (usable / 10)); }
        serial_putchar('0' + (usable % 10));
        vga_putchar('0' + (usable % 10));
        serial_puts("/");
        vga_puts("/");
        if (total >= 10) { serial_putchar('0' + (total / 10)); vga_putchar('0' + (total / 10)); }
        serial_putchar('0' + (total % 10));
        vga_putchar('0' + (total % 10));
        serial_puts(" entries usable\n");
        vga_puts(" entries usable\n");
    }

    vga_set_color(vga_entry_color(VGA_WHITE, VGA_BLACK));
    vga_puts("\n[ OK ] Kernel entered 64-bit long mode successfully.\n");
    serial_puts("[ OK ] Kernel entered 64-bit long mode successfully.\n");

    serial_puts("=== Kernel halted ===\n");

    /* Hang here — Phase 2+ will add the kernel start sequence */
    while (1)
        __asm__ volatile ("hlt");
}
