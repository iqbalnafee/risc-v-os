/* RV64 payload at 0x80200000 (away from OpenSBI at ~0x80000000) */
OUTPUT_ARCH(riscv)
ENTRY(kernel_entry)   /* <- small stub defined in C below */

SECTIONS {
  . = 0x80200000;

  .text : ALIGN(4) {
    KEEP(*(.text.boot))      /* where the stub will live */
    *(.text .text.*)
  }

  .rodata : ALIGN(8) {
    *(.rodata .rodata.*)
    *(.srodata .srodata.*)
  }

  .data : ALIGN(8) {
    *(.data .data.*)
    *(.sdata .sdata.*)
  }

  .bss : ALIGN(16) {
    __bss = .;
    *(.bss .bss.*)
    *(.sbss .sbss.*)
    *(COMMON)
    __bss_end = .;
  }

  /* Simple kernel stack; expose its top for the stub */
  . = ALIGN(16);
  . += 128 * 1024;          /* 128 KiB */
  __stack_top = .;
  
  
  /*the memory regions to be managed by the allocator*/
  . = ALIGN(4096);
    __free_ram = .;
    . += 64 * 1024 * 1024; /* 64MB */
    __free_ram_end = .;
  
}

