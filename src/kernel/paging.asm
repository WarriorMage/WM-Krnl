BITS 32

global init_paging
global load_fault_virtual_address
global return_page_directory
global switch_to_virtual_stack

extern kernel_cont
extern kernel_cont2

section .bootstrap

init_paging:
    mov eax, 0x180000
    mov cr3, eax
    mov eax, cr0
    or eax, 0x8000_0000
    mov cr0, eax
    add esp, 4
    jmp kernel_cont

section .text

switch_to_virtual_stack:
    mov esp, 0xF0000000
    jmp kernel_cont2

load_fault_virtual_address:
    mov eax, cr2
    ret

; returns the physical address of current page directory
return_page_directory:
    mov eax, cr3
    ret
