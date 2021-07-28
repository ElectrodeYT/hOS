#ifndef SYSCALL_H
#define SYSCALL_H

#include <interrupts.h>

namespace Kernel {

class SyscallHandler {
public:
    SyscallHandler() = default;

    static SyscallHandler& the() {
        static SyscallHandler instance;
        return instance;
    }

    void Init();
    void HandleSyscall(Interrupts::ISRRegisters* regs);
private:

};

}

#endif