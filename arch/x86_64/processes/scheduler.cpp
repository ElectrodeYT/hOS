#include <mem.h>
#include <mem/PM/physalloc.h>
#include <mem/VM/virtmem.h>
#include <processes/scheduler.h>
#include <timer.h>
#include <debug/serial.h>
#include <panic.h>
#include <CPP/string.h>
#include <early-boot.h>
#include <processes/elf.h>
#include <hardware/instructions.h>

namespace Kernel {
    namespace Processes {
        void Scheduler::Init() {
            // TODO
        }

        int Scheduler::CreateProcess(uint8_t* data, size_t length, uint64_t initial_instruction_pointer, char* name, bool is_kernel) {
            // Create process
            Process* new_proc = new Process;
            new_proc->page_table = VM::Manager::the().CreateNewPageTable();
            // Create the string in it, and copy the given name over
            size_t name_len = strlen(name);
            new_proc->name = new char[name_len + 1];
            memcopy(name, new_proc->name, name_len + 1);

            Debug::SerialPrintf("Creating new process %s\r\n", new_proc->name);

            // We want to save the current page table, as we need to switch to it later
            uint64_t curr_page_table = VM::Manager::the().CurrentPageTable();
            // Switch into new page table
            VM::Manager::the().SwitchPageTables(new_proc->page_table);

            // The process data should be located in V0x8000 and above

            // We want to allocate new memory and copy it into there.
            // To do this, we first allocate enough pages to copy all the data into it
            uint64_t page_count = 0;
            for(uint64_t i = 0; i < length; i += 4096) {
                uint64_t phys = PM::Manager::the().AllocatePages();
                // Map page
                VM::Manager::the().MapPage(phys, 0x8000 + i, 0b111);
                page_count++;
            }

            // Create VMObject mapping
            VM::Manager::VMObject* obj = new VM::Manager::VMObject(true, false);
            obj->base = 0x8000;
            obj->size = page_count * 4096;
            new_proc->mappings.push_back(obj);

            // Copy data
            memcopy((void*)data, (void*)0x8000, length);

            // Memory has been mapped and copied, switch back to old page table
            VM::Manager::the().SwitchPageTables(curr_page_table);

            // Initialize main thread
            Thread* main_thread = new Thread;
            main_thread->tid = 0;
            main_thread->regs.rip = initial_instruction_pointer;
            main_thread->regs.rflags = 0x202;
            main_thread->regs.cs = 0x18 | 0b11;
            main_thread->regs.ss = 0x20 | 0b11;
            main_thread->blocked = Thread::BlockState::Running;
            main_thread->regs.page_table = new_proc->page_table;
            // Create syscall thread stack
            VM::Manager::VMObject* syscall_stack = new VM::Manager::VMObject(true, false);
            syscall_stack->base = (uint64_t)VM::Manager::the().AllocatePages(4);
            syscall_stack->size = 4 * 4096;
            main_thread->syscall_stack_map = syscall_stack;

            // Attach thread to new process
            new_proc->threads.push_back(main_thread);

            // Add process
            processes.push_back(new_proc);
            return 0;
        }

        int Scheduler::CreateService(uint64_t initial_instruction_pointer, char* name) {
            // Create new process
            Process* new_proc = new Process;
            new_proc->page_table = VM::Manager::the().CurrentPageTable();
            new_proc->is_kernel = true;
            
            // Create the string in it, and copy the given name over
            size_t name_len = strlen(name);
            new_proc->name = new char[name_len + 1];
            memcopy(name, new_proc->name, name_len + 1);

            Debug::SerialPrintf("Creating new kernel service %s\r\n", new_proc->name);

            // Initialize main thread
            Thread* main_thread = new Thread;
            main_thread->tid = 0;
            main_thread->regs.rip = initial_instruction_pointer;
            main_thread->regs.rflags = 0x202;
            main_thread->regs.cs = 0x08;
            main_thread->regs.ss = 0x10;
            main_thread->blocked = Thread::BlockState::Running;
            // Initialize new stack
            VM::Manager::VMObject* stack = new VM::Manager::VMObject(true, false);
            stack->base = (uint64_t)VM::Manager::the().AllocatePages(16);
            stack->size = 4096 * 16;
            new_proc->mappings.push_back(stack);
            main_thread->regs.rsp = stack->base + stack->size;
            main_thread->regs.page_table = VM::Manager::the().CurrentPageTable();
/*
            // Create syscall thread stack
            VM::Manager::VMObject* syscall_stack = new VM::Manager::VMObject(true, false);
            syscall_stack->base = (uint64_t)VM::Manager::the().AllocatePages(4);
            syscall_stack->size = 4 * 4096;
            main_thread->syscall_stack_map = syscall_stack;
*/
            new_proc->threads.push_back(main_thread);
            processes.push_back(new_proc);
            return 0;
        }

