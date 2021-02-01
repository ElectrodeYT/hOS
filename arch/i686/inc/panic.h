#ifndef PANIC_H
#define PANIC_H


#define panic(a) Kernel::debug_puts("\n\rPANIC: "); Kernel::debug_puts(a); while(1) { asm volatile("cli; hlt;"); }
#define ASSERT(a) if(!(a)) panic("Assert failed");

#endif