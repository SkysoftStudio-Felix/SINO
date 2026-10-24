OUTPUT_FORMAT(pei-i386)
ENTRY(_user_start)
SECTIONS {
 . = 0x40000000;
 .text : { LONG(0x31584e53); LONG(1); LONG(0); LONG(0); LONG(0); LONG(0); LONG(0); LONG(0); *(.start) *(.text*) *(.rdata*) *(.rodata*) }
 . = ALIGN(4096);
 ___user_data = .;
 .data : { LONG(0); *(.data*) }
 .bss : { *(.bss*) *(COMMON) }
 ___user_end = .;
 /DISCARD/ : { *(.eh_frame*) *(.comment*) *(.drectve*) *(.idata*) *(.reloc*) }
}
