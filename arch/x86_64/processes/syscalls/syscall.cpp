#include <mem.h>
#include <processes/syscalls/syscall.h>
#include <debug/klog.h>
#include <processes/scheduler.h>
#include <mem/PM/physalloc.h>
#include <mem/VM/virtmem.h>
#include <kernel-drivers/VFS.h>
#include <errno.h>
#include <hardware/instructions.h>

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
            KLog::the().userDebug(str);
            delete str;
            break;
        }
        // mmap
        case 2: {
            uint64_t actual;
            // uint64_t wanted_pointer = regs->rcx;
            regs->rax = mmap(this_proc, regs->rbx, &actual, regs->rcx, regs->rdx);
            // KLog::the().printf("syscalls: mmap: wanted pointer %x, wanted size %x, got pointer %x, got size %x\n\r", regs->rcx, regs->rbx, regs->rax, actual);
            if(!(regs->rax & (1UL << 63))) { regs->rbx = actual; }
            // KLog::the().printf("syscalls: rbx=%x\n\r", regs->rbx);
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
            // TODO: yeah we might still one somewhat accurate timer interrupts (links in with timer system rework)
            asm volatile ("cli");
            Processes::Scheduler::the().Schedule(regs);
            asm volatile ("sti");
            break;
        }
        // fork
        case 5: {
            KLog::the().printf("fork pid=%i\r\n", this_proc->pid);
            regs->rax = fork(regs);
            KLog::the().printf("fork new_pid=%i\r\n", regs->rax);
            break;
        }
        // open
        case 6: {
            // Copy the string out
            char* str = new char[regs->rcx + 1];
            if(!this_proc->attemptCopyFromUser(regs->rbx, regs->rcx, str)) {
                delete str;
                regs->rax = -EINVAL;
                break;
            }
            str[regs->rcx] = '\0';
            // KLog::the().printf("open path=%s, flags=%i\n\r", str, regs->rdx);
            regs->rax = open(str, regs->rdx, this_proc);
            break;
        }
        // close
        case 7: {
            // KLog::the().printf("close fd=%i\n\r", regs->rbx);
            regs->rax = close(regs->rbx, this_proc);
            break;
        }
        // read
        case 8: {
            // KLog::the().printf("read %x:%x fd=%i count=%i\n\r", regs->rbx, regs->rcx, regs->rdx, regs->rcx);
            // Create a kernel buffer to store data into
            char* buffer = new char[regs->rcx];
            regs->rax = read(regs->rdx, buffer, regs->rcx, this_proc);
            if(regs->rax > 0) {
                if(!this_proc->attemptCopyToUser(regs->rbx, regs->rcx, buffer)) { regs->rax = -EFAULT; }
            }
            delete buffer;
            break;
        }
        // write
        case 9: {
            // KLog::the().printf("write\n\r");
            // Create a kernel buffer to store data into
            char* buffer = new char[regs->rcx];
            if(!this_proc->attemptCopyFromUser(regs->rbx, regs->rcx, buffer)) { regs->rax = -EFAULT; break; }
            regs->rax = write(regs->rdx, buffer, regs->rcx, this_proc);
            break;
        }
        // seek
        case 10: {
            // KLog::the().printf("seek fd=%i, offset=%i, whence=%i\n\r", regs->rbx, regs->rcx, regs->rdx);
            regs->rax = seek(regs->rbx, regs->rcx, regs->rdx, this_proc);
            break;
        }
        // set tcb
        case 11: {
            // KLog::the().printf("set_tcb pointer=%x\n\r", regs->rbx);
            // Update FS base
            this_proc->threads.at(Processes::Scheduler::the().curr_thread)->tcb_base = regs->rbx;
            write_msr(0xC0000100, this_proc->threads.at(Processes::Scheduler::the().curr_thread)->tcb_base);
            break;
        }
        // istty
        case 12: {
            // KLog::the().printf("istty fd=%i\n\r", regs->rbx);
            regs->rax = isatty(regs->rbx, this_proc) ? 0 : -ENOTTY;
            break;
        }
        // exec
        case 13: {
            // Copy out the name
            char* file = new char[regs->rcx + 1];
            // TODO: make this not exploit hell
            if(!this_proc->attemptCopyFromUser(regs->rbx, regs->rcx, file)) { regs->rax = -EFAULT; break; }
            file[regs->rcx] = '\0';
            KLog::the().printf("exec file=%s argv=%x envp=%x\n\r", file, regs->rdx, regs->rdi);
            // Try to load the file
            int64_t elf_fd = VFS::the().open(this_proc->working_dir, file, this_proc->pid);
            if(elf_fd < 0) { regs->rax = elf_fd; break; }
            size_t elf_size = VFS::the().size(elf_fd, this_proc->pid);
            uint8_t* elf_data = new uint8_t[elf_size];
            int64_t elf_read_ret = VFS::the().pread(elf_fd, elf_data, elf_size, 0, this_proc->pid);
            if(elf_read_ret < 0) { regs->rax = elf_read_ret; break; } 
            VFS::the().close(elf_fd, this_proc->pid);
            // Calculate argc, envc
            size_t argc = 0;
            size_t envc = 0;
            for(char** argv = (char**)(regs->rdx); *argv && (uint64_t)(*argv) > 4096; argv++) { argc++; }
            for(char** envp = (char**)(regs->rdi); *envp; envp++) { envc++; }

            // We copy this out now
            char** argv = new char*[argc + 1];
            char** proc_argv = (char**)(regs->rdx);
            char** envp = new char*[envc + 1];
            char** proc_envp = (char**)(regs->rdi);
            
            for(size_t i = 0; i < argc; i++) {
                argv[i] = new char[strlen(proc_argv[i]) + 1];
                if(!this_proc->attemptCopyFromUser((uint64_t)(proc_argv[i]), strlen(proc_argv[i]), argv[i])) { regs->rax = -EFAULT; break; }
                argv[i][strlen(proc_argv[i])] = '\0';
            }
            argv[argc] = NULL;

            for(size_t i = 0; i < envc; i++) {
                envp[i] = new char[strlen(proc_envp[i]) + 1];
                if(!this_proc->attemptCopyFromUser((uint64_t)(proc_envp[i]), strlen(proc_envp[i]), envp[i])) { regs->rax = -EFAULT; break; }
                envp[i][strlen(proc_envp[i])] = '\0';
            }
            proc_envp[envc] = NULL;
            

            uint64_t ret = Processes::Scheduler::the().Exec(elf_data, elf_size, argv, argc, envp, envc, regs);
            if(ret != 0) { regs->rax = ret; }
            break;
        }
        default: KLog::the().printf("Got invalid syscall: %x\r\n", (uint64_t)regs->rax); regs->rax = -ENOSYS; break;
    }
}

