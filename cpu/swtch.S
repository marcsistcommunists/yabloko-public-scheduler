.global swtch
swtch:
    mov 4(%esp), %eax  // oldstack ptr
    mov 8(%esp), %edx  // newstack ptr

    // Сохраняем только необходимые регистры (ABI)
    push %ebp
    push %ebx
    push %esi
    push %edi

    mov %esp, (%eax)   // Сохраняем текущий ESP
    mov %edx, %esp     // Переключаем стек

    pop %edi
    pop %esi
    pop %ebx
    pop %ebp

    ret