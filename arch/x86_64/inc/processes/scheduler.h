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
            int CreateProcess(uint8_t* data, size_t length, uint64_t initial_instruction_pointer, char* name, bool inKernel);

            // Create a new service which runs with kernel mappings.
            int CreateService(uint64_t initial_instruction_pointer, char* name);

            // Save the context of the current process.
            void SaveContext(Interrupts::ISRRegisters* regs);

            // Updates regs with the required information to go to the next process, and switches the page table.
            void Schedule(Interrupts::ISRRegisters* regs);

            // Do the first schedule.
            void FirstSchedule();

            static Scheduler& the() {
                static Scheduler instance;
                return instance;
            }

        private:
            const int timer_switch = 10;
            int timer_curr = 0;

            int curr_proc;
            int curr_thread;

            Vector<Process*> processes;
            bool first_schedule_init_done = false;
            bool first_schedule_complete = false;
        };
    }
}

#endif