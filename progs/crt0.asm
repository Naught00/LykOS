global _start
extern main
extern lykos_exit

_start:
    and rsp, -16
    call main

    mov rdi, rax
    xor rax, rax
    int 0x80
