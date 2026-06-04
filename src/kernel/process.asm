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
    mov edx, esp
    mov cr3, eax
    mov esp, 0xFFC00000

    ; Push EIP, CS and EFLAGS, SS, ESP to be popped by iretd
    push 0x23
    push 0xC0000000
    push 0x202
    push 0x1B
    push 0x00001000

    ; segment registers to be popped
    times 4 push 0x23
    ; Push all the 8 GPRs to be later popped by popad
    times 8 push 0
    mov eax, esp ; return the new stack top
    ; Restore the old directory
    mov cr3, ecx
    mov esp, edx
    ret

extern after_return_32

context_switch_asm:
    mov eax, [esp + 4]
    mov esp, [esp + 8]
    mov cr3, eax
    jmp after_return_32
