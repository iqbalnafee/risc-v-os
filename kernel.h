#pragma once

/* Symbols provided by the linker script */
extern char __stack_top[];

/* Minimal SBI return type (optional for callers) */
struct sbiret { long error; long value; };

