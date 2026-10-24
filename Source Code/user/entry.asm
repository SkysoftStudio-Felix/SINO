bits 32
section .start
global _user_start
extern _main
_user_start:
    cld
    call _main
    mov ebx, eax
    mov eax, 0x100
    int 0x80
.stop: jmp .stop
