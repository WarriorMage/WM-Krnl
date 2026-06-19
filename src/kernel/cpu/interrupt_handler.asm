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
    push ds
    push es
    push fs
    push gs
    pushad
    push esp    ; Pushes the original esp value before the push
    push %1
    call interrupt_dispatcher
    add esp, 2 * 4
    after_return_%1:
    popad
    pop gs
    pop fs
    pop es
    pop ds
    %if (%1 == 8) || (%1 >= 10 && %1 <= 14) || (%1 == 17)
        add esp, 4 ; CPU pushes an additional error code for these interrupt numbers
        ; pop it or else iretd will get corrupted.
    %endif
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
