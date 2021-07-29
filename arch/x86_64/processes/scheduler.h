#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <hardware/cpu.h>
#include <processes/process.h>
#include <CPP/vector.h>
#include <interrupts.h>

#include <stddef.h>

namespace Kernel {
    namespace Processes {
        class Scheduler {
        public:
            void Init();

            void TimerCallback(Interrupts::ISRRegisters* regs);

            // Create a new process with its own memory map.
            // data must be page aligned, in kernel memory, and length should be at page boundries.
            // Returns PID of the process
            int CreateProcess(uint8_t* data, size_t length, uint64_t initial_instruction_pointer, char* name, bool inKernel);

            // Create a new service which runs with kernel mappings.
            // Returns PID of the process
            int CreateService(uint64_t initial_instruction_pointer, char* name);

            // Save the context of the current process.
            void SaveContext(Interrupts::ISRRegisters* regs);

            // Updates regs with the required information to go to the next process, and switches the page table.
            void Schedule(Interrupts::ISRRegisters* regs);

            // Do the first schedule.
            void FirstSchedule();

            // Kill the current process.
            // TODO: add refcounting to the memory mappings
            void KillCurrentProcess();

            // Load a ELF file.
            int CreateProcessFromElf(uint8_t* data, size_t length, char* name);


            inline Process* CurrentProcess() { return processes.at(curr_proc); }

            static Scheduler& the() {
                static Scheduler instance;
                return instance;
            }

        private:
            const int timer_switch = 1;
            int timer_curr = 0;

            int curr_proc;
            int curr_thread;

            Vector<Process*> processes;
            bool first_schedule_init_done = false;
            bool first_schedule_complete = false;

            bool running_proc_killed = false;
            bool running_thread_killed = false;
        };
    }
}

#endif