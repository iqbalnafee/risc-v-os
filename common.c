#include "common.h"

/* helpers */
static inline void print_char(int c) { putchar((char)c); }

static void print_str(const char *s) {
    if (!s) s = "(null)";
    while (*s) putchar(*s++);
}

static void print_uint_dec(unsigned v) {
    /* print decimal without recursion */
    unsigned div = 1;
    while (v / div > 9) div *= 10;
    do {
        print_char('0' + (v / div));
        v %= div;
        div /= 10;
    } while (div);
}

static void print_int_dec(int v) {
    unsigned m = (unsigned)v;
    if (v < 0) { print_char('-'); m = (unsigned)(-v); }
    print_uint_dec(m);
}

static void print_hex32(unsigned v) {
    for (int i = 7; i >= 0; --i) {
        unsigned nibble = (v >> (i * 4)) & 0xF;
        print_char("0123456789abcdef"[nibble]);
    }
}

/* tiny printf: supports %% %c %s %d %x */
void printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    while (*fmt) {
        if (*fmt != '%') { putchar(*fmt++); continue; }
        ++fmt;  /* skip '%' */
        switch (*fmt) {
            case '\0': print_char('%'); goto done;
            case '%':  print_char('%'); break;
            case 'c':  print_char(va_arg(ap, int)); break;
            case 's':  print_str(va_arg(ap, const char*)); break;
            case 'd':  print_int_dec(va_arg(ap, int)); break;
            case 'x':  print_hex32(va_arg(ap, unsigned)); break;
            default:   /* unknown specifier: print literally */
                       print_char('%'); print_char(*fmt); break;
        }
        ++fmt;
    }

done:
    va_end(ap);
}


void *memset(void *buf, char c, size_t n) {
    uint8_t *p = (uint8_t *) buf;
    while (n--)
        *p++ = c;
    return buf;
}

char *strcpy(char *dst, const char *src) {
    char *d = dst;
    while (*src)
        *d++ = *src++;
    *d = '\0';
    return dst;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        if (*s1 != *s2)
            break;
        s1++;
        s2++;
    }

    return *(unsigned char *)s1 - *(unsigned char *)s2;
}



struct process procs[PROCS_MAX]; // All process control structures.

struct process *create_process(uint32_t pc) {
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
    uint32_t *sp = (uint32_t *) &proc->stack[sizeof(proc->stack)];
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
    *--sp = (uint32_t) pc;          // ra

    // Initialize fields.
    proc->pid = i + 1;
    proc->state = PROC_RUNNABLE;
    proc->sp = (uint32_t) sp;
    return proc;
}

