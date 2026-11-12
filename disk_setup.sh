#!/bin/bash
set -e

IMAGE=build/kernel/img/os.img
MBR=build/boot/bin/first.bin
VBR=build/boot/bin/second.bin
KERNEL=build/kernel/bin/kernel.bin

mkdir -p "$(dirname "$IMAGE")"

echo "Creating disk image..."

# Initialize 1 MiB disk image
dd if=/dev/zero of="$IMAGE" bs=1M count=1 2>/dev/null

# Calculate partition size
kernel_bytes=$(stat -c%s "$KERNEL")
kernel_sectors=$(( (kernel_bytes + 511) / 512 ))
boot_partition_sectors=$(( kernel_sectors + 1 ))

# Create MBR partition table
sfdisk "$IMAGE" <<EOF
label: dos
unit: sectors

start=1, size=$boot_partition_sectors, type=0xDA, bootable
EOF

# Write MBR boot code (preserve partition table)
dd if="$MBR" of="$IMAGE" bs=1 count=446 conv=notrunc 2>/dev/null

# Write VBR
dd if="$VBR" of="$IMAGE" bs=512 seek=1 conv=notrunc 2>/dev/null

# Write kernel
dd if="$KERNEL" of="$IMAGE" bs=512 seek=2 conv=notrunc 2>/dev/null

# Write test processes
dd if=src/test_processes/process_1.bin of="$IMAGE" bs=512 seek=100 conv=notrunc 2>/dev/null
dd if=src/test_processes/process_2.bin of="$IMAGE" bs=512 seek=101 conv=notrunc 2>/dev/null

echo "Disk image created."
