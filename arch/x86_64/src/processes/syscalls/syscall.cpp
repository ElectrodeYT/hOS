#include <mem.h>
#include <processes/syscalls/syscall.h>
#include <debug/serial.h>

namespace Kernel {

void SyscallHandler::Init() {
    // TODO
}

// syscalls:
// 1: debugWrite
// 

void SyscallHandler::HandleSyscall(Interrupts::ISRRegisters* regs) {
    switch(regs->rax) {
        case 1: {
            // Copy the string out
            char* str = new char[regs->rcx + 1];
            memcopy((void*)regs->rbx, str, regs->rcx);
            str[regs->rcx] = '\0';
            Debug::SerialPrint(str);
            delete str;
            break;
        }
        default: Debug::SerialPrintf("Got invalid syscall: %x", (uint64_t)regs->rax); regs->rax = 0; break;
    }
}

}