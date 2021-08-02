#include <mem.h>
#include <processes/syscalls/syscall.h>
#include <debug/serial.h>
#include <processes/scheduler.h>
#include <mem/PM/physalloc.h>
#include <mem/VM/virtmem.h>
#include <errno.h>

namespace Kernel {

void SyscallHandler::Init() {
    // TODO
}

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

// syscalls:
// 1: debugWrite
// 2: mmap
// 3: munmap (TODO)
// 4: exit

void SyscallHandler::HandleSyscall(Interrupts::ISRRegisters* regs) {
    asm volatile("sti");
    Processes::Process* this_proc = Processes::Scheduler::the().CurrentProcess();
    switch(regs->rax) {
        case 1: {
            // Copy the string out
            char* str = new char[regs->rcx + 1];
            if(!this_proc->attemptCopyFromUser(regs->rbx, regs->rcx, str)) {
                delete str;
                regs->rax = ~1;
                break;
            }
            // memcopy((void*)regs->rbx, str, regs->rcx);
            str[regs->rcx] = '\0';
            Debug::SerialPrint(str);
            delete str;
            break;
        }
        // mmap
        case 2: {
            uint64_t actual;
            regs->rax = mmap(this_proc, regs->rbx, &actual);
            if(regs->rax & (1UL << 63)) { regs->rbx = actual; }
            break;
        }
        // munmap
        case 3: {
            Debug::SerialPrintf("TODO: munmap\r\n");
            break;
        }
        // exit
        case 4: {
            Debug::SerialPrintf("Process %i exited with code %i\r\n", Processes::Scheduler::the().curr_proc, regs->rbx);
            Processes::Scheduler::the().KillCurrentProcess();
            // We dont want interrupts in the scheduler lol
            asm volatile ("cli");
            Processes::Scheduler::the().Schedule(regs);
            asm volatile ("sti");
            break;
        }
        // fork
        case 5: {
            Debug::SerialPrintf("fork syscall: pid %i\r\n", this_proc->pid);
            regs->rax = fork(regs);
            break;
        }
        // ipchint
        case 6: {
            // Debug::SerialPrintf("ipchint: pid %i, max size %x, string address %x, string length %x\r\n", this_proc->pid, regs->rbx, regs->rcx, regs->rdx);
            regs->rax = (uint64_t)Processes::Scheduler::the().IPCHint(regs->rbx, regs->rcx, regs->rdx);
            break;
        }
        // ipcsendpipe
        case 8: {
           //  Debug::SerialPrintf("ipcsendpipe: pid %i, buffer at %x, size %x, string at %x, string len %i\r\n", this_proc->pid, regs->rbx, regs->rcx, regs->rdx, regs->rdi);
            regs->rax = (uint64_t)Processes::Scheduler::the().IPCSendPipe(regs->rbx, regs->rcx, regs->rdx, regs->rdi);
            break;
        }
        // ipcrecv
        case 9: {
            // Debug::SerialPrintf("ipcrecv: pid %i, buffer at %x, size %x %s", this_proc->pid, regs->rbx, regs->rcx, regs->rdx ? ", blocking\r\n" : "\r\n");
            regs->rax = (uint64_t)Processes::Scheduler::the().IPCRecv(regs->rbx, regs->rcx, regs->rdx, regs);
            break;
        }
        default: Debug::SerialPrintf("Got invalid syscall: %x\r\n", (uint64_t)regs->rax); regs->rax = 0; break;
    }
}

uint64_t SyscallHandler::mmap(Processes::Process* processes, uint64_t requested_size, uint64_t* actual_size) {
    uint64_t size = round_to_page_up(requested_size);
    // Find the lowest starting from 0x400000 place to map this
    uint64_t current_map = 0x400000;
    for(size_t i = 0; i < processes->mappings.size(); i++) {
        // Check if this mapping would overlap with the current map
        // Check low end
        VM::Manager::VMObject* obj = processes->mappings.at(i);
        if((MAX((obj->base + obj->size), (current_map + size)) - MIN(obj->base, current_map)) < ((obj->base + obj->size) - obj->base) + ((current_map + size) - current_map)) {
            current_map = obj->base + obj->size;
        }
    }
    // Create new VM Object
    VM::Manager::VMObject* new_obj = new VM::Manager::VMObject(true, false);
    new_obj->base = current_map;
    new_obj->size = size;
    // Actually map the pages
    for(uint64_t curr = 0; curr < size; curr += 4096) {
        uint64_t phys = PM::Manager::the().AllocatePages(1);
        if(!phys) { return ~EINVAL; }
        VM::Manager::the().MapPage(phys, current_map + curr, 0b111);
    }
    processes->mappings.push_back(new_obj);
    if(actual_size) { *actual_size = size; }
    // Debug::SerialPrintf("mmap syscall: requested %i, allocated %i, address %x\r\n", requested_size, size, current_map);
    return current_map;
}

int SyscallHandler::fork(Interrupts::ISRRegisters* regs) {
    return Processes::Scheduler::the().ForkCurrent(regs);
}

}