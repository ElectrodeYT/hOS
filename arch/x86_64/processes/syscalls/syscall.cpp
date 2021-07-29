#include <mem.h>
#include <processes/syscalls/syscall.h>
#include <debug/serial.h>
#include <processes/scheduler.h>
#include <mem/PM/physalloc.h>
#include <mem/VM/virtmem.h>

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
            uint64_t size = round_to_page_up(regs->rbx);
            // Find the lowest starting from 0x400000 place to map this
            uint64_t current_map = 0x400000;
            for(size_t i = 0; i < this_proc->mappings.size(); i++) {
                // Check if this mapping would overlap with the current map
                // Check low end
                VM::Manager::VMObject* obj = this_proc->mappings.at(i);
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
                if(!phys) { regs->rax = ~1; return; }
                VM::Manager::the().MapPage(phys, current_map + curr, 0b111);
            }
            this_proc->mappings.push_back(new_obj);
            Debug::SerialPrintf("mmap syscall: requested %i, allocated %i, address %x\r\n", regs->rbx, size, current_map);
            regs->rax = current_map;
            regs->rbx = size;
            break;
        }
        case 3: {
            Debug::SerialPrintf("TODO: munmap\r\n");
            break;
        }
        default: Debug::SerialPrintf("Got invalid syscall: %x\r\n", (uint64_t)regs->rax); regs->rax = 0; break;
    }
}

}