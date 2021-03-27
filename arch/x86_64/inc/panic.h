#ifndef PANIC_H
#define PANIC_H

namespace Kernel {
    namespace Debug {
        void Panic(const char* error);
    }
}

#endif