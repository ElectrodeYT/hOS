#ifndef PANIC_H
#define PANIC_H

#define ASSERT(x, msg) if(!(x)) { Kernel::Debug::Panic("ASSERT FAILED: " msg); }

namespace Kernel {
    namespace Debug {
        void __attribute__((noreturn)) Panic(const char* error);
    }
}

#endif