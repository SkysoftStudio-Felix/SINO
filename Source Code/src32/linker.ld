OUTPUT_FORMAT(pei-i386)
ENTRY(_start)
SECTIONS
{
  . = 0x11000;
  .text : { *(.start) *(.text*) }
  .rdata : { ___locale_literals_start = .; *(.rdata*) *(.rodata*) ___locale_literals_end = .; }
  .data : { *(.data*) }
  .bss 0x100000 : {
    ___bss_start = .;
    *(.bss*) *(COMMON)
    ___bss_end = .;
  }
  /DISCARD/ : { *(.eh_frame*) *(.comment*) *(.drectve*) *(.idata*) *(.reloc*) }
}
