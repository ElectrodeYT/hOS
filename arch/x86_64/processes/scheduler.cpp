#include <mem.h>
#include <mem/PM/physalloc.h>
#include <mem/VM/virtmem.h>
#include <processes/scheduler.h>
#include <timer.h>
#include <debug/klog.h>
#include <panic.h>
#include <CPP/string.h>
#include <early-boot.h>
#include <processes/elf.h>
#include <hardware/instructions.h>
#include <processes/syscalls/syscall.h>
#include <errno.h>

namespace Kernel {
    namespace Processes {
        void Scheduler::Init() {
            // TODO
        }
        int64_t Scheduler::CreateProcessImpl(uint8_t* data, size_t length, char** argv, int argc, char** envp, int envc) {
            // Sanity checks
            if(!argv) { return -EINVAL; }
            if(!envp) { return -EINVAL; }
            if(argc < 0) { return -EINVAL; }
            if(envc < 0) { return -EINVAL; }
            // Calculate argv size
            size_t argv_size = 0;
            for(size_t i = 0; i < (size_t)argc; i++) {
                argv_size += strlen(argv[i]) + 1;
                argv_size += 8; // Pointer
            }
            if(argv_size > 0x1000) { return -E2BIG; }
            // Calculate envp size
            size_t envp_size = 0;
            for(size_t i = 0; i < (size_t)envc; i++) {
                envp_size += strlen(envp[i]) + 1;
                // Due to envp being hella weird, we actually have to pass a array of pointers
                // that point to strings, we need to take the pointer size into account
                envp_size += 8;
            } 
            envp_size += 8; // Terminator pointer
            if(envp_size > 0x1000) { return -E2BIG; }

            ELF elf(data, length);
            if(!elf.readHeader()) {
                KLog::the().printf("Failed to load ELF file for process %s\r\n", argv[0]);
                return -EINVAL;
            }

            // Create process
            Process* new_proc = new Process;
            new_proc->page_table = VM::Manager::the().CreateNewPageTable();
            new_proc->pid = GetNextPid();
            // Set the parent
            new_proc->parent = 0;
            // Create the string in it, and copy the given name over
            size_t name_len = strlen(argv[0]);
            new_proc->name = new char[name_len + 1];
            memcopy(argv[0], new_proc->name, name_len + 1);

            KLog::the().printf("Creating new process %s\r\n", new_proc->name);

            
            // We want to save the current page table, as we need to switch to it later
            uint64_t curr_page_table = VM::Manager::the().CurrentPageTable();
            // Switch into new page table
            VM::Manager::the().SwitchPageTables(new_proc->page_table);

            // We now need to loop through all the created segments, and see if we
            // need to make mappings for them
            for(size_t i = 0; i < elf.sections.size(); i++) {
                ELF::Section* section = elf.sections.at(i);
                if(section->loadable) {
                    // TODO: correct permissions
                    VM::Manager::VMObject* mapping = new VM::Manager::VMObject(true, false);
                    mapping->base = section->vaddr & ~(0xFFF);
                    mapping->size = round_to_page_up(section->segment_size);

                    KLog::the().printf("Mapping for ELF segment %i: base %x, size %x\r\n", i, mapping->base, mapping->size);

                    // Allocate it
                    size_t page_count = 0;
                    for(uint64_t i = 0; i < mapping->size; i += 4096) {
                        uint64_t phys = PM::Manager::the().AllocatePages();
                        // Map page
                        VM::Manager::the().MapPage(phys, mapping->base + i, 0b111);
                        page_count++;
                    }

                    // Now copy to it
                    section->copy_out((void*)section->vaddr);
                    new_proc->mappings.push_back(mapping);
                }
            }

            // Create main stack
            const uint64_t stack_size = 0x8000;
            uint64_t stack_base = SyscallHandler::the().mmap(new_proc, stack_size, NULL);

            // Copy argv
            uint64_t argv_base = SyscallHandler::the().mmap(new_proc, argv_size, &argv_size);
            uint64_t envp_base = SyscallHandler::the().mmap(new_proc, envp_size, &envp_size);
            

            // Copy argv
            uint64_t current_front = argv_base;
            uint64_t current_back = argv_base + argv_size;
            for(size_t i = 0; i < (size_t)argc; i++) {
                size_t string_len = strlen(argv[i]);
                current_back -= string_len + 1;
                memcopy(argv[i], (void*)current_back, string_len + 1);
                // Set pointer
                *(volatile uint64_t*)(current_front) = current_back;
                current_front += 8;
            }

            // Copy envp
            current_front = envp_base;
            current_back = envp_base + envp_size;
            for(size_t i = 0; i < (size_t)envc; i++) {
                size_t string_len = strlen(argv[i]);
                current_back -= string_len + 1;
                memcopy(argv[i], (void*)current_back, string_len + 1);
                // Set pointer
                *(volatile uint64_t*)(current_front) = current_back;
                current_front += 8;
            }
            // For envp we need to set the last pointer to 0 as well
            *(volatile uint64_t*)(current_front) = 0;

            // We put &argc and &argv on the stack
            // *(uint64_t* volatile)(stack_base + stack_size - 8) = argv_base;
            // *(uint64_t* volatile)(stack_base + stack_size - 16) = envp_base;

            // The sections have been copied out, we can now switch the page table back
            VM::Manager::the().SwitchPageTables(curr_page_table);

            // Initialize main thread
            Thread* main_thread = new Thread;
            main_thread->tid = 0;
            main_thread->regs.rip = elf.file_entry;
            main_thread->regs.rflags = 0x202;
            main_thread->regs.cs = 0x18 | 0b11;
            main_thread->regs.ss = 0x20 | 0b11;
            main_thread->blocked = Thread::BlockState::Running;
            main_thread->regs.page_table = new_proc->page_table;

            main_thread->stack_base = stack_base;
            main_thread->stack_size = stack_size;
            main_thread->regs.rsp = stack_base + stack_size; // -16 for the two pointers on the stack
            main_thread->regs.rdi = argc;
            main_thread->regs.rsi = argv_base;
            main_thread->regs.rdx = envp_base;
            main_thread->regs.rcx = envc;
            
            // Create syscall thread stack
            // TODO: guard pages
            VM::Manager::VMObject* syscall_stack = new VM::Manager::VMObject(true, false);
            syscall_stack->base = (uint64_t)VM::Manager::the().AllocatePages(4);
            syscall_stack->size = 4 * 4096;
            main_thread->syscall_stack_map = syscall_stack;



            // Attach thread to new process
            new_proc->threads.push_back(main_thread);

            // Add process
            processes.push_back(new_proc);
            return new_proc->pid;
        }

