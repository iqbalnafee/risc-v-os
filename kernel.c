#include "kernel.h"
#include "common.h"


extern char __bss[], __bss_end[], __stack_top[];
extern char __free_ram[], __free_ram_end[];


/* The naked attribute tells the compiler not to generate any other code than the inline assembly. */
__attribute__((naked, section(".text.boot")))
void kernel_entry(void) {
    __asm__ __volatile__(
        "la sp, __stack_top\n"
        "j kernel_main\n"
    );
}

// Save/restore callee-saved regs + ra, switch stacks.
// prev_sp: where to store the *current* sp
// next_sp: where to load the *next* sp from (must point to a stack
//          whose top has the 13 saved regs in the same layout).
__attribute__((naked))
void switch_context(uint64_t *prev_sp, uint64_t *next_sp) {
    __asm__ __volatile__(
        // Save callee-saved + ra onto *current* stack (13 x 8 bytes)
        "addi sp, sp, -13*8\n"
        "sd ra,   0*8(sp)\n"
        "sd s0,   1*8(sp)\n"
        "sd s1,   2*8(sp)\n"
        "sd s2,   3*8(sp)\n"
        "sd s3,   4*8(sp)\n"
        "sd s4,   5*8(sp)\n"
        "sd s5,   6*8(sp)\n"
        "sd s6,   7*8(sp)\n"
        "sd s7,   8*8(sp)\n"
        "sd s8,   9*8(sp)\n"
        "sd s9,  10*8(sp)\n"
        "sd s10, 11*8(sp)\n"
        "sd s11, 12*8(sp)\n"

        // *prev_sp = sp;   (a0 points to prev_sp)
        "sd sp, 0(a0)\n"

        // sp = *next_sp;   (a1 points to next_sp)
        "ld sp, 0(a1)\n"

        // Restore callee-saved + ra from the *next* stack (same layout)
        "ld ra,   0*8(sp)\n"
        "ld s0,   1*8(sp)\n"
        "ld s1,   2*8(sp)\n"
        "ld s2,   3*8(sp)\n"
        "ld s3,   4*8(sp)\n"
        "ld s4,   5*8(sp)\n"
        "ld s5,   6*8(sp)\n"
        "ld s6,   7*8(sp)\n"
        "ld s7,   8*8(sp)\n"
        "ld s8,   9*8(sp)\n"
        "ld s9,  10*8(sp)\n"
        "ld s10, 11*8(sp)\n"
        "ld s11, 12*8(sp)\n"

        // Pop the save area and return into next context
        "addi sp, sp, 13*8\n"
        "ret\n"
    );
}


/* Minimal SBI call and console putchar (legacy: EID=1, FID=0) */
static inline struct sbiret sbi_call(long a0,long a1,long a2,long a3,
                                     long a4,long a5,long fid,long eid) {
    register long x0 __asm__("a0") = a0;
    register long x1 __asm__("a1") = a1;
    register long x2 __asm__("a2") = a2;
    register long x3 __asm__("a3") = a3;
    register long x4 __asm__("a4") = a4;
    register long x5 __asm__("a5") = a5;
    register long x6 __asm__("a6") = fid;
    register long x7 __asm__("a7") = eid;
    __asm__ __volatile__("ecall"
        : "+r"(x0), "+r"(x1)
        : "r"(x2), "r"(x3), "r"(x4), "r"(x5), "r"(x6), "r"(x7)
        : "memory");
    return (struct sbiret){ .error = x0, .value = x1 };
}

void putchar(char ch) {
    (void)sbi_call((long)(unsigned char)ch, 0,0,0,0,0, 0, 1);
}

paddr_t alloc_pages(uint32_t n) {
    static paddr_t next_paddr = (paddr_t) __free_ram;
    paddr_t paddr = next_paddr;
    next_paddr += n * PAGE_SIZE;

    if (next_paddr > (paddr_t) __free_ram_end)
        PANIC("out of memory");

    memset((void *) paddr, 0, n * PAGE_SIZE);
    return paddr;
}


struct process procs[PROCS_MAX]; // All process control structures.

struct process *create_process(uint64_t pc) {
    // Find an unused process control structure.
    struct process *proc = NULL;
    int i;
    for (i = 0; i < PROCS_MAX; i++) {
        if (procs[i].state == PROC_UNUSED) {
            proc = &procs[i];
            break;
        }
    }

    if (!proc)
        PANIC("no free process slots");

    // Stack callee-saved registers. These register values will be restored in
    // the first context switch in switch_context.
    uint64_t *sp = (uint64_t *) &proc->stack[sizeof(proc->stack)];
    *--sp = 0;                      // s11
    *--sp = 0;                      // s10
    *--sp = 0;                      // s9
    *--sp = 0;                      // s8
    *--sp = 0;                      // s7
    *--sp = 0;                      // s6
    *--sp = 0;                      // s5
    *--sp = 0;                      // s4
    *--sp = 0;                      // s3
    *--sp = 0;                      // s2
    *--sp = 0;                      // s1
    *--sp = 0;                      // s0
    *--sp = (uint64_t) pc;          // ra

    // Initialize fields.
    proc->pid = i + 1;
    proc->state = PROC_RUNNABLE;
    proc->sp = (uint64_t) sp;
    return proc;
}

void delay(void) {
    for (int i = 0; i < 30000000; i++)
        __asm__ __volatile__("nop"); // do nothing
}

struct process *proc_a;
struct process *proc_b;


struct process *current_proc; // Currently running process
struct process *idle_proc;    // Idle process

/*The word "yield" is often used as the name for an API which allows giving up the CPU to another process voluntarily.*/
void yield(void) {
    // Search for a runnable process
    struct process *next = idle_proc;
    for (int i = 0; i < PROCS_MAX; i++) {
        struct process *proc = &procs[(current_proc->pid + i) % PROCS_MAX];
        if (proc->state == PROC_RUNNABLE && proc->pid > 0) {
            next = proc;
            break;
        }
    }

    // If there's no runnable process other than the current one, return and continue processing
    if (next == current_proc)
        return;

    // Context switch
    struct process *prev = current_proc;
    current_proc = next;
    switch_context(&prev->sp, &next->sp);
}

void proc_a_entry(void) {
    printf("starting process A\n");
    while (1) {
        putchar('A');
        yield();
    }
}

void proc_b_entry(void) {
    printf("starting process B\n");
    while (1) {
        putchar('B');
        yield();
    }
}

void kernel_main(void) {
    memset(__bss, 0, (size_t) __bss_end - (size_t) __bss);

    printf("\n\n");

    WRITE_CSR(stvec, (uint64_t) kernel_entry);

    idle_proc = create_process((uint64_t) NULL);
    idle_proc->pid = 0; // idle
    current_proc = idle_proc;

    proc_a = create_process((uint64_t) proc_a_entry);
    proc_b = create_process((uint64_t) proc_b_entry);

    yield();
    PANIC("switched to idle process");
}

