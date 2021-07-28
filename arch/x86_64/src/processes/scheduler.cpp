#include <mem.h>
#include <mem/PM/physalloc.h>
#include <mem/VM/virtmem.h>
#include <processes/scheduler.h>
#include <timer.h>
#include <debug/serial.h>
#include <panic.h>
#include <CPP/string.h>

namespace Kernel {
    namespace Processes {
        void Scheduler::Init() {
            // TODO
        }

        int Scheduler::CreateProcess(uint8_t* data, size_t length, uint64_t initial_instruction_pointer, char* name, bool isKernel) {
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
            int page_count = 0;
            for(int i = 0; i < length; i += 4096) {
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
            new_proc->isKernel = true;
            
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

            // Initialize new stack
            VM::Manager::VMObject* stack = new VM::Manager::VMObject(true, false);
            stack->base = (uint64_t)VM::Manager::the().AllocatePages(16);
            stack->size = 4096 * 16;
            new_proc->mappings.push_back(stack);

            main_thread->regs.rsp = stack->base + stack->size;
            new_proc->threads.push_back(main_thread);
            processes.push_back(new_proc);
            return 0;
        }

        void Scheduler::SaveContext(Interrupts::ISRRegisters* regs) {
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
        }

        void Scheduler::Schedule(Interrupts::ISRRegisters* regs) {
            // TODO: Support multiple threads
            // Increment pid
            curr_proc++;
            // Make sure PID 0 is scheduled first
            if(!first_schedule_complete) { curr_proc = 0; first_schedule_complete = true; }
            // Return if no process exists
            if(processes.size() == 0) { curr_proc = 0; return; }
            
            // Check if the process exists
            if(curr_proc >= processes.size()) { curr_proc = 0; }
            // curr_proc should now be accurate
            Process* proc = processes.at(curr_proc);
            ASSERT(proc->threads.size() > 0, "Scheduler: process has no threads registered with it!");
            Thread* thread = proc->threads.at(0);
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

            // Change to process page table
            VM::Manager::the().SwitchPageTables(proc->page_table);
            timer_curr = 0;
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