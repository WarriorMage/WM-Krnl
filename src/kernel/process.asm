section .text

extern setup_new_stack
extern context_switch_asm
extern switch_cr3

switch_cr3:
    mov eax, [esp + 4]
    mov cr3, eax
    ret

setup_new_stack:
    mov ecx, cr3
    ; New page directory address given as argument
    mov eax, [esp + 4]
    mov cr3, eax
    mov esp, 0xC0000000

    ; Push EIP, CS and EFLAGS to be popped by iretd
    push 0x202
    push 0x08
    push 0x00001000

    ; Push all the 8 GPRs to be later popped by popad
    times 8 push 0
    ; Restore the old directory
    mov cr3, ecx
    ret

extern after_return_32

context_switch_asm:
    mov eax, [esp + 4]
    mov cr3, eax
    mov esp, [esp + 8]
    jmp after_return_32
