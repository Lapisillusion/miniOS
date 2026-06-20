/*
 * init/mb2_init.c — Multiboot2 information structure parser
 *
 * Called by the 64-bit entry in header.s with the raw multiboot2 info
 * pointer that GRUB passes in ebx.  Walks the tag list, extracts
 * memory map + framebuffer info, and hands a unified boot_info_t to kmain.
 */

#include <miniOS/types.h>
#include <miniOS/boot_info.h>
#include <miniOS/compiler.h>
#include <drivers/vga.h>
#include <drivers/serial.h>

#define MB2_TAG_END          0
#define MB2_TAG_CMDLINE      1
#define MB2_TAG_BOOT_DEVICE  5
#define MB2_TAG_MEMMAP       6
#define MB2_TAG_FRAMEBUFFER  8

/* ---- Multiboot2 structure definitions ---- */

typedef struct {
    u32 total_size;
    u32 reserved;
} __packed mb2_info_t;

typedef struct {
    u32 type;
    u32 size;
} __packed mb2_tag_t;

typedef struct {
    u32 type;          /* 6 */
    u32 size;
    u32 entry_size;    /* size of each mmap entry (>= 20) */
    u32 entry_version; /* currently 0 */
} __packed mb2_tag_mmap_t;

typedef struct {
    u64 base_addr;
    u64 length;
    u32 type;
    u32 __reserved;
} __packed mb2_mmap_entry_t;

typedef struct {
    u32 type;          /* 8 */
    u32 size;
    u64 addr;
    u32 pitch;
    u32 width;
    u32 height;
    u8  bpp;
    u8  fb_type;       /* 0=indexed, 1=RGB, 2=EGAText */
    u16 __reserved;
} __packed mb2_tag_framebuffer_t;

extern void kmain(boot_info_t *info);

/* ------------------------------------------------------------------ */

void mb2_init(void *mb2_raw)
{
    /* Placed in .bss — zeroed by the boot code before we run */
    static boot_info_t info;

    /* ---- Stage 1: set safe defaults ---- */
    info.framebuffer_addr  = (void *)0xB8000;
    info.framebuffer_width  = 80;
    info.framebuffer_height = 25;
    info.framebuffer_pitch  = 160;
    info.framebuffer_bpp    = 16;
    info.memory_map_count   = 0;
    info.kernel_phys_base   = 0;
    info.kernel_phys_end    = 0;
    info.mb2_info           = mb2_raw;

    if (mb2_raw == NULL)
        goto no_tags;

    /* ---- Stage 2: walk the tag list ---- */
    mb2_info_t *hdr = (mb2_info_t *)mb2_raw;
    u8          *ptr = (u8 *)mb2_raw + 8;  /* skip the fixed header */

    while (ptr < (u8 *)mb2_raw + hdr->total_size) {
        mb2_tag_t *tag = (mb2_tag_t *)ptr;

        if (tag->type == MB2_TAG_END)
            break;

        switch (tag->type) {

        /* ----- memory map ----- */
        case MB2_TAG_MEMMAP: {
            mb2_tag_mmap_t  *mmap   = (mb2_tag_mmap_t *)tag;
            mb2_mmap_entry_t *src   = (mb2_mmap_entry_t *)(ptr + 16);
            int count = (int)((mmap->size - 16) / mmap->entry_size);

            if (count > 128)
                count = 128;

            for (int i = 0; i < count; i++) {
                info.memory_map[i].base   = src[i].base_addr;
                info.memory_map[i].length = src[i].length;
                info.memory_map[i].type   = src[i].type;
            }
            info.memory_map_count = (u32)count;
            break;
        }

        /* ----- linear framebuffer ----- */
        case MB2_TAG_FRAMEBUFFER: {
            mb2_tag_framebuffer_t *fb =
                (mb2_tag_framebuffer_t *)tag;

            if (fb->fb_type == 1 && fb->addr != 0) {
                /* RGB framebuffer available */
                info.framebuffer_addr  = (void *)(uintptr_t)fb->addr;
                info.framebuffer_width  = fb->width;
                info.framebuffer_height = fb->height;
                info.framebuffer_pitch  = fb->pitch;
                info.framebuffer_bpp    = fb->bpp;
            }
            /* fb_type == 2: text mode — keep VGA defaults */
            break;
        }

        default:
            break;
        }

        /* Advance to next tag, 8-byte aligned */
        ptr += (tag->size + 7) & ~7UL;
    }

no_tags:
    kmain(&info);
}