        int Scheduler::CreateProcessFromElf(uint8_t* data, size_t length, char* name) {
            ELF elf(data, length);
            if(!elf.readHeader()) {
                Debug::SerialPrintf("Failed to load ELF file for process %s\r\n", name);
                return -1;
            }

            
            // Create process
            Process* new_proc = new Process;
            new_proc->page_table = VM::Manager::the().CreateNewPageTable();
            // Create the string in it, and copy the given name over
            size_t name_len = strlen(name);
            new_proc->name = new char[name_len + 1];
            memcopy(name, new_proc->name, name_len + 1);

            Debug::SerialPrintf("Creating new process %s\r\n", new_proc->name);

            
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
                    mapping->base = section->vaddr;
                    mapping->size = round_to_page_up(section->segment_size);

                    // Allocate it
                    size_t page_count = 0;
                    for(uint64_t i = 0; i < mapping->size; i += 4096) {
                        uint64_t phys = PM::Manager::the().AllocatePages();
                        // Map page
                        VM::Manager::the().MapPage(phys, mapping->base + i, 0b111);
                        page_count++;
                    }

                    // Now copy to it
                    section->copy_out((void*)mapping->base);
                    new_proc->mappings.push_back(mapping);
                }
            }
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

            // Create syscall thread stack
            VM::Manager::VMObject* syscall_stack = new VM::Manager::VMObject(true, false);
            syscall_stack->base = (uint64_t)VM::Manager::the().AllocatePages(4);
            syscall_stack->size = 4 * 4096;
            main_thread->syscall_stack_map = syscall_stack;

            // Attach thread to new process
            new_proc->threads.push_back(main_thread);

            // Add process
            processes.push_back(new_proc);
            return 0;
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
            
            // Return if no process exists
            if(processes.size() == 0) { curr_proc = 0; curr_thread = 0; return; }

            // Check if we have overflowed the available processes
            if(curr_proc >= processes.size()) { curr_proc = 0; curr_thread = 0; }
            Process* proc = processes.at(curr_proc);
            
            // Check if we have overflowed the available threads
            if(curr_thread >= proc->threads.size()) { curr_thread = 0; curr_proc++; goto begin_no_increment; }
            Thread* thread = proc->threads.at(curr_thread);

            // Check if this thread should be destroyed
            if(thread->blocked == Processes::Thread::BlockState::ShouldDestroy) {
                VM::Manager::the().FreePages((void*)thread->syscall_stack_map->base);
                delete thread;
                proc->threads.remove(curr_thread);
                goto begin;
            }

            // Check if this thread can be scheduled
            if(thread->blocked != Processes::Thread::BlockState::Running) { goto begin; }

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
            Process* proc = processes.at(curr_proc);
            Debug::SerialPrintf("Killing %s\r\n", proc->name);
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

        void Scheduler::FirstSchedule() {
            // This doesnt actually schedule anything, but prepares internal values for scheduling to begin
            // Scheduling is always done from the timer interrupt
            ASSERT(processes.size(), "Attempted to begin scheduling without any processes");
            ASSERT(processes.at(0)->threads.size(), "PID 0 has no threads");
            curr_proc = 0;
            Debug::SerialPrintf("First schedule init complete, processes count %i\r\n", processes.size());
            first_schedule_init_done = true;
        }

        void Scheduler::TimerCallback(Interrupts::ISRRegisters* regs) {
            //Debug::SerialPrintf("Scheduler timer callback\r\n");
            // Bail if the schedule has not yet been inited, or if we were in kernel mode
            if(!first_schedule_init_done) { return; }
            
            timer_curr++;
            if(timer_curr >= timer_switch) {
                if(first_schedule_complete) {
                    SaveContext(regs);
                }
                //Debug::SerialPrintf("Calling scheduler\r\n");
                Schedule(regs);
            }
        }
    }
}