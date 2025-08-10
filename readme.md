1. opensbi is a bios (initializes hardware, runs before your code) for risc-v architecture

# Linker script 
A linker script is a file which defines the memory layout of executable files. 
A linker script is a text file that tells the linker where to put things (like code, data, stack, etc.) in memory when creating an executable (like a .elf file).
It defines how your program is arranged in memory when it runs.

Why Do We Need It?

When you're writing a program (especially in embedded or OS development), you don’t just want to compile it, you want to control where in memory each part goes.

Example:
You may want:

    Your code (.text) to start at address 0x80000000

    Your global variables (.data) at 0x81000000

    Your stack at 0x82000000

Without a linker script, the linker chooses default locations. But if you're building an OS, bootloader, or bare-metal program, you often need precise control over memory layout.

SECTIONS {
  . = 0x80000000;           /* Start address */

  .text : { *(.text) }      /* Code goes here */
  .data : { *(.data) }      /* Initialized variables */
  .bss  : { *(.bss)  }      /* Uninitialized variables */
}

## when developing os, we are actually writing boot loader (Loads your OS kernel from storage/firmware) and kernel (actual operating system) not the bios.


## BIOS vs Bootloader vs Kernel – Who Does What?

| **Stage**          | **Who Runs It**         | **What It Does**                                     |
|--------------------|-------------------------|------------------------------------------------------|
| **BIOS / OpenSBI** | Firmware (*not you*)    | Initializes CPU, RAM, and starts the next stage     |
| **Bootloader**     | You (e.g. `boot.S`)     | Loads your kernel into memory and jumps to it       |
| **Kernel**         | You (`kernel.c`)        | Implements memory, scheduling, and system calls     |



RAM (grows upward in address space)
↑
| 0x80220000   ← __stack_top      ← Stack starts here
|              ← [128KB reserved for stack]
|              ← nothing above this unless heap added later
|
| 0x80200000   ← Kernel code, data, bss all below this line
↓

void *memset(...) 
Means:

    "This function returns a pointer to void — a generic pointer — not nothing!"

So it's not a function that returns nothing.
It's a function that returns a generic pointer (like void *p = malloc(10)).

The boot function has two special attributes. The __attribute__((naked)) attribute instructs the compiler not to generate unnecessary code before and after the function body, such as a return instruction. This ensures that the inline assembly code is the exact function body.

The boot function also has the __attribute__((section(".text.boot"))) attribute, which controls the placement of the function in the linker script. Since OpenSBI simply jumps to 0x80200000 without knowing the entry point, the boot function needs to be placed at 0x80200000.


one-liner to set up a fresh laptop for RV64 Hello World:

sudo apt-get update && \
sudo apt-get install -y qemu-system-misc opensbi clang lld














