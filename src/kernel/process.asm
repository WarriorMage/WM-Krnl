section .text

extern setup_new_stack
extern context_switch_asm

setup_new_stack:
    mov ecx, esp
    mov edx, [esp + 8]
    ; New stack top given as argument
    mov esp, [esp + 4]

    ; Push EIP, CS and EFLAGS to be popped by iretd
    push 0x202
    push 0x08
    push edx

    ; Push all the 8 GPRs to be later popped by popad
    times 8 push 0
    mov eax, esp
    ; Restore the old stack
    mov esp, ecx
    ret

extern after_return_32

context_switch_asm:
    mov esp, [esp + 4]
    jmp after_return_32
