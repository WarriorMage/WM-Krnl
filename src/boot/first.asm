;   SPDX-License-Identifier: GPL-3.0-or-later

BITS 16
ORG 0x7C00

code_segment equ 0x0000
vbr_address equ 0x7C00

cli
cld

xor ax, ax
mov ds, ax
mov es, ax
mov ss, ax
mov esp, 0x7C00

mov [boot_drive], dl

read_partition_table:
    mov cl, 4
    mov bx, partition_table
    read_loop:
        cmp byte [bx], 0x80    ; only consider the first active entry
        jz copy_mbr
        add bx, 16
        dec cl
        jnz read_loop
    jmp fail

selected_partition dw 0

copy_mbr:
    mov [selected_partition], bx
    add word [selected_partition], 512
    mov si, 0x7C00
    mov di, 0x7E00
    mov cx, 256
    rep movsw
    jmp code_segment:(load_vbr + 512)

load_vbr:
    mov si, [selected_partition + 512]
    mov eax, [si + 8]
    mov [dap + 512 + 8], eax
    mov ah, 0x42    ; Extended read sectors (0x42)
    mov dl, [boot_drive + 512]
    mov si, dap + 512
    int 0x13    ; BIOS interrupt
    jc (disk_error + 512)

mov dl, [boot_drive + 512]
mov si, [selected_partition + 512]
jmp code_segment:vbr_address

disk_error:
fail:
    jmp $

boot_drive db 0

align 4
dap:
    db 0x10        ; 0  Size of packet (16 bytes)
    db 0           ; 1  Reserved (must be 0)
    dw 1         ; 2  Number of sectors to read
    dw 0x7C00      ; 4  Buffer offset
    dw 0x0000    ; 6  Buffer segment
    dq 0         ; 8  64-bit starting LBA

times 446 - ($ - $$) db 0
partition_table times 64 db 0
dw 0xAA55
