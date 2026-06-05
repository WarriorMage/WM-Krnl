global sys_gate

sys_gate:
    push ebp
    mov ebp, esp

    push ebx
    push esi
    push edi

    mov eax, [ebp + 8]
    mov ebx, [ebp + 12]
    mov ecx, [ebp + 16]
    mov edx, [ebp + 20] ; its ok to just set all 6 registers irrespective of 
    mov esi, [ebp + 24] ; number of arguments, final handler will only use what is
    mov edi, [ebp + 28] ; required.
    mov ebp, [ebp + 32] ; ebp lost but ok we won't need it after this
    int 0x80

    pop edi
    pop esi
    pop ebx

    pop ebp ; don't use leave, esp is already correct and ebp is modified
    ret
