# Makefile — miniOS kernel build system
#
# Usage:
#   make                       Build kernel ELF
#   make iso                   Build bootable ISO  (multiboot2 mode)
#   make run                   Build ISO and run in QEMU
#   make clean                 Remove build artifacts
#   make BOOT_METHOD=bios      Switch boot method (future phases)
#   make BOOT_METHOD=uefi      Switch boot method (future phases)

BOOT_METHOD ?= multiboot2

# ---------------------------------------------------------------------------
# Toolchain
# ---------------------------------------------------------------------------
CC       := gcc
AS       := as
LD       := ld
OBJCOPY  := objcopy

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
ARCH_DIR     := arch/x86_64
INCLUDE_DIRS := -Iinclude -I$(ARCH_DIR)/include

# ---------------------------------------------------------------------------
# Compiler & linker flags
# ---------------------------------------------------------------------------
CFLAGS := -std=gnu99 -ffreestanding -O0 -g -gdwarf-4 \
          -Wall -Wextra \
          -nostdinc -nostdlib -fno-stack-protector \
          -fno-pic -fno-pie \
          -mno-red-zone -mcmodel=kernel \
          -mno-mmx -mno-sse -mno-sse2 -mno-80387 -mgeneral-regs-only \
          $(INCLUDE_DIRS)

AFLAGS := -nostdinc -nostdlib --gdwarf-2

LDFLAGS := -nostdlib -T linker.ld -z max-page-size=0x1000

# ---------------------------------------------------------------------------
# Kernel objects
# ---------------------------------------------------------------------------
C_OBJS := \
    init/main.o \
    drivers/vga/vga.o \
    drivers/serial/serial.o

ASM_OBJS := \
    arch/x86_64/boot/multiboot2/header.o

OBJS := $(ASM_OBJS) $(C_OBJS)

# ---------------------------------------------------------------------------
# Targets
# ---------------------------------------------------------------------------
.PHONY: all iso run clean debug

all: kernel.elf

kernel.elf: $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)
	@echo "  [LD]   kernel.elf ready"

# C compilation rule
%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<
	@echo "  [CC]   $<"

# Assembly rule
%.o: %.s
	@mkdir -p $(dir $@)
	$(AS) $(AFLAGS) -o $@ $<
	@echo "  [AS]   $<"

# ISO image (Multiboot2 via GRUB)
iso: kernel.elf
	@mkdir -p isodir/boot/grub
	cp kernel.elf isodir/boot/kernel.elf
	cp scripts/grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o miniOS.iso isodir 2>/dev/null
	@echo "  [ISO]  miniOS.iso ready"

# Run in QEMU
run: iso
	qemu-system-x86_64 \
		-cdrom miniOS.iso \
		-serial stdio \
		-no-reboot \
		-no-shutdown \
		-m 256M

# Debug mode: QEMU waits for GDB on port 1234
run-debug: iso
	qemu-system-x86_64 \
		-cdrom miniOS.iso \
		-serial stdio \
		-no-reboot \
		-no-shutdown \
		-m 256M \
		-s -S

# Connect GDB
debug:
	gdb -ex "target remote localhost:1234" \
	    -ex "symbol-file kernel.elf" \
	    -ex "b kmain" \
	    -ex "c"

# Clean
clean:
	rm -rf kernel.elf miniOS.iso isodir $(OBJS)
	find . -name '*.o' -delete
	@echo "  [CLEAN] done"
