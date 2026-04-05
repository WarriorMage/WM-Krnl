BITS 32

global init_paging

init_paging:
    mov eax, [esp + 4]
    mov cr3, eax
    mov eax, cr0
    or eax, 0x8000_0000
    mov cr0, eax
    ret
