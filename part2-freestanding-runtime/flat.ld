/* Flat, contiguous layout for the freestanding blob, linked at a FIXED virtual
 * address (0x500000). The blob holds absolute pointers (Lua's internal function
 * tables), so it must be mapped at the address it was linked for. The non-PIE
 * target loads at fixed addresses, so 0x500000 is free and predictable. */
SECTIONS {
  . = 0x500000;
  .text   : { *(.text*) }
  .rodata : { *(.rodata*) }
  .data   : { *(.data*) }
  .bss    : { *(.bss*)  }
}
