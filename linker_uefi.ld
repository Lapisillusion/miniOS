/*
 * linker_uefi.ld — Kernel linker script for the UEFI boot path
 *
 * The kernel is built as a standard ELF64, loaded by the UEFI bootloader
 * at the physical addresses specified in the program headers.
 */

ENTRY(_start)
OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

SECTIONS
{
    . = 1M;

    .text : ALIGN(4K) {
        *(.text.head)                   /* uefi/entry.s — must be first */
        _text_start = .;
        *(.text .text.*)
        _text_end = .;
    }

    .rodata : ALIGN(4K) {
        _rodata_start = .;
        *(.rodata .rodata.*)
        _rodata_end = .;
    }

    .data : ALIGN(4K) {
        _data_start = .;
        *(.data .data.*)
        _data_end = .;
    }

    .bss : ALIGN(4K) {
        _bss_start = .;
        *(COMMON)
        *(.bss .bss.*)
        *(.bss.stack)
        _bss_end = .;
    }

    _kernel_end = .;
}
