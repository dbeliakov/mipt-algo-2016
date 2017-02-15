extern printf
global main

section .data
    msg db "Hello, world (%d)!", 10, 0

section .text
main:
    mov ecx, 5
print:
    push    ecx
    push    ebp
    mov     ebp,esp

    push ecx
    push msg
    call printf

    add esp, 8

    mov     esp, ebp
    pop     ebp
    pop ecx
    loop print

    mov eax, 0
    ret
