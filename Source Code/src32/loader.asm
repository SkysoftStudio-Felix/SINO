; BIOS services are confined to this real-mode loader.
bits 16
org 0
%define BASE 0x10000
start:
    cli
    mov ax, cs
    mov ds, ax
    xor ax, ax
    mov ss, ax
    mov sp, 0x7c00
    sti
    cld
    mov [drive], dl
    mov si, message
    call print
    ; Collect up to 32 E820 entries in low memory.
    xor ax, ax
    mov es, ax
    mov word [es:0x4f00], 0
    mov di, 0x5000
    xor ebx, ebx
.e820:
    mov eax, 0xe820
    mov edx, 0x534d4150
    mov ecx, 24
    mov dword [es:di+20], 1
    int 0x15
    jc .map_done
    cmp eax, 0x534d4150
    jne .map_done
    cmp ecx, 20
    jb .map_done
    inc word [es:0x4f00]
    add di, 24
    cmp word [es:0x4f00], 32
    jae .map_done
    test ebx, ebx
    jnz .e820
.map_done:
    cmp word [es:0x4f00], 0
    je failure
    ; Load the full floppy into RAM at 2 MB. Each track is first read
    ; into a low-memory bounce buffer, then copied using BIOS INT 15/87.
    mov word [disk_lba], 0
.track:
    mov byte [retries], 3
.retry:
    mov ax, 0x7000
    mov es, ax
    xor bx, bx
    cmp byte [drive], 0x80
    jb .chs
    ; Metadata lives in the first 64 KiB, not the bounce buffer segment.
    xor ax, ax
    mov es, ax
    movzx eax, word [disk_lba]
    add eax, [es:0x4f08]
    mov [dap_lba], eax
    mov si, dap
    mov dl, [drive]
    mov ah, 0x42
    int 0x13
    jmp .read_result
.chs:
    mov ax, [disk_lba]
    xor dx, dx
    mov cx, 18
    div cx
    xor dx, dx
    mov si, 2
    div si
    mov ch, al
    mov cl, 1
    mov dh, dl
    mov dl, [drive]
    mov ax, 0x0212
    int 0x13
.read_result:
    jnc .copy
    xor ax, ax
    mov dl, [drive]
    int 0x13
    dec byte [retries]
    jnz .retry
    jmp failure
.copy:
    movzx eax, word [disk_lba]
    shl eax, 9
    add eax, 0x200000
    mov [move_table+24+2], ax
    shr eax, 16
    mov [move_table+24+4], al
    push cs
    pop es
    mov si, move_table
    mov cx, 4608
    mov ax, 0x8700
    int 0x15
    jc failure
    push cs
    pop ds
    add word [disk_lba], 18
    cmp word [disk_lba], 2880
    jb .track
    ; Save the 8x16 ROM font for early diagnostics; the desktop loads OFL fonts.
    mov ax, 0x1130
    mov bh, 6
    int 0x10
    push ds
    mov ax, es
    mov ds, ax
    mov si, bp
    xor ax, ax
    mov es, ax
    mov di, 0x6000
    mov cx, 4096
    rep movsb
    pop ds
    ; Ask the supplied VBE BIOS for 1024x768, 32-bit direct color.
    xor ax, ax
    mov es, ax
    mov di, 0x5800
    mov ax, 0x4f01
    mov cx, 0x144
    int 0x10
    cmp ax, 0x004f
    jne failure
    test word [es:0x5800], 0x80
    jz failure
    cmp byte [es:0x5819], 32
    jne failure
    mov ax, 0x4f02
    mov bx, 0x4144
    int 0x10
    cmp ax, 0x004f
    jne failure
    xor ax, ax
    mov es, ax
    mov word [es:0x4f04], 0x144
    mov byte [es:0x4f06], 16
    ; A20, flat GDT, then a 32-bit far jump into the native kernel.
    cli
    in al, 0x92
    or al, 2
    and al, 0xfe
    out 0x92, al
    lgdt [gdt_pointer]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp dword 0x08:0x11000
failure:
    push cs
    pop ds
    mov si, failed
    call print
    cli
.halt:
    hlt
    jmp .halt
print:
    lodsb
    test al, al
    jz .done
    out 0xe9, al
    mov ah, 0x0e
    mov bx, 7
    int 0x10
    jmp print
.done:
    ret
align 8
gdt:
    dq 0
    dq 0x00cf9a000000ffff     ; 4 GB, D=1, kernel code
    dq 0x00cf92000000ffff     ; 4 GB, kernel data
gdt_end:
gdt_pointer:
    dw gdt_end-gdt-1
    dd BASE+gdt
align 8
move_table:
    dq 0, 0
    dw 0xffff, 0
    db 7, 0x93, 0, 0       ; source 00070000
    dw 0xffff, 0
    db 0x20, 0x93, 0, 0    ; destination 00200000, advanced per track
    dq 0, 0
drive db 0
retries db 0
disk_lba dw 0
message db 'Loading SINO 0.10 32-bit kernel and SINO SH...', 13, 10, 0
failed db 'SINO loader failed: disk or memory map unavailable.', 13, 10, 0
align 4
dap db 16,0
dw 18,0,0x7000
dap_lba dd 0,0
times 4096-($-$$) db 0