        int64_t Scheduler::CreateProcess(uint8_t* data, size_t length, const char* name) {
            // Create a fake envc, envp
            char* fake_env[1] = { NULL };
            // Create a argv containing only the program name
            char* argv[1] = { (char*)name };
            return CreateProcessImpl(data, length, argv, 1, fake_env, 0); // parent = UINT64_MAX; this will make CreateProcessImpl set parent to itself 
        }

        int64_t Scheduler::CreateProcess(uint8_t* data, size_t length, char** argv, int argc) {
            // We use the envc, envp of the parent
            // TODO: yeah that
            char* fake_env[1] = { NULL };
            return CreateProcessImpl(data, length, argv, argc, fake_env, 0);
        }

        int64_t Scheduler::ForkCurrent(Interrupts::ISRRegisters* regs) {
            // Create new process
            Processes::Process* new_proc = new Processes::Process();
            Processes::Process* curr_proc = CurrentProcess();
            
            // Copy the name over
            size_t name_len = strlen(curr_proc->name);
            new_proc->name = new char[name_len + 1];
            new_proc->pid = GetNextPid();
            memcopy(curr_proc->name, new_proc->name, name_len);

            // Copy the memory mappings over
            new_proc->page_table = VM::Manager::the().CreateNewPageTable();
            uint64_t current_table = VM::Manager::the().CurrentPageTable();
            for(size_t i = 0; i < curr_proc->mappings.size(); i++) {
                uint64_t* physical_addresses = new uint64_t[curr_proc->mappings.at(i)->size / 4096];
                VM::Manager::VMObject* new_cow = curr_proc->mappings.at(i)->copyAsCopyOnWrite(physical_addresses);
                // Now we need to switch to the new page table
                VM::Manager::the().SwitchPageTables(new_proc->page_table);
                // Now we need to map all the pages as read only
                for(size_t x = 0; x < new_cow->size; x += 4096) {
                    VM::Manager::the().MapPage(physical_addresses[x / 4096], new_cow->base + x, 0b101);
                }
                new_proc->mappings.push_back(new_cow);
                // Switch back
                VM::Manager::the().SwitchPageTables(current_table);
                delete physical_addresses;
            }

            // Create the new thread
            Thread* main_thread = new Thread();
            // Very fun code
            main_thread->regs.rax = 0;
            main_thread->regs.rbp = regs->rbp;
            main_thread->regs.rbx = regs->rbx;
            main_thread->regs.rcx = regs->rcx;
            main_thread->regs.rdi = regs->rdi;
            main_thread->regs.rdx = regs->rdx;
            main_thread->regs.rip = regs->rip;
            main_thread->regs.rsi = regs->rsi;
            main_thread->regs.rsp = regs->rsp;
            main_thread->regs.r8 = regs->r8;
            main_thread->regs.r9 = regs->r9;
            main_thread->regs.r10 = regs->r10;
            main_thread->regs.r11 = regs->r11;
            main_thread->regs.r12 = regs->r12;
            main_thread->regs.r13 = regs->r13;
            main_thread->regs.r14 = regs->r14;
            main_thread->regs.r15 = regs->r15;
            main_thread->regs.rflags = regs->rflags;
            main_thread->regs.cs = regs->cs;
            main_thread->regs.ss = regs->ss;
            main_thread->regs.page_table = new_proc->page_table;

            // Create syscall thread stack
            // TODO: guard pages
            VM::Manager::VMObject* syscall_stack = new VM::Manager::VMObject(true, false);
            syscall_stack->base = (uint64_t)VM::Manager::the().AllocatePages(4);
            syscall_stack->size = 4 * 4096;
            main_thread->syscall_stack_map = syscall_stack;

            new_proc->threads.push_back(main_thread);
            processes.push_back(new_proc);
            return new_proc->pid;
        }

