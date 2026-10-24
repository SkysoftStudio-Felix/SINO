bits 16
org 0x7c00
jmp short boot
nop
db 'SINO    '
dw 512
db 1
dw 321                        ; 4 KB loader + up to 156 KB native kernel
db 2
dw 224
dw 2880
db 0xf0
dw 9
dw 18
dw 2
dd 0, 0
db 0, 0, 0x29
dd 0x46445833
db 'SINO       '
db 'FAT12   '
boot:
    jmp 0:normalized
normalized:
    cli
    xor ax, ax
    mov ds, ax
    mov ss, ax
    mov sp, 0x7c00
    sti
    cld
    mov [drive], dl
    mov [0x4f0c], dl
    cmp dl,0x80
    jae .metadata_ready
    mov dword [0x4f10],0
.metadata_ready:
    mov eax, [0x7c00+28]
    mov [0x4f08], eax
    mov ax, 0x1000
    mov es, ax
    xor bx, bx
    mov word [lba], 1
.load:
    mov byte [retry], 3
.try:
    cmp byte [drive], 0x80
    jb .chs
    mov [dap_offset], bx
    mov ax, es
    mov [dap_segment], ax
    movzx eax, word [lba]
    add eax, [0x4f08]
    mov [dap_lba], eax
    mov si, dap
    mov dl, [drive]
    mov ah, 0x42
    push bx
    int 0x13
    pop bx
    jmp .result
.chs:
    mov ax, [lba]
    xor dx, dx
    mov cx, 18
    div cx
    mov cl, dl
    inc cl
    xor dx, dx
    mov di, 2
    div di
    mov ch, al
    mov dh, dl
    mov dl, [drive]
    mov ax, 0x0201
    push bx
    int 0x13
    pop bx
.result:
    jnc .ok
    xor ax, ax
    mov dl, [drive]
    int 0x13
    dec byte [retry]
    jnz .try
    mov si, error
.print:
    lodsb
    test al, al
    jz .stop
    mov ah, 0x0e
    mov bx, 7
    int 0x10
    jmp .print
.stop:
    cli
    hlt
    jmp .stop
.ok:
    add bx, 512
    jnc .segment_ready
    mov ax, es
    add ax, 0x1000
    mov es, ax
.segment_ready:
    inc word [lba]
    cmp word [lba], 321
    jb .load
    mov dl, [drive]
    jmp 0x1000:0
drive db 0
retry db 0
lba dw 1
error db 'SINO: boot disk read failed.', 0
align 4
dap db 16,0
dw 1
dap_offset dw 0
dap_segment dw 0
dap_lba dd 0,0
times 510-($-$$) db 0
dw 0xaa55
