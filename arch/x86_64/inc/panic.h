#ifndef PANIC_H
#define PANIC_H

namespace Kernel {
    namespace Debug {
        void __attribute__((noreturn)) Panic(const char* error);
    }
}

#endif