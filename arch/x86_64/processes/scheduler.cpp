#include <mem.h>
#include <mem/PM/physalloc.h>
#include <mem/VM/virtmem.h>
#include <processes/scheduler.h>
#include <timer.h>
#include <debug/klog.h>
#include <panic.h>
#include <CPP/string.h>
#include <early-boot.h>
#include <hardware/instructions.h>
#include <processes/syscalls/syscall.h>
#include <errno.h>
#include <kernel-drivers/VFS.h>

namespace Kernel {
    namespace Processes {
        void Scheduler::Init() {
            // TODO
        }
        int64_t Scheduler::CreateProcessImpl(uint8_t* data, size_t length, char** argv, int argc, char** envp, int envc, const char* working_dir, bool init) {
            acquire(&mutex);
            // Sanity checks
            if(!argv) { return -EINVAL; }
            if(!envp) { return -EINVAL; }
            if(argc < 0) { return -EINVAL; }
            if(envc < 0) { return -EINVAL; }

            ELF* elf = new ELF(data, length);
            if(!elf->readHeader()) {
                KLog::the().printf("Failed to load ELF file for process %s\r\n", argv[0]);
                return -EINVAL;
            }
            // If this is a dynamic executable, we need to load the dynamic relocator
            char* interpreter_name;
            bool is_interpreter = false;
            if(elf->isDynamic) {
                KLog::the().printf("Loading ELF interpreter %s\n\r", elf->interpreterPath);
                // Copy out the name
                interpreter_name = new char[strlen(elf->interpreterPath)];
                memcopy(elf->interpreterPath, interpreter_name, strlen(elf->interpreterPath));
                // Try to load the ld.so used by this executable
                int64_t ld_fd = VFS::the().open("/", elf->interpreterPath, -1);
                if(ld_fd < 0) {
                    KLog::the().printf("Error executing ELF file %s: interpreter %s doesnt exist\n\r", argv[0], elf->interpreterPath);
                    delete elf;
                    return -EINVAL;
                }
                delete elf;
                size_t int_length = VFS::the().size(ld_fd, -1);
                uint8_t* int_data = new uint8_t[int_length];
                VFS::the().pread(ld_fd, int_data, int_length, 0, -1);
                VFS::the().close(ld_fd, -1);
                elf = new ELF(int_data, int_length);
                if(!elf->readHeader()) {
                    KLog::the().printf("Failed to load ELF header for ld\n\r");
                }
                is_interpreter = true;
            }
        

            // Calculate argv size
            size_t argv_size = 0;
            for(size_t i = 0; i < (size_t)argc; i++) {
                argv_size += strlen(argv[i]) + 1;
                argv_size += 8; // Pointer
            }
            if(is_interpreter) {
                argv_size += strlen(interpreter_name);
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


            // Create process
            Process* new_proc = new Process;
            new_proc->page_table = VM::CreateNewPageTable();
            if(init) {
                ASSERT(!init_spawned, "CreateProcessImpl called with init after init has been spawned");
                new_proc->pid = 0;
                init_spawned = true;
            } else {
                new_proc->pid = GetNextPid();
            }
            // Set the parent
            new_proc->parent = 0;
            // Create the string in it, and copy the given name over
            size_t name_len = strlen(argv[0]);
            new_proc->name = new char[name_len + 1];
            memcopy(argv[0], new_proc->name, name_len + 1);
            // Copy working dir
            size_t working_dir_len = strlen(working_dir);
            new_proc->working_dir = new char[working_dir_len + 1];
            memcopy((void*)working_dir, new_proc->working_dir, working_dir_len + 1);
            

            KLog::the().printf("Creating new process %s, working dir %s\r\n", new_proc->name, new_proc->working_dir);

            
            // We want to save the current page table, as we need to switch to it later
            uint64_t curr_page_table = VM::CurrentPageTable();
            // Switch into new page table
            SwitchPageTables(new_proc->page_table);

            MapELF(elf, new_proc, is_interpreter ? 0x40000000 : 0, is_interpreter);

            // If we loaded the interpreter, we can now perform relocations on the ELF binary
            if(is_interpreter) {
                //KLog::the().printf("Relocating interpreter\n\r");
                //elf->relocate((void*)0x40000000);
            }

            // Create main stack
            const uint64_t stack_size = 16 * 4096;
            uint64_t stack_base = SyscallHandler::the().mmap(new_proc, stack_size, NULL);

            // If we have a dynamic link, we need to copy the elf file of the original executable into memory
            uint64_t aux_phdr = 0;
            uint64_t aux_phent = 0;
            uint64_t aux_phnum = 0;
            uint64_t aux_entry = 0;
            if(is_interpreter) {
                ELF* real_elf = new ELF(data, length); // original elf file
                real_elf->readHeader();
                
                // Map the original ELF to memory
                MapELF(real_elf, new_proc, real_elf->file_base ? 0 : 0x4000000, false);
                
                aux_entry = real_elf->file_entry;
                aux_phdr = real_elf->file_base + real_elf->file_program_header_offset;
                aux_phent = real_elf->file_program_header_entry_size;
                aux_phnum = real_elf->file_program_header_count;
            }

            // Setup stack
            uint64_t proc_rsp;
            ProcessSetupStack(argv, argc, envp, envc, (void*)(stack_base + stack_size - 8), aux_entry, aux_phdr, aux_phent, aux_phnum, &proc_rsp);

            // The sections have been copied out, we can now switch the page table back
            SwitchPageTables(curr_page_table);

            // Initialize main thread
            Thread* main_thread = new Thread;
            main_thread->tid = 0;
            main_thread->regs.rip = elf->file_entry;
            if(is_interpreter) {
                main_thread->regs.rip += 0x40000000;
            }
            main_thread->regs.rflags = 0x202;
            main_thread->regs.cs = 0x18 | 0b11;
            main_thread->regs.ss = 0x20 | 0b11;
            main_thread->blocked = Thread::BlockState::Running;
            main_thread->regs.page_table = new_proc->page_table;

            main_thread->stack_base = stack_base;
            main_thread->stack_size = stack_size;
            main_thread->regs.rsp = proc_rsp; // -16 for the two pointers on the stack
            main_thread->regs.rdi = argc;
            main_thread->regs.rsi = 0;
            main_thread->regs.rdx = 0;
            main_thread->regs.rcx = envc;
            
            // Create syscall thread stack
            // TODO: guard pages
            VM::VMObject* syscall_stack = new VM::VMObject(true, false);
            syscall_stack->base = (uint64_t)VM::AllocatePages(4);
            syscall_stack->size = 4 * 4096;
            main_thread->syscall_stack_map = syscall_stack;

            // Initialize process FD table
            new_proc->fd_translation_table = new Vector<Process::VFSTranslation>;
            new_proc->fd_translation_table_ref_count = new int;

            // Attach thread to new process
            new_proc->threads.push_back(main_thread);

            // Add process
            processes.push_back(new_proc);
            release(&mutex);
            return new_proc->pid;
        }

        bool Scheduler::ProcessSetupStack(char** argv, int argc, char** envp, int envc, void* stack_base, uint64_t entry, uint64_t phdr, uint64_t phent, uint64_t phnum, uint64_t* rsp) {
            // We save the pointers to the args/env we put on the stack here, to quickly push them
            char** argv_stack_pos = new char*[argc];
            char** envp_stack_pos = new char*[envc];
            
            uint64_t* proc_stack = (uint64_t*)stack_base;
            #define push_to_stack(a) *--proc_stack = a;
            
            // Copy the argv to the stack
            for(size_t i = 0; i < (size_t)argc; i++) {
                size_t arg_len = strlen(argv[i]); if(arg_len > 0x1000) { return false; }
                proc_stack = (uint64_t*)((uint8_t*)(proc_stack) - arg_len + 1);
                memset(proc_stack, 0, arg_len + 1);
                memcopy(argv[i], proc_stack, arg_len);
                argv_stack_pos[i] = (char*)proc_stack;
            }

            // Copy the envp to the stack
            for(size_t i = 0; i < (size_t)envc; i++) {
                size_t env_len = strlen(envp[i]); if(env_len > 0x1000) { return false; }
                proc_stack = (uint64_t*)((uint8_t*)(proc_stack) - env_len + 1);
                memset(proc_stack, 0, env_len + 1);
                memcopy(envp[i], proc_stack, env_len);
                envp_stack_pos[i] = (char*)proc_stack;
            }

            // Pad the stack
            size_t stack_pad_size = ((size_t)proc_stack & 0xF);
            proc_stack = (uint64_t*)((uint8_t*)(proc_stack) -  stack_pad_size);

            // We now create the AUX vector
            // First we push the ending of it
            push_to_stack(0);
            // Now we add phdr, phent, phnum and entry
            push_to_stack(phdr);
            push_to_stack(ELF::ElfAuxVector::AT_PHDR);
            push_to_stack(phent);
            push_to_stack(ELF::ElfAuxVector::AT_PHENT);
            push_to_stack(phnum);
            push_to_stack(ELF::ElfAuxVector::AT_PHNUM);
            push_to_stack(entry);
            push_to_stack(ELF::ElfAuxVector::AT_ENTRY);

            // Push the env shit
            push_to_stack(0);
            for(int i = envc - 1; i >= 0; i--) { push_to_stack((uint64_t)envp_stack_pos[i]); }

            push_to_stack(0);
            // Push the arguments
            for(int i = argc - 1; i >= 0; i--) { push_to_stack((uint64_t)argv_stack_pos[i]); }
            push_to_stack(argc);

            *rsp = (uint64_t)proc_stack;
            return true;
        }

        void Scheduler::MapELF(ELF* elf, Process* new_proc, uint64_t offset, bool is_interpreter) {
            // We now need to loop through all the created segments, and see if we
            // need to make mappings for them
            for(size_t i = 0; i < elf->sections.size(); i++) {
                ELF::Section* section = elf->sections.at(i);
                if(section->loadable) {
                    // TODO: correct permissions
                    VM::VMObject* mapping = new VM::VMObject(true, false);
                    mapping->base = (section->vaddr + offset) & ~(0xFFF);
                    mapping->size = round_to_page_up(section->segment_size);

                    if(mapping->base == 0) { continue; }

                    KLog::the().printf("Mapping for ELF segment %i: base %x, size %x\r\n", i, mapping->base, mapping->size);

                    // Allocate it
                    size_t page_count = 0;
                    for(uint64_t i = 0; i < mapping->size; i += 4096) {
                        uint64_t phys = PM::AllocatePages();
                        // Map page
                        VM::MapPage(phys, mapping->base + i, 0b111);
                        page_count++;
                    }

                    // Now copy to it
                    section->copy_out((void*)mapping->base);
                    new_proc->mappings.push_back(mapping);
                }
            }

            (void)is_interpreter;
        }

        int64_t Scheduler::CreateProcess(uint8_t* data, size_t length, const char* name, const char* working_dir, bool init) {
            // Create a fake envc, envp
            char* fake_env[1] = { NULL };
            // Create a argv containing only the program name
            char* argv[1] = { (char*)name }; 
            return CreateProcessImpl(data, length, argv, 1, fake_env, 0, working_dir, init); // parent = UINT64_MAX; this will make CreateProcessImpl set parent to itself 
        }

        int64_t Scheduler::CreateProcess(uint8_t* data, size_t length, char** argv, int argc) {
            // We use the envc, envp of the parent
            // TODO: yeah that
            char* fake_env[1] = { NULL };
            return CreateProcessImpl(data, length, argv, argc, fake_env, 0, CurrentProcess()->working_dir);
        }

        int Scheduler::CreateKernelTask(void (*start)(void*), void* arg, uint64_t stack_size) {
            acquire(&mutex);
            Processes::Process* proc = new Processes::Process;
            proc->page_table = VM::CreateNewPageTable();
            proc->is_kernel = true;
            proc->name = new char[8];
            proc->pid = GetNextPid();
            // Set the parent
            proc->parent = 0;
            // Copy the name
            memcopy((void*)"kworker", proc->name, 8);
            // Init thread
            Thread* thread = new Processes::Thread;
            thread->stack_size = stack_size * 1024;
            thread->stack_base = (uint64_t)VM::AllocatePages(stack_size);
            thread->tid = 0;
            thread->regs.page_table = proc->page_table;
            thread->regs.rflags = 0x202;
            thread->regs.cs = 0x8;
            thread->regs.ss = 0x10;
            thread->regs.rip = (uint64_t)start;
            thread->regs.rdi = (uint64_t)arg;
            thread->regs.rsp = thread->stack_base + thread->stack_size;
            thread->blocked = Thread::BlockState::Running;
            
            // Create interrupt stack
            VM::VMObject* intr_stack = new VM::VMObject(true, false);
            intr_stack->base = (uint64_t)VM::AllocatePages(4);
            intr_stack->size = 4 * 4096;
            thread->syscall_stack_map = intr_stack;

            // Add thread
            proc->threads.push_back(thread);
            // Add process
            processes.push_back(proc);
            release(&mutex);
            return proc->pid;
        }

        int64_t Scheduler::ForkCurrent(Interrupts::ISRRegisters* regs) {
            acquire(&mutex);
            // Create new process
            Processes::Process* new_proc = new Processes::Process();
            Processes::Process* curr_proc = CurrentProcess();
            
            // Copy the name over
            size_t name_len = strlen(curr_proc->name);
            new_proc->name = new char[name_len + 1];
            new_proc->pid = GetNextPid();
            memcopy(curr_proc->name, new_proc->name, name_len);

            // Copy the memory mappings over
            new_proc->page_table = VM::CreateNewPageTable();
            uint64_t current_table = VM::CurrentPageTable();
            for(size_t i = 0; i < curr_proc->mappings.size(); i++) {
                uint64_t* physical_addresses = new uint64_t[curr_proc->mappings.at(i)->size / 4096];
                VM::VMObject* new_cow = curr_proc->mappings.at(i)->copyAsCopyOnWrite(physical_addresses);
                // Now we need to switch to the new page table
                SwitchPageTables(new_proc->page_table);
                // Now we need to map all the pages as read only
                for(size_t x = 0; x < new_cow->size; x += 4096) {
                    VM::MapPage(physical_addresses[x / 4096], new_cow->base + x, 0b101);
                }
                new_proc->mappings.push_back(new_cow);
                // Switch back
                SwitchPageTables(current_table);
                delete physical_addresses;
            }

            // The forked processes share file descriptors, copy the pointers
            new_proc->fd_translation_table = curr_proc->fd_translation_table;
            new_proc->fd_translation_table_ref_count = curr_proc->fd_translation_table_ref_count;
            *(new_proc->fd_translation_table_ref_count) = *(new_proc->fd_translation_table_ref_count) + 1;

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
            VM::VMObject* syscall_stack = new VM::VMObject(true, false);
            syscall_stack->base = (uint64_t)VM::AllocatePages(4);
            syscall_stack->size = 4 * 4096;
            main_thread->syscall_stack_map = syscall_stack;

            new_proc->threads.push_back(main_thread);
            processes.push_back(new_proc);
            release(&mutex);
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
            curr_t->regs.page_table = VM::CurrentPageTable();
        }

        void Scheduler::Schedule(Interrupts::ISRRegisters* regs) {
            running_proc_killed = false;
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
            
            // Check if this process should actually still exist
            if(proc->attempt_destroy) {
                // Check if we share a stack with any of the threads
                // Demap all the stacks and delete all threads
                bool unmapped_everything = true;
                for(size_t i = 0; i < proc->threads.size(); i++) {
                    Thread* thread = proc->threads.at(i);
                    if(!thread->syscall_stack_map->base) { continue; }
                    // Check if we are currently using this stack (lol)
                    uint64_t curr_frame = (uint64_t)__builtin_frame_address(0);
                    if(thread->syscall_stack_map->base <= curr_frame && (thread->syscall_stack_map->base + thread->syscall_stack_map->size) >= curr_frame) {
                        unmapped_everything = false;
                    } else {
                        VM::FreePages((void*)thread->syscall_stack_map->base, thread->syscall_stack_map->size / 4096);
                        delete thread;
                    }
                }
                if(unmapped_everything) {
                    delete proc;
                    processes.remove(curr_proc);
                } else {
                    curr_proc++;
                    curr_thread = 0;
                }
                goto begin_no_increment;
            }

            // Check if we have overflowed the available threads
            if(curr_thread >= proc->threads.size()) { curr_thread = 0; curr_proc++; goto begin_no_increment; }
            Thread* thread = proc->threads.at(curr_thread);

            // Check if this thread should be destroyed
            if(thread->blocked == Thread::BlockState::ShouldDestroy) {
                VM::FreePages((void*)thread->syscall_stack_map->base, thread->syscall_stack_map->size / 4096);
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
            tss_set_rsp0(thread->syscall_stack_map->base + thread->syscall_stack_map->size);
            // Change to process page table
            SwitchPageTables(thread->regs.page_table);
            timer_curr = 0;
        }

        void Scheduler::KillCurrentProcess() {
            ASSERT(processes.size() > curr_proc, "KillCurrentProcess() called while curr_proc is out of range!");
            Process* proc = processes.at(curr_proc);
            ASSERT(proc->pid, "Attempted to kill init!");
            // KLog::the().printf("Killing %s\r\n", proc->name);
            uint64_t state = save_irqdisable();
            
            // Free the physical pages and unmap the process
            // Switch to its page table
            uint64_t current_page_table = VM::CurrentPageTable();
            SwitchPageTables(proc->page_table);
            for(size_t i = 0; i < proc->mappings.size(); i++) {
                VM::VMObject* obj = proc->mappings.at(i);
                if(obj->isShared()) { continue; }
                for(uint64_t curr = obj->base; curr < obj->size; curr += 4096) {
                    uint64_t physical = VM::GetPhysical(curr);
                    PM::FreePages(physical);
                }
                delete obj;
            }
            // The memory this process used has been freed, delete the process itself
            SwitchPageTables(current_page_table);
            // TODO: destroy page table
            // probably has to be done in scheduler
            delete proc->name;
            proc->attempt_destroy = true;
            running_proc_killed = true;
            // Restart interrupts
            irqrestore(state);
        }

        void Scheduler::WaitOnIRQ(int irq) {
            // The timer subsystem needs IRQ 0, so we simply return lol
            if(irq == 0) { return; }
            // We now check if we need to register a callback for this interrupt
            if(irq_wait_states[irq]++) { Interrupts::the().RegisterIRQHandler(irq, SchedulerIRQCallbackWrapper); }
            // Set wait state, being carefull to set irq before the block
            Process* curr = CurrentProcess();
            Thread* curr_t = curr->threads.at(curr_thread);
            curr_t->irq = irq;
            curr_t->blocked = Thread::BlockState::WaitingOnIRQ;
            while(curr_t->blocked == Thread::BlockState::WaitingOnIRQ);
            // We have now in fact unblocked, decrement irq_wait_states
            irq_wait_states[irq]--;
            if(irq_wait_states[irq] == 0) { Interrupts::the().DeregisterIRQHandler(irq); }
        }

        void Scheduler::IRQHandler(Interrupts::ISRRegisters* regs) {
            // Loop through all processes and threads and unblock all ones waiting for our irq
            for(size_t i = 0; i < processes.size(); i++) {
                Process* proc = processes.at(i);
                for(size_t y = 0; y < proc->threads.size(); y++) {
                    Thread* thread = proc->threads.at(y);
                    if(thread->blocked == Thread::BlockState::WaitingOnIRQ && thread->irq == regs->int_num) {
                        thread->blocked = Thread::BlockState::Running;
                    }
                }
            }
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
                if(mutex) { return; } // lol
                if(first_schedule_complete) {
                    SaveContext(regs);
                }
                //KLog::the().printf("Calling scheduler\r\n");
                Schedule(regs);
            }
        }

        void SchedulerIRQCallbackWrapper(Interrupts::ISRRegisters* regs) {
            Scheduler::the().IRQHandler(regs);
        }
    }
}