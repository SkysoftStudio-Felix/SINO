; Position-independent, native 32-bit application with INT 80h syscalls.
bits 32
org 0
db 'FX32'
dd entry
dd 0, 0
entry:
    call .position
.position:
    pop ebx
    add ebx, message-.position
    mov edx, message_end-message
    mov eax, 1
    int 0x80
    xor eax, eax
    int 0x80
    ud2
message db 'Hello from a real 32-bit program!', 10
        db 'SINO native INT 80h works.', 10
        db 'Returned safely to SINO shell.', 10
message_end:
