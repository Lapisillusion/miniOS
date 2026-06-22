#include <miniOS/types.h>
#include <miniOS/boot_info.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <drivers/pit.h>
#include <drivers/keyboard.h>
#include <lib/printf.h>
#include <lib/assert.h>
#include <mm/pmm.h>

extern void idt_init(void);
extern void irq_init(void);

static void count_memory(boot_info_t *info)
{
    if (!info || !info->memory_map_count) { kprintf("  mem: no map\n"); return; }
    u64 tot = 0; u32 uc = 0;
    for (u32 i = 0; i < info->memory_map_count; i++)
        if (info->memory_map[i].type == MEM_TYPE_USABLE) { tot += info->memory_map[i].length; uc++; }
    kprintf("  mem     : %d/%d entries, %llu MiB usable\n",
            uc, info->memory_map_count, tot / (1024*1024));
}

void start_kernel(boot_info_t *info)
{
    serial_init();
    vga_init();
    vga_set_color(vga_entry_color(VGA_LIGHT_GREEN, VGA_BLACK));

    kprintf("\n=== miniOS kernel started ===\nminiOS booting...\n");
    kprintf("  arch    : x86_64\n");
    if (info && info->framebuffer_bpp == 16)
        kprintf("  display : VGA text mode (%ux%u)\n", info->framebuffer_width, info->framebuffer_height);
    else if (info && info->framebuffer_addr)
        kprintf("  display : %ux%u %ubpp linear fb\n", info->framebuffer_width, info->framebuffer_height, info->framebuffer_bpp);
    count_memory(info);

    pmm_init(info);

    /* Quick smoke test: allocate two pages, verify they differ,
     * free them, confirm the free count returns to baseline. */
    {
        u64 before = pmm_free_pages();
        void *p1 = pmm_alloc_page();
        void *p2 = pmm_alloc_page();
        kprintf("  pmm test: alloc %p %p (%llu -> %llu free)\n",
                p1, p2, before, pmm_free_pages());
        if (p1 == p2)
            kpanic("pmm test: alloc returned same page twice");
        pmm_free_page(p1);
        pmm_free_page(p2);
        u64 after = pmm_free_pages();
        if (after != before)
            kpanic("pmm test: free count mismatch");
        kprintf("  pmm test: OK (%llu free after free)\n", after);
    }

    idt_init();
    irq_init();
    pit_init(100);
    keyboard_init();

    kprintf("[ OK ] Kernel entered 64-bit long mode successfully.\n");
    kprintf("=== Kernel running — interrupts enabled ===\n");
    __asm__ volatile ("sti");

    u64 last = 0;
    while (1) { __asm__ volatile ("hlt"); if (g_tick - last >= 500) { kprintf("[tick %llu] ", g_tick); last = g_tick; } }
}