        void Scheduler::SaveContext(Interrupts::ISRRegisters* regs) {
            if(running_proc_killed || running_thread_killed) { return; } // Dont try to save the context if the running process got killed
            Process* curr_p = processes.at(curr_proc);
            Thread* curr_t = curr_p->threads.at(curr_thread);

            // Very fun code
            curr_t->regs.rax = regs->rax;
            curr_t->regs.rbp = regs->rbp;
            curr_t->regs.rbx = regs->rbx;
            curr_t->regs.rcx = regs->rcx;
            curr_t->regs.rdi = regs->rdi;
            curr_t->regs.rdx = regs->rdx;
            curr_t->regs.rip = regs->rip;
            curr_t->regs.rsi = regs->rsi;
            curr_t->regs.rsp = regs->rsp;
            curr_t->regs.r8 = regs->r8;
            curr_t->regs.r9 = regs->r9;
            curr_t->regs.r10 = regs->r10;
            curr_t->regs.r11 = regs->r11;
            curr_t->regs.r12 = regs->r12;
            curr_t->regs.r13 = regs->r13;
            curr_t->regs.r14 = regs->r14;
            curr_t->regs.r15 = regs->r15;
            curr_t->regs.rflags = regs->rflags;
            curr_t->regs.cs = regs->cs;
            curr_t->regs.ss = regs->ss;
            curr_t->regs.page_table = VM::Manager::the().CurrentPageTable();
        }

