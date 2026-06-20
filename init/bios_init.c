/*
 * init/bios_init.c — BIOS-specific early init
 *
 * Called by head64.s with the raw E820 buffer from stage2.
 * Constructs a unified boot_info_t and hands off to kmain().
 */

#include <miniOS/types.h>
#include <miniOS/boot_info.h>
#include <drivers/vga.h>
#include <drivers/serial.h>

/* kmain is in init/main.c */
extern void kmain(boot_info_t *info);

/*
 * bios_init — bridge from BIOS boot path to the common kernel entry.
 *
 * @e820_buf:   pointer to the raw E820 entries collected by stage2
 * @e820_count: number of valid entries (max 128)
 */
void bios_init(void *e820_buf, int e820_count)
{
    /* Placed in .bss — zeroed by head64.s before we run */
    static boot_info_t info;

    /*
     * Framebuffer: BIOS boot does not request a linear framebuffer;
     * we default to the VGA text-mode buffer at 0xB8000.
     */
    info.framebuffer_addr  = (void *)0xB8000;
    info.framebuffer_width  = 80;
    info.framebuffer_height = 25;
    info.framebuffer_pitch  = 160;    /* 80 cols × 2 bytes/char */
    info.framebuffer_bpp    = 16;     /* 4-bit fg + 4-bit bg */

    /*
     * Copy E820 entries into the unified boot_info.
     * Both the raw buffer and memory_map_entry_t are 20-byte structures
     * (u64 base, u64 length, u32 type), so a straight memcpy works.
     */
    if (e820_count > 0 && e820_buf != NULL) {
        int n = (e820_count > 128) ? 128 : e820_count;

        memory_map_entry_t *dst = info.memory_map;
        memory_map_entry_t *src = (memory_map_entry_t *)e820_buf;

        for (int i = 0; i < n; i++)
            dst[i] = src[i];

        info.memory_map_count = (u32)n;
    } else {
        info.memory_map_count = 0;
    }

    /* Kernel physical location (hard-coded — matches linker script) */
    info.kernel_phys_base = 0x100000;
    info.kernel_phys_end  = 0;        /* filled later when we know size */

    /*
     * Hand off to the architecture-independent kernel entry.
     * kmain() never returns.
     */
    kmain(&info);

    /* NOTREACHED */
    while (1)
        __asm__ volatile ("hlt");
}
