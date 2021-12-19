#ifndef SYSCALL_H
#define SYSCALL_H

#include <interrupts.h>
#include <processes/process.h>

namespace Kernel {

class SyscallHandler {
public:
    SyscallHandler() = default;

    static SyscallHandler& the() {
        static SyscallHandler instance;
        return instance;
    }

    enum Syscalls {
        SyscallDebugWrite = 1,
        SyscallMMap,
        SyscallMUnmap,
        SyscallExit,
        NumSyscalls
    };

    void Init();
    void HandleSyscall(Interrupts::ISRRegisters* regs);

    // Syscall implementations
    uint64_t mmap(Processes::Process* processes, uint64_t requested_size, uint64_t* actual_size, uint64_t requested_pointer = 0, uint64_t flags = 1);
    uint64_t munmap(Processes::Process* process);
    uint64_t iommap(Processes::Process* process, uint64_t requested_size, uint64_t phys_pointer);

    // These must be called from a interrupt context
    void debugWrite(Interrupts::ISRRegisters* regs, Processes::Process* process);
    void exit(Interrupts::ISRRegisters* regs, Processes::Process* process);
    int fork(Interrupts::ISRRegisters* regs); // Returns PID
private:

};

}

#endif