        void Scheduler::Schedule(Interrupts::ISRRegisters* regs) {
            running_proc_killed = false;
            // TODO: Support multiple threads
            // Increment tid
            begin:
            curr_thread++;
            begin_no_increment:

            // Make sure PID 0 is scheduled first
            if(!first_schedule_complete) { curr_proc = 0; curr_thread = 0; first_schedule_complete = true; }
            
            ASSERT(processes.size(), "No processes");

            // Check if we have overflowed the available processes
            if(curr_proc >= processes.size()) { curr_proc = 0; curr_thread = 0; }
            Process* proc = processes.at(curr_proc);
            
            // Check if we have overflowed the available threads
            if(curr_thread >= proc->threads.size()) { curr_thread = 0; curr_proc++; goto begin_no_increment; }
            Thread* thread = proc->threads.at(curr_thread);

            // Check if this thread should be destroyed
            if(thread->blocked == Thread::BlockState::ShouldDestroy) {
                VM::Manager::the().FreePages((void*)thread->syscall_stack_map->base);
                delete thread;
                proc->threads.remove(curr_thread);
                goto begin;
            } else if(thread->blocked == Thread::BlockState::WaitingOnMessage && proc->msg_count) {
                // This thread is blocked waiting for a IPC message, and we got one
                size_t msg_size = proc->nextMessageSize();
                if(msg_size > thread->regs.rcx) {
                    // This message is bigger than the buffer, set error and unblock
                    thread->regs.rax = -E2BIG;
                    thread->blocked = Thread::BlockState::Running;
                } else {
                    // We can copy this message to the process
                    if(!proc->attemptCopyToUser(thread->regs.rbx, msg_size, proc->buffer)) {
                        // Copy failed
                        thread->regs.rax = -EINVAL;
                    } else {
                        // Copy succeded
                        thread->regs.rax = msg_size;
                    }
                    thread->blocked = Thread::BlockState::Running;
                }
            }

            // Check if this thread can be scheduled
            if(thread->blocked != Thread::BlockState::Running) { goto begin; }

            ASSERT(proc->threads.size() > 0, "Scheduler: process has no threads registered with it!");

            // Set isr registers to the saved registers in thread
            regs->rax = thread->regs.rax;
            regs->rbp = thread->regs.rbp;
            regs->rbx = thread->regs.rbx;
            regs->rcx = thread->regs.rcx;
            regs->rdi = thread->regs.rdi;
            regs->rdx = thread->regs.rdx;
            regs->rip = thread->regs.rip;
            regs->rsi = thread->regs.rsi;
            regs->rsp = thread->regs.rsp;
            regs->r8 = thread->regs.r8;
            regs->r9 = thread->regs.r9;
            regs->r10 = thread->regs.r10;
            regs->r11 = thread->regs.r11;
            regs->r12 = thread->regs.r12;
            regs->r13 = thread->regs.r13;
            regs->r14 = thread->regs.r14;
            regs->r15 = thread->regs.r15;
            regs->rflags = thread->regs.rflags;
            regs->cs = thread->regs.cs;
            regs->ss = thread->regs.ss;

            // Update TSS RSP0
            if(!proc->is_kernel) { tss_set_rsp0(thread->syscall_stack_map->base + thread->syscall_stack_map->size); }

            // Change to process page table
            VM::Manager::the().SwitchPageTables(thread->regs.page_table);
            timer_curr = 0;
        }

        void Scheduler::KillCurrentProcess() {
            ASSERT(processes.size() > curr_proc, "KillCurrentProcess() called while curr_proc is out of range!");
            Process* proc = processes.at(curr_proc);
            // KLog::the().printf("Killing %s\r\n", proc->name);
            uint64_t state = save_irqdisable();
            
            // Demap all the stacks and delete all threads
            for(size_t i = 0; i < proc->threads.size(); i++) {
                Thread* thread = proc->threads.at(i);
                if(thread->syscall_stack_map->base) {
                    VM::Manager::the().FreePages((void*)thread->syscall_stack_map->base);
                }
                delete thread;
            }
            // Free the physical pages and unmap the process
            // Switch to its page table
            uint64_t current_page_table = VM::Manager::the().CurrentPageTable();
            VM::Manager::the().SwitchPageTables(proc->page_table);
            for(size_t i = 0; i < proc->mappings.size(); i++) {
                VM::Manager::VMObject* obj = proc->mappings.at(i);
                if(obj->isShared()) { continue; }
                for(uint64_t curr = obj->base; curr < obj->size; curr += 4096) {
                    uint64_t physical = VM::Manager::the().GetPhysical(curr);
                    PM::Manager::the().FreePages(physical);
                }
                delete obj;
            }
            // The memory this process used has been freed, delete the process itself
            VM::Manager::the().SwitchPageTables(current_page_table);
            // TODO: destroy page table
            delete proc->name;
            delete proc;
            processes.remove(curr_proc);
            running_proc_killed = true;
            // Restart interrupts
            irqrestore(state);
        }

        int64_t Scheduler::IPCHint(int64_t msg_len, int64_t pipe_name_string, int64_t pipe_name_string_len) {
            Process* proc = processes.at(curr_proc);
            if(msg_len == 0) {
                // This process does not (at the moment) accept any IPC requests
                proc->accepts_ipc = false;
                if(proc->buffer) {
                    delete proc->buffer;
                    proc->msg_count = 0;
                    proc->buffer_size = 0;
                }
            } else {
                // No IPC message may be bigger than 16k (0x4000)
                if(msg_len > 0x4000) { return -E2BIG; }
                proc->buffer = new uint64_t[msg_len / 8];
                proc->buffer_size = msg_len;
                proc->accepts_ipc = true;
            }

            if(pipe_name_string) {
                // Pipe string must be smaller than 511 chars
                if(pipe_name_string_len < 0 || pipe_name_string_len > 511) { return -E2BIG; }
                char* string = new char[pipe_name_string_len + 1];
                if(!proc->attemptCopyFromUser(pipe_name_string, pipe_name_string_len, string)) { return -EINVAL; }
                string[pipe_name_string_len] = '\0';
                // Verify no pipe named like this exists
                if(FindNamedPipe(string)) { return -EEXIST; }
                IPCNamedPipe* pipe = new IPCNamedPipe;
                pipe->name = string;
                pipe->pid = proc->pid;
                named_pipes.push_back(pipe);
            }
            return 0;
        }

