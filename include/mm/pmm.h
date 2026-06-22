/*
 * include/mm/pmm.h — Physical Memory Manager interface
 *
 * Implements a simple bitmap-based physical page allocator.
 * Each bit in the bitmap represents one 4 KiB physical page.
 *
 * Usage:
 *   pmm_init(info)                  — once at boot
 *   void *p = pmm_alloc_page()      — get one free physical page
 *   pmm_free_page(p)                — return it
 *   pmm_free_pages()                — how many are left
 */

#ifndef _MINIOS_PMM_H
#define _MINIOS_PMM_H

#include <miniOS/types.h>
#include <miniOS/boot_info.h>

/* Number of 4 KiB pages one bitmap byte covers */
#define PMM_PAGES_PER_BYTE  8

/* Number of pages a single bitmap page can track */
#define PMM_PAGES_PER_BITMAP_PAGE  (PAGE_SIZE * 8)

/* ---- Public API --------------------------------------------------------- */

/**
 * pmm_init - Initialise the physical memory manager.
 *
 * Parses the memory map from boot_info, places a bitmap right after
 * _kernel_end, marks all usable pages as free and everything else
 * (kernel image, bitmap itself, BIOS areas, MMIO holes) as used.
 *
 * Must be called exactly once, before any pmm_alloc_page / pmm_free_page.
 */
void pmm_init(boot_info_t *info);

/**
 * pmm_alloc_page - Allocate a single zero-filled 4 KiB physical page.
 *
 * Returns the physical address of the page, or panics if OOM.
 * The page is NOT zeroed — the caller must zero it if needed.
 */
void *pmm_alloc_page(void);

/**
 * pmm_free_page - Return a previously allocated physical page.
 *
 * @page  Physical address returned by pmm_alloc_page().
 *        Must be page-aligned.  Double-free is silently ignored.
 */
void pmm_free_page(void *page);

/**
 * pmm_total_pages - Total number of physical pages tracked by the bitmap.
 */
u64 pmm_total_pages(void);

/**
 * pmm_free_pages - Number of currently free physical pages.
 */
u64 pmm_free_pages(void);

#endif /* _MINIOS_PMM_H */
