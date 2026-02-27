;   SPDX-License-Identifier: GPL-3.0-or-later

BITS 32

section .text

extern idtr_descriptor
extern interrupt_dispatcher

global load_idtr
global clear_interrupts
global set_interrupts
global inb
global outb
global isr_table

%macro ISR 1
isr_%1:
    pushad
    push %1
    call interrupt_dispatcher
    add esp, 4
    popad
    iretd
%endmacro

%assign i 0
%rep 256
    ISR i
%assign i i+1
%endrep

isr_table:
    %assign i 0
    %rep 256
        dd isr_%+i
    %assign i i+1
    %endrep

load_idtr:
    lidt[idtr_descriptor]
    ret

clear_interrupts:
    cli
    ret

set_interrupts:
    sti
    ret

inb:    ; uint8_t inb (uint16_t port);
    mov dx, [esp + 4]   ; Read the 16 bit port number
    in al, dx   ; return value in eax, only 8 bits so in al
    ret

outb:   ; void outb (uint16_t port, uint8_t value);
    mov al, [esp + 8]  ; Read the value at top as push right -> left
    mov dx, [esp + 4]   ; Read the 16 bit port number
    out dx, al
    ret
