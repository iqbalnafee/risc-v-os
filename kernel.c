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

void delay(void) {
    for (int i = 0; i < 30000000; i++)
        __asm__ __volatile__("nop"); // do nothing
}

struct process *proc_a;
struct process *proc_b;

void proc_a_entry(void) {
    printf("starting process A\n");
    while (1) {
        putchar('A');
        switch_context(&proc_a->sp, &proc_b->sp);
        delay();
    }
}

void proc_b_entry(void) {
    printf("starting process B\n");
    while (1) {
        putchar('B');
        switch_context(&proc_b->sp, &proc_a->sp);
        delay();
    }
}



void kernel_main(void) {
    // zero BSS
    memset(__bss, 0, (size_t)(__bss_end - __bss));

    
    WRITE_CSR(stvec, (uint32_t) kernel_entry);

    proc_a = create_process((uint32_t) proc_a_entry);
    proc_b = create_process((uint32_t) proc_b_entry);
    proc_a_entry();

    PANIC("unreachable here!");
}

