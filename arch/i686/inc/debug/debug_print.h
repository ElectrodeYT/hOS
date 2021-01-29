#ifndef DEBUG_PRINT_H
#define DEUBG_PRINT_H

// Debug print stuff.
// Very simple stuff, cause it doesnt need to be complex lol
namespace Kernel {

void debug_serial_init();

void debug_put(char c);
void debug_puts(const char* s);
void debug_puti(uint32_t i, int base = 10);

}
#endif