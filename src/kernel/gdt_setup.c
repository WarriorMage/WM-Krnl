#include <stdint.h>
#include "gdt_setup.h"

typedef struct gdt_entry
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access_byte;
    uint8_t limit_high_and_flags;
    uint8_t base_high;
} __attribute__((packed)) gdt_entry;

gdt_entry global_descriptor_table[4];

// TSS is used for variety of purposes by hardware but we don't need it so we don't
// care about 96 out 104 bytes so we pad them with zeroes.
typedef struct tss_layout
{
    uint32_t padding;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t unused[23];
} __attribute__((packed)) tss_layout;

tss_layout task_state_segment = {0, 0xFF000000, 0x10, {0}};

void switch_kernel_stack(uint32_t new_kstack)
{
    task_state_segment.esp0 = new_kstack;
}

void fill_gdt_entry(uint8_t entry_number, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags)
{
    // The mask not really required since the upper 16 bits are discarded.
    global_descriptor_table[entry_number].base_low = base & 0xFFFF;
    // Masking still done to clarify the intent.
    global_descriptor_table[entry_number].base_mid = (base >> 16) & 0xFF;
    global_descriptor_table[entry_number].base_high = (base >> 24) & 0xFF;
    global_descriptor_table[entry_number].limit_low = limit & 0xFFFF;
    global_descriptor_table[entry_number].limit_high_and_flags = (flags << 4) | ((limit >> 16) & 0x0F);
    global_descriptor_table[entry_number].access_byte = access;
}

void setup_gdt(void)
{
    // Required for whatever purpose null descriptor
    global_descriptor_table[0] = (gdt_entry){0};
    // Kernel code segment
    fill_gdt_entry(1, 0, 0xFFFFF, 0x9A, 0x0C);
    // Kernel data segment
    fill_gdt_entry(2, 0, 0xFFFFF, 0x92, 0x0C);
    // Task state segment, 9 in 0x89 = 1001: Available 32 bit TSS (11 for busy)
    // Those 4 access bytes interpreted differently in system segments like TSS.
    fill_gdt_entry(3, (uint32_t)&task_state_segment, sizeof(tss_layout) - 1, 0x89, 0x00);
}

typedef struct gdtr
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) gdtr;

gdtr gdtr_descriptor;

void setup_gdtr(void)
{
    gdtr_descriptor.base = (uint32_t)global_descriptor_table;
    gdtr_descriptor.limit = sizeof(global_descriptor_table) - 1;
    load_gdtr();
    // TSS required to switch from user to kernel stack in privilege transition.
    load_tss();
}
