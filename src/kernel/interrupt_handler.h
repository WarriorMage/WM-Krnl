#pragma once

void remap_pic(void);
void setup_idt(void);
void setup_idtr(void);
void load_idtr(void);
void clear_interrupts(void);
void set_interrupts(void);