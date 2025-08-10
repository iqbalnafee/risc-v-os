#include "kernel.h"
#include "common.h"

extern char __bss[], __bss_end[], __stack_top[];
/* entry stub: set SP then jump to kernel_main */
__attribute__((naked, section(".text.boot")))
void kernel_entry(void) {
    __asm__ __volatile__(
        "la sp, __stack_top\n"
        "j kernel_main\n"
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



void kernel_main(void) {
    printf("\n\nHello %s\n", "World!");
    printf("1 + 2 = %d, %x\n", 1 + 2, 0x1234abcd);

    for (;;) {
        __asm__ __volatile__("wfi"); // wait for interrupt
    }
    
    // panic
    /*memset(__bss, 0, (size_t) __bss_end - (size_t) __bss);

    PANIC("booted!");
    printf("unreachable here!\n");*/
}
