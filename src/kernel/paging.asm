BITS 32

global init_paging
global load_fault_virtual_address
global return_page_directory
global switch_to_virtual_stack

extern kernel_main

init_paging:
    mov eax, [esp + 4]
    mov cr3, eax
    mov eax, cr0
    or eax, 0x8000_0000
    mov cr0, eax
    ret

switch_to_virtual_stack:
    mov esp, 0x900000
    jmp kernel_main

load_fault_virtual_address:
    mov eax, cr2
    ret

return_page_directory:
    mov eax, cr3
    ret
