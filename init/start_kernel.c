/*
 * init/start_kernel.c — Subsystem initialisation sequence.
 *
 * Called by kmain() once the boot-method-specific init has filled out
 * the boot_info_t.  This function brings up subsystems in dependency
 * order and then (for now) halts.
 */

#include <miniOS/types.h>
#include <miniOS/boot_info.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <lib/printf.h>
#include <lib/assert.h>

static void count_memory(boot_info_t *info)
{
    if (info == NULL || info->memory_map_count == 0) {
        kprintf("  mem     : no memory map\n");
        return;
    }

    u64 total_usable = 0;
    u32 usable_count = 0;

    for (u32 i = 0; i < info->memory_map_count; i++) {
        if (info->memory_map[i].type == MEM_TYPE_USABLE) {
            total_usable += info->memory_map[i].length;
            usable_count++;
        }
    }

    u64 mb = total_usable / (1024 * 1024);
    kprintf("  mem     : %d/%d entries, %llu MiB usable\n",
            usable_count, info->memory_map_count, mb);
}

void start_kernel(boot_info_t *info)
{
    /*
     * 1. Debug lifeline first — serial can work even when VGA is dead.
     */
    serial_init();

    /*
     * 2. Visual output.
     */
    vga_init();
    vga_set_color(vga_entry_color(VGA_LIGHT_GREEN, VGA_BLACK));

    /*
     * 3. Greeting.
     */
    kprintf("\n=== miniOS kernel started ===\n");
    kprintf("miniOS booting...\n");

    /*
     * 4. Architecture & display.
     */
    kprintf("  arch    : x86_64\n");

    if (info != NULL && info->framebuffer_bpp == 16) {
        kprintf("  display : VGA text mode (%ux%u)\n",
                info->framebuffer_width, info->framebuffer_height);
    } else if (info != NULL && info->framebuffer_addr != NULL) {
        kprintf("  display : %ux%u %ubpp linear framebuffer\n",
                info->framebuffer_width, info->framebuffer_height,
                info->framebuffer_bpp);
    }

    /*
     * 5. Memory layout.
     */
    count_memory(info);

    /*
     * 6. All subsystems ready.
     */
    kprintf("[ OK ] Kernel entered 64-bit long mode successfully.\n");
    kprintf("=== Kernel halted ===\n");

    /* Spin.  Phase 3 will add the interrupt-driven idle loop. */
    while (1)
        __asm__ volatile ("hlt");
}
