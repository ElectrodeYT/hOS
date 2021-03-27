#include <mem.h>
#include <kmain.h>
#include <panic.h>

namespace Kernel {
    void KernelMain() {
        Debug::Panic("test");
    }
}