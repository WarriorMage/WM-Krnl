;   SPDX-License-Identifier: GPL-3.0-or-later

BITS 32

section .text

extern idtr_descriptor
extern interrupt_dispatcher

global load_idtr
global clear_interrupts
global set_interrupts
global isr_table

global after_return_32

%macro ISR 1
isr_%1:
    pushad
    mov eax, esp
    push eax
    push %1
    call interrupt_dispatcher
    add esp, 2 * 4
    after_return_%1:
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


