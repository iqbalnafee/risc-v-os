#!/bin/bash
set -xue

QEMU=${QEMU:-qemu-system-riscv64}
CC=${CC:-clang}

CFLAGS="-std=gnu11 -O2 -g3 -Wall -Wextra \
  --target=riscv64-unknown-elf -march=rv64imac -mabi=lp64 \
  -mcmodel=medany -fno-pie -no-pie \
  -ffreestanding -fno-stack-protector -nostdlib"

# Build
$CC $CFLAGS -Wl,-Tkernel.ld -Wl,-Map=kernel.map -o kernel.elf kernel.c common.c

# Run (uses your installed RV64 OpenSBI)
exec "$QEMU" -machine virt -cpu rv64 \
  -nographic -serial stdio -monitor none \
  --no-reboot \
  -bios /usr/lib/riscv64-linux-gnu/opensbi/generic/fw_dynamic.bin \
  -kernel kernel.elf

