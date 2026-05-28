# SPDX-License-Identifier: GPL-3.0-or-later

# These are not real files, run these rules even if an up to date file in the same name exists.
.PHONY: all run debug clean

# -------- Tools --------
CC      = i686-elf-gcc
AS      = nasm
EMU		= qemu-system-i386

# -------- Directories --------
SRC_BOOT    = src/boot
SRC_KERNEL  = src/kernel

BUILD       = build
OBJ_DIR     = $(BUILD)/obj
BIN_DIR     = $(BUILD)/bin
IMG_DIR     = $(BUILD)/img

# -------- Flags --------
# -ffreestanding: Don't replace my code with libc code for optimizing, it doesn't exist
# -fno-stack-protector: Modern compilers protect against "Stack Smashing" by adding a random number to the stack. When a function returns, it checks if that number is still there. If it's gone, it calls an error-handling function. Disable it we don't have any error-handler, or the random number creator.
# -fno-pie: Use absolute addressing. Don't emit relative symbols in output, there
# is no loader to decide the addresses.
CFLAGS  = -ffreestanding -fno-pie -fno-stack-protector -O0 -Wall -Wextra

# Don't link the C runtime to setup argc, argv, entry point till main, we don't 
# have it. Also use the 32 bit linker with ELF input.
LDFLAGS = -T src/linker.ld -nostdlib
ASFLAGS = -f elf32

# -------- Files --------
BOOT    = $(BIN_DIR)/first.bin
KERNEL  = $(BIN_DIR)/kernel.bin
IMAGE   = $(IMG_DIR)/os.img

# -------- Sources ------
C_FILES   = kernel_entry.c keyboard.c interrupt_handler.c gdt_setup.c process.c sample_processes.c phy_allocator.c paging.c vir_allocator.c
ASM_FILES = interrupt_handler.asm io.asm gdt_setup.asm process.asm paging.asm

C_SRC   = $(addprefix $(SRC_KERNEL)/, $(C_FILES))
ASM_SRC = $(addprefix $(SRC_KERNEL)/, $(ASM_FILES))

# Suffix change syntax, .asm.o to avoid name collisions with x.c and x.asm
OBJ_C   = $(addprefix $(OBJ_DIR)/, $(C_FILES:.c=.o))
OBJ_ASM = $(addprefix $(OBJ_DIR)/, $(ASM_FILES:.asm=.asm.o))
OBJ_SRC = $(OBJ_C) $(OBJ_ASM)

# first dependency =  $<
# all dependencies = $^
# target =  $@

# Default target
all: run

# -------- Directory Targets --------
$(OBJ_DIR):
	mkdir -p $@
$(BIN_DIR):
	mkdir -p $@
$(IMG_DIR):
	mkdir -p $@

# Bootloader is a separate binary linked with nothing. It loads the kernel which is 
# a separate binary from the disk into memory. We directly jump from there so no
# need to link with the kernel.

# -------- Bootloader --------
$(BOOT): $(SRC_BOOT)/first.asm | $(BIN_DIR)
	$(AS) -f bin $< -o $@

# We first compile to ELF object instead of raw since they have useful metadata
# such as section information that the linker can use which a raw object lacks. We
# are anyways producing a raw binary after linking. Check the linker script.

# -------- Kernel C -> ELF Object --------
$(OBJ_DIR)/%.o: $(SRC_KERNEL)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@ -Wall -Wextra -g

# -------- Kernel ASM -> ELF Object --------
$(OBJ_DIR)/%.asm.o: $(SRC_KERNEL)/%.asm | $(OBJ_DIR)
	$(AS) $(ASFLAGS) $< -o $@

# -------- Link Kernel -> Raw binary --------
$(BIN_DIR)/kernel.elf: $(OBJ_SRC) | $(BIN_DIR)
	$(CC) $(LDFLAGS) -o $@ $^ -lgcc

$(KERNEL): $(BIN_DIR)/kernel.elf
	i686-elf-objcopy -O binary $< $@

# -------- Disk Image --------
$(IMAGE): $(BOOT) $(KERNEL) | $(IMG_DIR)
# 2 to dev/null hides the progress clutter, remove if required for debug
	@echo "Creating disk image..."

# Fill in of os.img for 1MB * 1 = 1MB with zeroes from if /dev/zero
	dd if=/dev/zero of=$(IMAGE) bs=1M count=1 2>/dev/null

# Overwrite the image with the first.bin we created. dd truncates the rest of file
# after writing to avoid this we use notrunc
	dd if=$(BOOT) of=$(IMAGE) conv=notrunc 2>/dev/null

# Skip 1 block (seek) of 512 bytes (block size) and write kernel.bin
	dd if=$(KERNEL) of=$(IMAGE) bs=512 seek=1 conv=notrunc 2>/dev/null

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
clean:
	rm -rf $(BUILD)
