MEMORY {
  FLASH (RX): origin=0x00000000, length=0x0001F000
  SRAM (RWX): origin=0x20200000, length=0x00008000
}
SECTIONS {
 .text: {} > FLASH
 .const: {} > FLASH
 .rodata: {} > FLASH
 .ARM.exidx: {} > FLASH
 .data: {} > SRAM
 .bss: {} > SRAM
 .stack: {} > SRAM (HIGH)
 .sysmem: {} > SRAM
}
