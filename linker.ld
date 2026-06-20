/*
 * linker.ld — Kernel linker script for miniOS (x86_64) — Multiboot2 path
 *
 * Layout:
 *   .multiboot  @ 1M          (Multiboot2 header — MUST be first)
 *   .text32      @ 1M + 4K     (32-bit bootstrap code)
 *   .text        @ 1M + 8K     (64-bit kernel text)
 *   .rodata                    (read-only data)
 *   .data                      (initialised data)
 *   .bss                       (zero-initialised data, page tables, stack)
 *
 * All sections are folded into a single .text output section so the
 * linker does not drop the small bootstrap sections.
 */

ENTRY(_start)
OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

SECTIONS
{
    . = 1M;

    .text BLOCK(4K) : ALIGN(4K) {
        /* Multiboot2 header — must stay within the first 32 KiB */
        KEEP(*(.multiboot))

        /* 32-bit bootstrap (entry point) */
        . = ALIGN(4K);
        *(.text32)

        /* 64-bit kernel */
        _text_start = .;
        *(.text .text.*)
        _text_end = .;
    }

    /* ---- Read-only data ---- */
    .rodata : ALIGN(4K) {
        _rodata_start = .;
        *(.rodata .rodata.*)
        _rodata_end = .;
    }

    /* ---- Initialised data ---- */
    .data : ALIGN(4K) {
        _data_start = .;
        *(.data .data.*)
        _data_end = .;
    }

    /* ---- BSS (zero-initialised data, page tables, stack) ---- */
    .bss : ALIGN(4K) {
        _bss_start = .;
        *(COMMON)
        *(.bss .bss.*)
        *(.bss.page_tables)
        *(.bss.stack)
        _bss_end = .;
    }

    _kernel_end = .;
}
