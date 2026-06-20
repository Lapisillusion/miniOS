# Makefile — miniOS kernel build system
#
# Usage:
#   make BOOT_METHOD=multiboot2    Build for GRUB2/Multiboot2 (default)
#   make BOOT_METHOD=bios          Build for legacy BIOS
#   make BOOT_METHOD=uefi          Build for UEFI  (Phase 1B)
#
#   make iso       Build bootable ISO    (multiboot2 only)
#   make disk      Build raw disk image  (bios only)
#   make run       Build and run in QEMU
#   make clean     Remove build artifacts

BOOT_METHOD ?= multiboot2

# ---------------------------------------------------------------------------
# Toolchain
# ---------------------------------------------------------------------------
CC       := gcc
AS       := as
LD       := ld
OBJCOPY  := objcopy
NASM     := nasm

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

# ---------------------------------------------------------------------------
# Shared kernel objects  (used by all boot methods)
# ---------------------------------------------------------------------------
CORE_C_OBJS := \
    init/main.o \
    init/start_kernel.o \
    drivers/vga/vga.o \
    drivers/serial/serial.o \
    lib/vsprintf.o \
    lib/printf.o \
    lib/assert.o

# ---------------------------------------------------------------------------
# Boot-method-specific objects and flags
# ---------------------------------------------------------------------------
ifeq ($(BOOT_METHOD),bios)
    BOOT_ASM_OBJS := arch/x86_64/boot/bios/head64.o
    BOOT_C_OBJS   := init/bios_init.o
    LDSCRIPT      := linker_bios.ld
    KERNEL_BIN    := kernel.bin
else ifeq ($(BOOT_METHOD),multiboot2)
    BOOT_ASM_OBJS := arch/x86_64/boot/multiboot2/header.o
    BOOT_C_OBJS   := init/mb2_init.o
    LDSCRIPT      := linker.ld
    KERNEL_BIN    :=
else ifeq ($(BOOT_METHOD),uefi)
    $(error UEFI boot not yet implemented)
endif

OBJS := $(BOOT_ASM_OBJS) $(BOOT_C_OBJS) $(CORE_C_OBJS)

LDFLAGS := -nostdlib -T $(LDSCRIPT) -z max-page-size=0x1000

# ---------------------------------------------------------------------------
# Top-level targets
# ---------------------------------------------------------------------------
.PHONY: all iso disk run clean

all: kernel.elf

ifeq ($(BOOT_METHOD),bios)
all: kernel.bin
endif

# ---------------------------------------------------------------------------
# Link
# ---------------------------------------------------------------------------
kernel.elf: $(OBJS) $(LDSCRIPT)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)
	@echo "  [LD]   kernel.elf ready"

ifeq ($(BOOT_METHOD),bios)
kernel.bin: kernel.elf
	$(OBJCOPY) -O binary $< $@
	@echo "  [OBJ]  kernel.bin ready ($(shell stat -c%s $@ 2>/dev/null || echo ?) bytes)"
endif

# ---------------------------------------------------------------------------
# Build rules
# ---------------------------------------------------------------------------
%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<
	@echo "  [CC]   $<"

%.o: %.s
	@mkdir -p $(dir $@)
	$(AS) $(AFLAGS) -o $@ $<
	@echo "  [AS]   $<"

# ---------------------------------------------------------------------------
# NASM build rules (for BIOS boot components)
# ---------------------------------------------------------------------------
arch/x86_64/boot/bios/mbr.bin: arch/x86_64/boot/bios/mbr.s
	$(NASM) -f bin -o $@ $<
	@echo "  [NASM] mbr.bin ($(shell stat -c%s $@ 2>/dev/null || echo ?) bytes)"

arch/x86_64/boot/bios/stage2.bin: arch/x86_64/boot/bios/stage2.s kernel.bin
	$(NASM) -f bin \
		-dKERNEL_SECTORS=$(shell echo $$(( ($$(stat -c%s kernel.bin 2>/dev/null || echo 1) + 511) / 512 ))) \
		-o $@ $<
	@echo "  [NASM] stage2.bin ($(shell stat -c%s $@ 2>/dev/null || echo ?) bytes)"

# ---------------------------------------------------------------------------
# Disk image  (BIOS only)
# ---------------------------------------------------------------------------
disk.img: arch/x86_64/boot/bios/mbr.bin \
          arch/x86_64/boot/bios/stage2.bin \
          kernel.bin
	@echo "  [DISK] Building disk.img..."
	@# Stage2 is padded to exactly 8 sectors (4096 bytes)
	@cp arch/x86_64/boot/bios/stage2.bin /tmp/stage2.bin
	@truncate -s 4096 /tmp/stage2.bin
	@# Kernel: pad to fill the KERNEL_SECTORS worth of sectors
	@KERNEL_SEC=$$(echo $$(( ($$(stat -c%s kernel.bin) + 511) / 512 ))); \
	cp kernel.bin /tmp/kernel.bin; \
	truncate -s $$((KERNEL_SEC * 512)) /tmp/kernel.bin
	@# Concatenate: MBR (512) + stage2 (4096) + kernel (padded to sector boundary)
	@cat arch/x86_64/boot/bios/mbr.bin \
	     /tmp/stage2.bin \
	     /tmp/kernel.bin > disk.img
	@rm -f /tmp/stage2.bin /tmp/kernel.bin
	@echo "  [DISK] disk.img ready ($(shell stat -c%s disk.img 2>/dev/null || echo ?) bytes)"

disk: disk.img

# ---------------------------------------------------------------------------
# ISO image  (Multiboot2 only)
# ---------------------------------------------------------------------------
iso: kernel.elf
	@mkdir -p isodir/boot/grub
	cp kernel.elf isodir/boot/kernel.elf
	cp scripts/grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o miniOS.iso isodir 2>/dev/null
	@echo "  [ISO]  miniOS.iso ready"

# ---------------------------------------------------------------------------
# Run in QEMU
# ---------------------------------------------------------------------------
ifeq ($(BOOT_METHOD),bios)
run: disk.img
	qemu-system-x86_64 \
		-drive format=raw,file=disk.img \
		-serial stdio \
		-no-reboot \
		-no-shutdown \
		-m 256M
else ifeq ($(BOOT_METHOD),multiboot2)
run: iso
	qemu-system-x86_64 \
		-cdrom miniOS.iso \
		-serial stdio \
		-no-reboot \
		-no-shutdown \
		-m 256M
endif

# ---------------------------------------------------------------------------
# Debug run  (QEMU with GDB stub)
# ---------------------------------------------------------------------------
run-debug:
ifeq ($(BOOT_METHOD),bios)
	qemu-system-x86_64 \
		-drive format=raw,file=disk.img \
		-serial stdio \
		-no-reboot \
		-no-shutdown \
		-m 256M \
		-s -S
else
	qemu-system-x86_64 \
		-cdrom miniOS.iso \
		-serial stdio \
		-no-reboot \
		-no-shutdown \
		-m 256M \
		-s -S
endif

# ---------------------------------------------------------------------------
# GDB
# ---------------------------------------------------------------------------
debug:
	gdb -ex "target remote localhost:1234" \
	    -ex "symbol-file kernel.elf"

# ---------------------------------------------------------------------------
# Clean
# ---------------------------------------------------------------------------
clean:
	rm -rf kernel.elf kernel.bin miniOS.iso disk.img isodir
	rm -f arch/x86_64/boot/bios/mbr.bin
	rm -f arch/x86_64/boot/bios/stage2.bin
	find . -name '*.o' -delete
	@echo "  [CLEAN] done"
