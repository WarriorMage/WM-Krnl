section .text

global inb
global outb
global inw
global outw

inb:    ; uint8_t inb (uint16_t port);
    mov dx, [esp + 4]   ; Read the 16 bit port number
    in al, dx   ; return value in eax, only 8 bits so in al
    ret

outb:   ; void outb (uint16_t port, uint8_t value);
    mov al, [esp + 8]  ; Read the value at top as push right -> left
    mov dx, [esp + 4]   ; Read the 16 bit port number
    out dx, al
    ret

inw:
    mov dx, [esp + 4]   ; Read the 16 bit port number
    in ax, dx   ; return value in eax, only 16 bits so in ax
    ret

outw:   ; void outb (uint16_t port, uint16_t value);
    mov ax, [esp + 8]  ; Read the value at top as push right -> left
    mov dx, [esp + 4]   ; Read the 16 bit port number
    out dx, ax
    ret
    