;   SPDX-License-Identifier: GPL-3.0-or-later

BITS 32

section .text

extern idtr_descriptor

global dummy_handler
global load_idtr
global set_interrupts

dummy_handler:
    iret

load_idtr:
    lidt[idtr_descriptor]
    ret

set_interrupts:
    sti
    ret