        int64_t Scheduler::IPCRecv(int64_t buffer_len, int64_t buffer, int64_t should, Interrupts::ISRRegisters* regs) {
            if(!buffer) { return -EINVAL; }
            // Check if any messages are available
            Process* proc = CurrentProcess();
            if(proc->msg_count) {
                // There is a message waiting, then we can just copy it (providing the size works out);
                uint64_t msg_size = proc->nextMessageSize();
                if(msg_size > (uint64_t)buffer_len) { return -E2BIG; }
                if(!proc->attemptCopyToUser(buffer, msg_size, proc->buffer)) { return -EINVAL; }
                memcopy((void*)((uint64_t)(proc->buffer) + msg_size), proc->buffer, proc->buffer_size - msg_size);
                proc->msg_count--;
            } else {
                if(!should) { return -EWOULDBLOCK; }
                // Block the process pending message
                asm volatile("cli"); // disable interrupts for now
                SaveContext(regs);
                proc->threads.at(curr_thread)->blocked = Thread::BlockState::WaitingOnMessage;
                Schedule(regs);
            }
            return 0;
        }

        int64_t Scheduler::IPCSendPipe(int64_t buffer, int64_t buffer_len, int64_t pipe_name_string, int64_t pipe_name_string_len) {
            if(!buffer || !pipe_name_string) { return -EINVAL; }
            if(buffer_len < 0 || pipe_name_string_len < 0) { return -EINVAL; }
            // Copy out the string
            Process* proc = CurrentProcess();
            char* pipe_name = new char[pipe_name_string_len + 1];
            if(!proc->attemptCopyFromUser(pipe_name_string, pipe_name_string_len, pipe_name)) { return -EINVAL; }
            pipe_name[pipe_name_string_len] = '\0';

            // Get the pipe
            IPCNamedPipe* pipe = FindNamedPipe(pipe_name);
            if(!pipe) { return -ENOENT; }
            Process* destination_proc = GetProcessByPid(pipe->pid);
            if(!destination_proc->accepts_ipc) { return -EPERM; }
            
            // Check if the buffer size allows it
            if((uint64_t)buffer_len > (destination_proc->buffer_size - destination_proc->currentBufferOffset())) { return -E2BIG; }
            // Set pid and size
            volatile uint64_t* buffer_as_long = (volatile uint64_t*)((uint64_t)destination_proc->buffer + destination_proc->currentBufferOffset());
            buffer_as_long[0] = proc->pid;
            buffer_as_long[1] = buffer_len;
            // Copy the message
            proc->attemptCopyFromUser(buffer, buffer_len, (void*)((uint64_t)destination_proc->buffer + destination_proc->currentBufferOffset() + 16));
            destination_proc->msg_count++;
            return 0;
        }

        void Scheduler::FirstSchedule() {
            // This doesnt actually schedule anything, but prepares internal values for scheduling to begin
            // Scheduling is always done from the timer interrupt
            ASSERT(processes.size(), "Attempted to begin scheduling without any processes");
            ASSERT(processes.at(0)->threads.size(), "PID 0 has no threads");
            curr_proc = 0;
            KLog::the().printf("First schedule init complete, processes count %i\r\n", processes.size());
            first_schedule_init_done = true;
        }

        void Scheduler::TimerCallback(Interrupts::ISRRegisters* regs) {
            //KLog::the().printf("Scheduler timer callback\r\n");
            // Bail if the schedule has not yet been inited, or if we were in kernel mode
            if(!first_schedule_init_done) { return; }
            
            timer_curr++;
            if(timer_curr >= timer_switch) {
                if(first_schedule_complete) {
                    SaveContext(regs);
                }
                //KLog::the().printf("Calling scheduler\r\n");
                Schedule(regs);
            }
        }
    }
}