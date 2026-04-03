section .text

global load_gdtr
global load_tss
extern gdtr_descriptor

load_gdtr:
    lgdt [gdtr_descriptor]
    ret

load_tss:
    ; Selector: index in gdt << 3 | TI (GDT/LDT) | RPL
    mov ax, 0x18
    ltr ax
    ret