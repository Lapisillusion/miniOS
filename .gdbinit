# GDB init script for miniOS debugging
# This file is auto-loaded by GDB when debugging from the project root.

# Disable pagination — essential for VSCode GDB/MI
set pagination off

# Confirm off — don't ask "are you sure?" on every step
set confirm off

# Print function arguments on each step
set print frame-arguments all

# Pretty-print structures
set print pretty on

# Show 8 lines of disassembly around $pc
# set disassemble-next-line on

# Better info registers output
set print all-registers on

# ---------------------------------------------------------------------------
# Useful commands (type in VSCode Debug Console):
# ---------------------------------------------------------------------------
# info registers         — show all CPU registers
# info registers rax     — show specific register
# x/40x 0xB8000          — dump 40 dwords from VGA memory
# x/s 0xB8000            — display VGA memory as string
# x/1i $pc               — disassemble current instruction
# x/512x 0x105000        — dump PML4 (first page table)
# break kmain            — set breakpoint at kmain
# info breakpoints       — list all breakpoints
# delete                 — delete all breakpoints

# Symbols are loaded from kernel.elf; VSCode handles this.
# Target connection is also handled by VSCode via miDebuggerServerAddress.

echo [GDB] miniOS debug init loaded\n
