#include <mem.h>
#include <processes/syscalls/syscall.h>
#include <debug/klog.h>
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

// Syscalls are in syscalls.ods

void SyscallHandler::HandleSyscall(Interrupts::ISRRegisters* regs) {
    asm volatile("sti");
    Processes::Process* this_proc = Processes::Scheduler::the().CurrentProcess();
    switch(regs->rax) {
        case 1: {
            // Copy the string out
            char* str = new char[regs->rcx + 1];
            if(!this_proc->attemptCopyFromUser(regs->rbx, regs->rcx, str)) {
                delete str;
                regs->rax = -EINVAL;
                break;
            }
            // memcopy((void*)regs->rbx, str, regs->rcx);
            str[regs->rcx] = '\0';
            KLog::the().printf(str);
            delete str;
            break;
        }
        // mmap
        case 2: {
            uint64_t actual;
            regs->rax = mmap(this_proc, regs->rbx, &actual, regs->rcx, regs->rdx);
            if(regs->rax & (1UL << 63)) { regs->rbx = actual; }
            break;
        }
        // munmap
        case 3: {
            KLog::the().printf("TODO: munmap\r\n");
            regs->rax = -ENOSYS;
            break;
        }
        // exit
        case 4: {
            KLog::the().printf("Process %i exited with code %i\r\n", Processes::Scheduler::the().curr_proc, regs->rbx);
            Processes::Scheduler::the().KillCurrentProcess();
            // We dont want interrupts in the scheduler lol
            asm volatile ("cli");
            Processes::Scheduler::the().Schedule(regs);
            asm volatile ("sti");
            break;
        }
        // fork
        case 5: {
            KLog::the().printf("fork syscall: pid %i\r\n", this_proc->pid);
            regs->rax = fork(regs);
            break;
        }
        // ipchint
        case 6: {
            // KLog::the().printf("ipchint: pid %i, max size %x, string address %x, string length %x\r\n", this_proc->pid, regs->rbx, regs->rcx, regs->rdx);
            regs->rax = (uint64_t)Processes::Scheduler::the().IPCHint(regs->rbx, regs->rcx, regs->rdx);
            break;
        }
        // ipcsendpid
        case 7: {
            KLog::the().printf("ipcsendpid: TODO\r\n");
            regs->rax = -ENOSYS;
            break;
        }
        // ipcsendpipe
        case 8: {
           //  KLog::the().printf("ipcsendpipe: pid %i, buffer at %x, size %x, string at %x, string len %i\r\n", this_proc->pid, regs->rbx, regs->rcx, regs->rdx, regs->rdi);
            regs->rax = (uint64_t)Processes::Scheduler::the().IPCSendPipe(regs->rbx, regs->rcx, regs->rdx, regs->rdi);
            break;
        }
        // ipcrecv
        case 9: {
            // KLog::the().printf("ipcrecv: pid %i, buffer at %x, size %x %s", this_proc->pid, regs->rbx, regs->rcx, regs->rdx ? ", blocking\r\n" : "\r\n");
            regs->rax = (uint64_t)Processes::Scheduler::the().IPCRecv(regs->rbx, regs->rcx, regs->rdx, regs);
            break;
        }
        // iohint
        case 10: {
            KLog::the().printf("iohint: TODO\r\n");
            regs->rax = -ENOSYS;
            break;
        }
        // iowrite
        case 11: {
            KLog::the().printf("iowrite: TODO\r\n");
            regs->rax = -ENOSYS;
            break;
        }
        // ioread
        case 12: {
            KLog::the().printf("ioread: TODO\r\n");
            regs->rax = -ENOSYS;
            break;
        }
        // irqhint
        case 13: {
            KLog::the().printf("irqhint: TODO\r\n");
            regs->rax = -ENOSYS;
            break;
        }
        // iommap
        case 14: {
            regs->rax = iommap(this_proc, regs->rbx, regs->rcx);
            break;
        }
        default: KLog::the().printf("Got invalid syscall: %x\r\n", (uint64_t)regs->rax); regs->rax = -ENOSYS; break;
    }
}

uint64_t SyscallHandler::mmap(Processes::Process* process, uint64_t requested_size, uint64_t* actual_size, uint64_t requested_pointer, uint64_t flags) {
    uint64_t size = round_to_page_up(requested_size);
    uint64_t current_map = 0x400000;
    uint64_t req_pointer_page = requested_pointer & ~(0xFFF);
    if(req_pointer_page) { current_map = req_pointer_page; } // If a pointer was requested, then we force current_map to the page that was requested

    // Find the lowest starting from 0x400000 place to map this
    for(size_t i = 0; i < process->mappings.size(); i++) {
        // Check if this mapping would overlap with the current map
        // Check low end
        VM::VMObject* obj = process->mappings.at(i);
        if((MAX((obj->base + obj->size), (current_map + size)) - MIN(obj->base, current_map)) < ((obj->base + obj->size) - obj->base) + ((current_map + size) - current_map)) {
            if(req_pointer_page) { return -EINVAL; } // Check if this address works
            current_map = obj->base + obj->size;
        }
    }
    // Create new VM Object
    VM::VMObject* new_obj = new VM::VMObject(true, false);
    new_obj->base = current_map;
    new_obj->size = size;
    // Actually map the pages
    for(uint64_t curr = 0; curr < size; curr += 4096) {
        uint64_t phys = PM::AllocatePages(1);
        if(!phys) { return -EINVAL; }
        VM::MapPage(phys, current_map + curr, flags ? 0b111 : 0b101);
    }
    new_obj->write = flags != 0;
    new_obj->read = true;
    new_obj->execute = true; // lol
    process->mappings.push_back(new_obj);
    if(actual_size) { *actual_size = size; }
    return current_map;
}

int SyscallHandler::fork(Interrupts::ISRRegisters* regs) {
    return Processes::Scheduler::the().ForkCurrent(regs);
}

uint64_t SyscallHandler::iommap(Processes::Process* process, uint64_t requested_size, uint64_t phys_pointer) {
    // Check if the memory requested is actually a IO space (or, to be more accurate, not a memory space)
    uint64_t size = round_to_page_up(requested_size);
    phys_pointer &= ~(0xFFF);
    if(!PM::CheckIOSpace(phys_pointer, size)) { return -EINVAL; }
    // It is ok, we can map this
    uint64_t current_map = 0x400000;

    // Find the lowest starting from 0x400000 place to map this
    for(size_t i = 0; i < process->mappings.size(); i++) {
        // Check if this mapping would overlap with the current map
        // Check low end
        VM::VMObject* obj = process->mappings.at(i);
        if((MAX((obj->base + obj->size), (current_map + size)) - MIN(obj->base, current_map)) < ((obj->base + obj->size) - obj->base) + ((current_map + size) - current_map)) {
            current_map = obj->base + obj->size;
        }
    }
    // Since this doesnt require deallocation (and i can not be bothered to code saftey into this) we can just map this
    for(uint64_t curr = 0; curr < size; curr += 4096) {
        VM::MapPage(phys_pointer + curr, current_map + curr, 0b111);
    }
    return current_map;
}

}