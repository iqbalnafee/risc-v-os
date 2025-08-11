#include "process.h"
#include "common.h"

struct process procs[PROCS_MAX];



static inline uintptr_t align_down(uintptr_t x, uintptr_t a) {
    return x & ~(a - 1);
}

struct process *create_process(uint64_t pc) {
    struct process *proc = NULL;
    for (int i = 0; i < PROCS_MAX; i++) {
        if (procs[i].state == PROC_UNUSED) { proc = &procs[i]; break; }
    }
    if (!proc) PANIC("no free process slots");

    uintptr_t top = (uintptr_t)proc->stack + sizeof(proc->stack);
    top = align_down(top, 16);

    // We need initial sp % 16 == 8 (so that after +104 it’s aligned).
    uintptr_t sp_addr = top - 8;  // now sp is 8 mod 16
    uint64_t *frame = (uint64_t *)sp_addr;

    // Layout expected by switch_context restore:
    //  [0]=ra(pc), [1]=s0, [2]=s1, ..., [12]=s11
    frame[0]  = pc;  // ra
    frame[1]  = 0;   // s0
    frame[2]  = 0;   // s1
    frame[3]  = 0;   // s2
    frame[4]  = 0;   // s3
    frame[5]  = 0;   // s4
    frame[6]  = 0;   // s5
    frame[7]  = 0;   // s6
    frame[8]  = 0;   // s7
    frame[9]  = 0;   // s8
    frame[10] = 0;   // s9
    frame[11] = 0;   // s10
    frame[12] = 0;   // s11

    proc->pid   = (int)(proc - procs) + 1;
    proc->state = PROC_RUNNABLE;
    proc->sp    = (uint64_t)(uintptr_t)frame;
    return proc;
}


__attribute__((noreturn))
void proc_a_entry(void) {
    for (;;) {
        putchar('A'); putchar('\n');
        for (volatile int i = 0; i < 100000; i++) {}
    }
}

__attribute__((noreturn))
void proc_b_entry(void) {
    for (;;) {
        putchar('B'); putchar('\n');
        for (volatile int i = 0; i < 100000; i++) {}
    }
}
