/*
 * mm/pmm.c — Bitmap-based physical memory manager
 *
 * Tracks every 4 KiB physical page with a single bit:
 *   1 = used (allocated or reserved)
 *   0 = free
 *
 * The bitmap is placed right after _kernel_end so it doesn't rely
 * on any allocator.  At init time all bits are set to 1 ("used")
 * and then cleared for every usable region in the memory map.
 * This guarantees that firmware-reserved holes, MMIO ranges, and
 * the kernel image itself are never handed out.
 */

#include <mm/pmm.h>
#include <miniOS/types.h>
#include <miniOS/boot_info.h>
#include <miniOS/compiler.h>
#include <lib/printf.h>
#include <lib/assert.h>

/* ---- Linker-defined symbols -------------------------------------------- */

/* End of the kernel BSS — first free physical address after the kernel */
extern u8 _kernel_end[];

/* ---- Bitmap state ------------------------------------------------------ */

static u8   *bitmap;          /* pointer to the bitmap array            */
static u64   bitmap_bytes;    /* size of bitmap in bytes                */
static u64   total_pages;     /* total tracked pages                    */
static u64   free_pages;      /* currently free pages                   */

/* ---- Internal helpers -------------------------------------------------- */

/** Set bit → page is now in use */
static inline void bitmap_set(u64 page)
{
	bitmap[page / 8] |= (u8)(1U << (page % 8));
}

/** Clear bit → page is free */
static inline void bitmap_clear(u64 page)
{
	bitmap[page / 8] &= (u8)~(1U << (page % 8));
}

/** Test whether a page is marked as used */
static inline int bitmap_test(u64 page)
{
	return (bitmap[page / 8] >> (page % 8)) & 1;
}

/**
 * mark_used — Mark a physical address range as "used" in the bitmap.
 *
 * Pages already marked used are skipped (no double-count of free_pages).
 */
static void mark_used(u64 start, u64 end)
{
	u64 sp = ALIGN_DOWN(start, PAGE_SIZE) / PAGE_SIZE;
	u64 ep = PAGE_ALIGN(end) / PAGE_SIZE;

	if (ep > total_pages)
		ep = total_pages;
	for (u64 p = sp; p < ep; p++) {
		if (!bitmap_test(p)) {
			bitmap_set(p);
			free_pages--;
		}
	}
}

/**
 * mark_free — Mark a physical address range as "free" in the bitmap.
 *
 * Pages already marked free are skipped.
 */
static void mark_free(u64 start, u64 end)
{
	u64 sp = PAGE_ALIGN(start) / PAGE_SIZE;
	u64 ep = ALIGN_DOWN(end, PAGE_SIZE) / PAGE_SIZE;

	if (ep > total_pages)
		ep = total_pages;
	for (u64 p = sp; p < ep; p++) {
		if (bitmap_test(p)) {
			bitmap_clear(p);
			free_pages++;
		}
	}
}

/**
 * find_first_free — Scan the bitmap for the first zero bit.
 *
 * Returns the page index, or (u64)-1 when no page is available.
 * Uses __builtin_ctz which maps to the BSF instruction on x86.
 */
static u64 find_first_free(void)
{
	for (u64 byte = 0; byte < bitmap_bytes; byte++) {
		if (bitmap[byte] != 0xFF) {
			/* ~bitmap[byte] has 1-bits where the original has 0-bits.
			 * __builtin_ctz counts trailing zeros → position of the
			 * lowest free bit in this byte. */
			unsigned int inv = (u8)~bitmap[byte];
			int bit = __builtin_ctz(inv);
			return byte * 8 + (u64)bit;
		}
	}
	return (u64)-1;
}

/* ---- Public API -------------------------------------------------------- */

void pmm_init(boot_info_t *info)
{
	if (!info || !info->memory_map_count)
		kpanic("pmm_init: no memory map");

	/* 1. Find the maximum physical address among *usable* regions only.
	 *    Using all entries would include MMIO / PCI ECAM at > 64 GiB,
	 *    making the bitmap impractically large. */
	u64 max_pa = 0;
	for (u32 i = 0; i < info->memory_map_count; i++) {
		if (info->memory_map[i].type != MEM_TYPE_USABLE)
			continue;
		u64 end = info->memory_map[i].base + info->memory_map[i].length;
		if (end > max_pa)
			max_pa = end;
	}

	if (max_pa == 0)
		kpanic("pmm_init: empty memory map");

	/* 2. Calculate bitmap dimensions */
	total_pages  = max_pa / PAGE_SIZE;
	bitmap_bytes = total_pages / 8;
	if (total_pages % 8)
		bitmap_bytes++;

	/* 3. Place the bitmap right after the kernel image */
	bitmap = (u8 *)PAGE_ALIGN((u64)_kernel_end);

	/* 4. Default: everything is used */
	for (u64 i = 0; i < bitmap_bytes; i++)
		bitmap[i] = 0xFF;
	free_pages = 0;

	/* 5. Mark usable regions as free */
	for (u32 i = 0; i < info->memory_map_count; i++) {
		if (info->memory_map[i].type == MEM_TYPE_USABLE) {
			mark_free(info->memory_map[i].base,
				  info->memory_map[i].base +
				  info->memory_map[i].length);
		}
	}

	/* 6. Re-protect: kernel + bitmap must stay reserved.
	 *    Everything from physical 0 up to the end of the bitmap
	 *    is re-marked as used, which covers:
	 *      - IVT / BDA / BIOS data (0 – 1 MiB)
	 *      - Kernel image (text, rodata, data, bss)
	 *      - Bitmap pages themselves
	 */
	u64 reserved_end = PAGE_ALIGN((u64)bitmap + bitmap_bytes);
	mark_used(0, reserved_end);

	/* 7. Sanity check */
	if (free_pages == 0)
		kpanic("pmm_init: zero free pages — bitmap too large?");

	kprintf("  pmm     : %llu pages total, %llu free, "
		"bitmap %llu KiB at %p\n",
		total_pages, free_pages,
		PAGE_ALIGN(bitmap_bytes) / 1024, bitmap);
}

void *pmm_alloc_page(void)
{
	u64 idx = find_first_free();
	if (idx == (u64)-1)
		kpanic("pmm_alloc_page: out of memory — no free pages");
	bitmap_set(idx);
	free_pages--;
	return (void *)(idx * PAGE_SIZE);
}

void pmm_free_page(void *page)
{
	u64 addr = (u64)page;

	/* Page-alignment sanity */
	if (addr & (PAGE_SIZE - 1))
		return;

	u64 idx = addr / PAGE_SIZE;
	if (idx >= total_pages)
		return;

	/* Silent ignore of double-free (page was already free) */
	if (!bitmap_test(idx))
		return;

	bitmap_clear(idx);
	free_pages++;
}

u64 pmm_total_pages(void) { return total_pages; }
u64 pmm_free_pages(void)  { return free_pages; }
