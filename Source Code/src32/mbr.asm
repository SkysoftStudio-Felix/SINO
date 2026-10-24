; Relocating BIOS MBR, selecting the active primary partition via EDD.
bits 16
org 0x600
start:
    cli
    xor ax,ax
    mov ds,ax
    mov es,ax
    mov ss,ax
    mov sp,0x7c00
    cld
    mov si,0x7c00
    mov di,0x600
    mov cx,256
    rep movsw
    jmp 0:relocated
relocated:
    sti
    mov [drive],dl
    mov eax,[0x600+440]
    mov [0x4f10],eax
    mov si,0x600+446
    mov cx,4
.find:
    cmp byte [si],0x80
    je .load
    add si,16
    loop .find
    jmp failure
.load:
    mov eax,[si+8]
    mov [dap_lba],eax
    mov si,dap
    mov dl,[drive]
    mov ah,0x42
    int 0x13
    jc failure
    cmp word [0x7dfe],0xaa55
    jne failure
    mov dl,[drive]
    jmp 0:0x7c00
failure:
    mov si,message
.print:
    lodsb
    test al,al
    jz .stop
    out 0xe9,al
    mov ah,0x0e
    mov bx,7
    int 0x10
    jmp .print
.stop:
    cli
    hlt
    jmp .stop
drive db 0
message db 'SINO: no bootable system partition.',0
align 4
dap db 16,0
dw 1,0x7c00,0
dap_lba dd 0,0
times 446-($-$$) db 0
times 64 db 0
dw 0xaa55
