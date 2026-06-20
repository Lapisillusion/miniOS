#!/usr/bin/env python3
"""Patch a PE32+ image to add the IMAGE_FILE_DLL (0x2000) characteristic.
   This is required for UEFI applications built with objcopy pei-x86-64."""

import struct, sys

def patch_pe(path):
    with open(path, 'r+b') as f:
        # DOS header offset 0x3C → PE signature offset
        f.seek(0x3C)
        pe_off = struct.unpack('<I', f.read(4))[0]
        # COFF header: PE sig (4) + COFF fields, Characteristics at +18
        f.seek(pe_off + 4 + 18)
        ch = struct.unpack('<H', f.read(2))[0]
        ch |= 0x2000   # IMAGE_FILE_DLL
        f.seek(pe_off + 4 + 18)
        f.write(struct.pack('<H', ch))
    print(f"  [PATCH] {path}: characteristics now 0x{ch:04x}")

if __name__ == '__main__':
    patch_pe(sys.argv[1])
