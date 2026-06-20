/*
 * linker_bios.ld — Kernel linker script for the BIOS boot path
 *
 * The kernel is loaded at 0x100000 (1 MiB) as a flat binary.
 * head64.s (.text.head) is linked first so that bios64_entry sits at the
 * very beginning of the binary; stage2 jumps to 0x100000 without needing
 * to know any intra-kernel offsets.
 *
 * BSS is NOT part of the flat binary (objcopy -O binary drops it), so
 * head64.s zeroes it explicitly at boot.
 */

ENTRY(bios64_entry)
OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)

SECTIONS
{
    . = 0x100000;                       /* 1 MiB */

    .text : ALIGN(4K) {
        *(.text.head)                   /* bios64_entry — MUST be first */
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
