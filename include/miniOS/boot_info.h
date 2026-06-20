#ifndef _MINIOS_BOOT_INFO_H
#define _MINIOS_BOOT_INFO_H

#include <miniOS/types.h>
#include <miniOS/compiler.h>

/* Memory map entry types */
#define MEM_TYPE_USABLE      1
#define MEM_TYPE_RESERVED    2
#define MEM_TYPE_ACPI_RECLAIM 3
#define MEM_TYPE_ACPI_NVS    4
#define MEM_TYPE_BAD         5

/* Memory map entry — exactly 20 bytes, matches BIOS E820 format */
typedef struct {
    u64 base;
    u64 length;
    u32 type;
} __packed memory_map_entry_t;

/*
 * Unified boot information structure.
 * Filled in by each boot method (BIOS/UEFI/Multiboot2)
 * and passed to kmain().
 */
typedef struct {
    /* Framebuffer */
    void     *framebuffer_addr;
    u32       framebuffer_width;
    u32       framebuffer_height;
    u32       framebuffer_pitch;
    u8        framebuffer_bpp;

    /* Physical memory map */
    memory_map_entry_t memory_map[128];
    u32                memory_map_count;

    /* Kernel location in physical memory */
    u64       kernel_phys_base;
    u64       kernel_phys_end;

    /* Multiboot2 info pointer (for mb2_parse to use) */
    void     *mb2_info;
} boot_info_t;

#endif /* _MINIOS_BOOT_INFO_H */
