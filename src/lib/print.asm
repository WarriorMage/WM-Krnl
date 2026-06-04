global print_buffer_to_vga

print_buffer_to_vga:
    push ebp
    mov ebp, esp

    push ebx
    push esi
    push edi

    mov eax, 4
    mov ebx, [ebp + 8]
    mov ecx, [ebp + 12]
    mov edx, [ebp + 16]
    mov esi, [ebp + 20]
    mov edi, [ebp + 24]
    int 0x80

    pop edi
    pop esi
    pop ebx

    leave
    ret
