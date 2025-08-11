#pragma once
#include "common.h"

enum proc_state {
  PROC_UNUSED = 0,
  PROC_RUNNABLE,
};

#define PROCS_MAX 8
#define PROCESS_STACK_SIZE (8 * 1024)  /* 8 KiB */

struct process {
  int       pid;
  int       state;              /* PROC_UNUSED / PROC_RUNNABLE */
  uint64_t  sp;                 /* saved stack pointer (RV64) */
  __attribute__((aligned(16)))
  uint8_t   stack[PROCESS_STACK_SIZE];
};

extern struct process procs[PROCS_MAX];

struct process *create_process(uint64_t pc);

