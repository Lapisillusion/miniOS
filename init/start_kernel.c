/*
 * init/start_kernel.c — Subsystem initialisation sequence.
 *
 * Called by kmain() with a populated boot_info_t.
 * Brings up every subsystem in dependency order.
 */

#include <miniOS/types.h>
#include <miniOS/boot_info.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <drivers/pit.h>
#include <drivers/keyboard.h>
#include <lib/printf.h>
#include <lib/assert.h>

/* Declared in arch/x86_64/kernel/ */
extern void idt_init(void);
extern void irq_init(void);

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
     * 1. Debug lifeline — must come before anything that can crash.
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
     * 6. Interrupt subsystem — IDT, ISR stubs, PIC.
     */
    idt_init();
    irq_init();

    /*
     * 7. Peripherals that rely on IRQs.
     */
    pit_init(100);          /* 100 Hz scheduler tick */
    keyboard_init();        /* PS/2 keyboard on IRQ1 */

    /*
     * 8. All subsystems ready.
     */
    kprintf("[ OK ] Kernel entered 64-bit long mode successfully.\n");
    kprintf("=== Kernel running — interrupts enabled ===\n");

    /* Enable interrupts.  From this point on, hlt will be woken by PIT. */
    __asm__ volatile ("sti");

    /*
     * Idle loop — tick counter increments in the background.
     * Every ~5 s we print a heartbeat so we know interrupts are alive.
     */
    u64 last = 0;
    while (1) {
        __asm__ volatile ("hlt");
        if (g_tick - last >= 500) {
            kprintf("[tick %llu] ", g_tick);
            last = g_tick;
        }
    }
}
