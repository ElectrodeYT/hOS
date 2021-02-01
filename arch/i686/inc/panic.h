#ifndef PANIC_H
#define PANIC_H


#define panic(a) debug_puts("PANIC: "); debug_puts(a); while(1) { asm volatile("cli; hlt;"); }
#define ASSERT(a) if(!(a)) panic("Assert failed");

#endif