uint64_t SyscallHandler::mmap(Processes::Process* process, uint64_t requested_size, uint64_t* actual_size, uint64_t requested_pointer, uint64_t flags) {
    uint64_t size = round_to_page_up(requested_size);
    uint64_t current_map = 0x6000000;
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

int64_t SyscallHandler::open(const char* path, int flags, Processes::Process* process) {
    int64_t global_fd = VFS::the().open(process->working_dir, path, process->pid);
    int64_t process_fd;
    if(global_fd >= 0) {
        // Create translation table
        process_fd = process->getLowestVFSInt();
        process->fd_translation_table->push_back(new Processes::Process::VFSTranslation(global_fd, process_fd));
    }
    return global_fd >= 0 ? process_fd : global_fd;
    (void)flags;
}

int64_t SyscallHandler::close(int64_t fd, Processes::Process* process) {
    Processes::Process::VFSTranslation* vfs_translation = process->getGlobalFd(fd);
    if(vfs_translation == NULL) { return -EBADF; }
    int ret = VFS::the().close(vfs_translation->global_fd, process->pid);
    if(ret != 0) { return ret; }
    process->deleteFd(vfs_translation);
    return 0;
}

int64_t SyscallHandler::read(int64_t fd, void* buf, size_t count, Processes::Process* process) {
    Processes::Process::VFSTranslation* vfs_translation = process->getGlobalFd(fd);
    if(vfs_translation == NULL) { return -EBADF; }
    int64_t ret = VFS::the().pread(vfs_translation->global_fd, buf, count, vfs_translation->pos, process->pid);
    if(ret > 0) { vfs_translation->pos += ret; }
    return ret;
}

int64_t SyscallHandler::write(int64_t fd, void* buf, size_t count, Processes::Process* process) {
    Processes::Process::VFSTranslation* vfs_translation = process->getGlobalFd(fd);
    if(vfs_translation == NULL) { return -EBADF; }
    int64_t ret = VFS::the().pwrite(vfs_translation->global_fd, buf, count, vfs_translation->pos, process->pid);
    if(ret > 0) { vfs_translation->pos += ret; }
    return ret;
}

int64_t SyscallHandler::seek(int64_t fd, size_t offset, int whence, Processes::Process* process) {
    Processes::Process::VFSTranslation* vfs_translation = process->getGlobalFd(fd);
    if(vfs_translation == NULL) { return -EBADF; }
    switch(whence) {
        // SEEK_SET
        case 3: {
            vfs_translation->pos = offset;
            break;
        }
        // SEEK_CUR
        case 1: {
            vfs_translation->pos += offset;
            break;
        }
        // SEEK_END
        case 2: {
            vfs_translation->pos = VFS::the().size(vfs_translation->global_fd, -1) - offset;
            break;
        }
        default: {
            return -EINVAL;
        }
    }
    return vfs_translation->pos;
    (void)process;
}

bool SyscallHandler::isatty(int64_t fd, Processes::Process* process) {
    Processes::Process::VFSTranslation* vfs_translation = process->getGlobalFd(fd);
    if(vfs_translation == NULL) { return -EBADF; }
    return VFS::the().isatty(vfs_translation->global_fd, process->pid);
}

}