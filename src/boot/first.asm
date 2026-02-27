;   SPDX-License-Identifier: GPL-3.0-or-later

BITS 16
ORG 0x7C00

code_segment equ 0x08
kernel_address equ 0x10000

cli

xor ax, ax
mov ds, ax
mov es, ax
mov ss, ax
mov esp, 0x7C00

mov [boot_drive], dl
lgdt [gdtr_descriptor]

enable_a20:
    in al, 0x92
    or al, 0b10
    out 0x92, al

load_kernel:
    mov ah, 0x42
    mov dl, [boot_drive]
    mov si, dap
    int 0x13
    jc disk_error

switch_to_protected:
    mov eax, cr0
    or eax, 1
    mov cr0, eax

jmp code_segment:protected_mode_entry

BITS 32

protected_mode_entry:

mov ax, 0x10
mov ds, ax
mov es, ax
mov fs, ax 
mov gs, ax
mov ss, ax

mov esp, 0x90000
cld

jmp kernel_address

disk_error:
    jmp $

boot_drive db 0

gdt:
    null_descriptor        dq 0x00_00_00_00_00_00_00_00
    kernel_code_descriptor dq 0x00_CF_9A_00_00_00_FF_FF
    kernel_data_descriptor dq 0x00_CF_92_00_00_00_FF_FF

gdt_size equ ($ - gdt)

gdtr_descriptor:
    limit dw gdt_size - 1
    base dd gdt

dap:
    db 0x10        ; 0  Size of packet (16 bytes)
    db 0           ; 1  Reserved (must be 0)
    dw 20          ; 2  Number of sectors to read
    dw 0x0000      ; 4  Buffer offset
    dw 0x1000     ; 6  Buffer segment
    dq 1         ; 8  64-bit starting LBA

times 510 - ($ - $$) db 0
dw 0xAA55
