bits 32
section .start
global _start
extern _kmain, ___bss_start, ___bss_end
_start:
    cli
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x80000
    cld
    xor eax, eax
    mov edi, ___bss_start
    mov ecx, ___bss_end
    sub ecx, edi
    rep stosb
    call _kmain
.stop:
    cli
    hlt
    jmp .stop

section .text
extern _irq_dispatch, _fault_resume, _fault_expected
%assign i 0
%rep 256
global _isr%+i
_isr%+i:
%if i != 8 && i != 10 && i != 11 && i != 12 && i != 13 && i != 14 && i != 17 && i != 21 && i != 29 && i != 30
    push dword 0
%endif
    push dword i
    jmp interrupt_common
%assign i i+1
%endrep
interrupt_common:
    pushad
    cld
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    push esp
    call _irq_dispatch
    add esp, 4
    cmp dword [_user_switch_out], 0
    jne user_leave
    test byte [esp+44], 3
    jz .kernel_segments
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
.kernel_segments:
    popad
    add esp, 8
    iretd

extern _user_switch_out
global _user_enter
_user_enter:
    pushfd
    push ebp
    push ebx
    push esi
    push edi
    mov [user_kernel_stack], esp
    mov esp, [esp+24]
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popad
    add esp, 8
    iretd
user_leave:
    mov dword [_user_switch_out], 0
    mov esp, [user_kernel_stack]
    pop edi
    pop esi
    pop ebx
    pop ebp
    popfd
    ret

global _run_native, _native_exit, _test_page_fault, _test_invalid_opcode
_run_native:
    push ebp
    push ebx
    push esi
    push edi
    mov [native_stack], esp
    mov eax, [esp+20]
    call eax
native_return:
    pop edi
    pop esi
    pop ebx
    pop ebp
    ret
_native_exit:
    mov esp, [native_stack]
    jmp native_return
_test_page_fault:
    mov dword [_fault_resume], .resume
    mov dword [_fault_expected], 14
    mov dword [0x40000000], 0xcafebabe
.resume:
    ret
_test_invalid_opcode:
    mov dword [_fault_resume], .resume
    mov dword [_fault_expected], 6
    ud2
.resume:
    ret
section .rdata
global _isr_table
_isr_table:
%assign i 0
%rep 256
    dd _isr%+i
%assign i i+1
%endrep
section .bss
align 4
native_stack resd 1
user_kernel_stack resd 1
