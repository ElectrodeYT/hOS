#include <stdint.h>
#include <stddef.h>
#include <stivale2.h>
#include <early-boot.h>
#include <mem.h>

// The following will be our kernel's entry point.
extern "C" void __attribute__((optimize("O0"))) _start(struct stivale2_struct *stivale2_struct) {
    // We are in a very limited enviroment right now, including the fact that the kernel heap doesnt work yet
    // (on purpose, to prevent undef. behaviour incase we accidentally call it)

    // Very first thing is we create the gdt  
    load_gdt();
    // We initialize the kernel internal heap as well, it is needed for initializing 
    __init_heap();

    // TODO: convert stivale memory stuff into a somewhat bootloader agnostic format
    

    // We're done, just hang...
    for (;;) {
        asm ("hlt");
    }
}