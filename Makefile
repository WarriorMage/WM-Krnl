# SPDX-License-Identifier: GPL-3.0-or-later

# These are not real files, run these rules even if an up to date file in the same name exists.
.PHONY: all run debug clean clean_kernel clean_lib lib

# -------- Tools --------
CC      = i686-elf-gcc
AS      = nasm
EMU		= qemu-system-i386
AR 		= i686-elf-ar

# -------- Directories --------
SRC_BOOT    = src/boot
SRC_KERNEL  = src/kernel
SRC_LIB		= src/lib
SRC_PROC	= src/test_processes

BUILD_BOOT   = build/boot
BUILD_KERNEL = build/kernel
BUILD_LIB    = build/lib
BUILD_PROC   = build/test_processes

# -------- Flags --------
# -ffreestanding: Don't replace my code with libc code for optimizing, it doesn't exist
# -fno-stack-protector: Modern compilers protect against "Stack Smashing" by adding a random number to the stack. When a function returns, it checks if that number is still there. If it's gone, it calls an error-handling function. Disable it we don't have any error-handler, or the random number creator.
# -fno-pie: Use absolute addressing. Don't emit relative symbols in output, there
# is no loader to decide the addresses.
KERNEL_CFLAGS  = -ffreestanding -fno-pie -fno-stack-protector -O0 -Wall -Wextra -Isrc/kernel
LIB_CFLAGS  = -ffreestanding -fno-pie -fno-stack-protector -O0 -Wall -Wextra -Isrc/lib

# Don't link the C runtime to setup argc, argv, entry point till main, we don't 
# have it. Also use the 32 bit linker with ELF input.
LDFLAGS = -T $(SRC_KERNEL)/linker.ld -nostdlib
ASFLAGS = -f elf32
ARFLAGS = rcs

# -------- Files --------
BOOT       = $(BUILD_BOOT)/bin/first.bin
KERNEL_ELF = $(BUILD_KERNEL)/bin/kernel.elf
KERNEL     = $(BUILD_KERNEL)/bin/kernel.bin
IMAGE      = $(BUILD_KERNEL)/img/os.img
LIBRARY    = $(BUILD_LIB)/ar/lib.a

# -------- Sources - Kernel ------
KERNEL_C_SRC   := $(shell find $(SRC_KERNEL) -name "*.c")
KERNEL_ASM_SRC := $(shell find $(SRC_KERNEL) -name "*.asm")

# Suffix change syntax, .asm.o to avoid name collisions with x.c and x.asm
KERNEL_OBJ_C   := $(patsubst $(SRC_KERNEL)/%.c,$(BUILD_KERNEL)/obj/%.c.o,$(KERNEL_C_SRC))
KERNEL_OBJ_ASM := $(patsubst $(SRC_KERNEL)/%.asm,$(BUILD_KERNEL)/obj/%.asm.o,$(KERNEL_ASM_SRC))
KERNEL_OBJ_SRC := $(KERNEL_OBJ_C) $(KERNEL_OBJ_ASM)

# -------- Sources - Library ------

LIB_C_SRC   := $(shell find $(SRC_LIB) -name "*.c")
LIB_ASM_SRC := $(shell find $(SRC_LIB) -name "*.asm")

LIB_OBJ_C   := $(patsubst $(SRC_LIB)/%.c,$(BUILD_LIB)/obj/%.c.o,$(LIB_C_SRC))
LIB_OBJ_ASM := $(patsubst $(SRC_LIB)/%.asm,$(BUILD_LIB)/obj/%.asm.o,$(LIB_ASM_SRC))
LIB_OBJ_SRC := $(LIB_OBJ_C) $(LIB_OBJ_ASM)

# -------- Sources - Processes ------

PROC_C_SRC   := $(shell find $(SRC_PROC) -name "*.c")
PROC_OBJ_SRC := $(patsubst $(SRC_PROC)/%.c,$(BUILD_PROC)/obj/%.c.o,$(PROC_C_SRC))

# first dependency =  $<
# all dependencies = $^
# target =  $@

# Default target
all: run

# Bootloader is a separate binary linked with nothing. It loads the kernel which is 
# a separate binary from the disk into memory. We directly jump from there so no
# need to link with the kernel.

# -------- Bootloader --------
$(BOOT): $(SRC_BOOT)/first.asm
	@mkdir -p $(dir $@)
	$(AS) -f bin $< -o $@

# We first compile to ELF object instead of raw since they have useful metadata
# such as section information that the linker can use which a raw object lacks. We
# are anyways producing a raw binary after linking. Check the linker script.

# -------- Kernel C -> ELF Object --------
$(BUILD_KERNEL)/obj/%.c.o: $(SRC_KERNEL)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(KERNEL_CFLAGS) -c $< -o $@ -g

# -------- Kernel ASM -> ELF Object --------
$(BUILD_KERNEL)/obj/%.asm.o: $(SRC_KERNEL)/%.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@ -g -F dwarf

# -------- Link Kernel -> Raw binary --------

$(KERNEL_ELF): $(KERNEL_OBJ_SRC)
	@mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) -o $@ $^ -lgcc

$(KERNEL): $(KERNEL_ELF)
	i686-elf-objcopy -O binary $< $@

# -------- Library C -> ELF Object --------
$(BUILD_LIB)/obj/%.c.o: $(SRC_LIB)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(LIB_CFLAGS) -c $< -o $@ -g

# -------- Library ASM -> ELF Object --------
$(BUILD_LIB)/obj/%.asm.o: $(SRC_LIB)/%.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@ -g -F dwarf

# -------- Library Obj -> Archive --------
$(LIBRARY): $(LIB_OBJ_SRC)
	@mkdir -p $(dir $@)
	$(AR) $(ARFLAGS) $@ $^

lib: $(LIBRARY) # Build the library

# -------- Disk Image --------
$(IMAGE): $(BOOT) $(KERNEL)
	@mkdir -p $(dir $@)
# 2 to dev/null hides the progress clutter, remove if required for debug
	@echo "Creating disk image..."

# Fill in of os.img for 1MB * 1 = 1MB with zeroes from if /dev/zero
	dd if=/dev/zero of=$(IMAGE) bs=1M count=1 2>/dev/null

# Overwrite the image with the first.bin we created. dd truncates the rest of file
# after writing to avoid this we use notrunc
	dd if=$(BOOT) of=$(IMAGE) conv=notrunc 2>/dev/null

# Skip 1 block (seek) of 512 bytes (block size) and write kernel.bin
	dd if=$(KERNEL) of=$(IMAGE) bs=512 seek=1 conv=notrunc 2>/dev/null

	dd if=src/test_processes/process_1.bin of=$(IMAGE) bs=512 seek=100 conv=notrunc 2>/dev/null
	dd if=src/test_processes/process_2.bin of=$(IMAGE) bs=512 seek=101 conv=notrunc 2>/dev/null

# -------- Run --------
run: $(IMAGE)
# Consider os.img as a hard disk drive with raw format with no headers and such.
# It contains exactly the bytes written (raw format). i386 to emulate 32 bit x86.
	$(EMU) -m 512 -drive format=raw,file=$(IMAGE)

# -------- Debug (optional) --------
debug: $(IMAGE)
# -S pauses execution right at start, -s allows to open gdb in another window and 
# debug the machine by opening a backdoor like localhost:1234
	$(EMU) -m 512 -drive format=raw,file=$(IMAGE) -s -S

# -------- Clean --------
clean: clean_kernel clean_lib

clean_kernel:
	rm -rf $(BUILD_KERNEL)
clean_lib:
	rm -rf $(BUILD_LIB)
