#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <hardware/cpu.h>
#include <processes/process.h>
#include <CPP/vector.h>
#include <interrupts.h>
#include <CPP/string.h>
#include <stddef.h>

namespace Kernel {
    namespace Processes {
        class Scheduler {
        public:
            // Named IPC pipe
            struct IPCNamedPipe {
                char* name;
                int64_t pid;
            };

            void Init();

            void TimerCallback(Interrupts::ISRRegisters* regs);

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
            // This is used to create "first pids"; Other exec and forks should result from other versions of this process.
            // This initializes argc to 1 and argv to name, and sets env and enc to NULL.
            int64_t CreateProcess(uint8_t* data, size_t length, const char* name, const char* working_dir, bool init = false);

            // Create a process as a parent from another process.
            // This copies the enviroment from the parent process.
            int64_t CreateProcess(uint8_t* data, size_t length, char** argv, int argc);

            // Fork the current process. 
            // This creates a new process, copies all its memory mappings, and returns the new process's PID.
            // The new process will have RAX set to 0.
            // The new process does not share the same page table, but does share the same memory contents.
            int64_t ForkCurrent(Interrupts::ISRRegisters* regs);

            // Exec a new process.
            // This creates a new process in place of the current process. It will decrease the refcount of all its VMObjects,
            // and deallocate them if neccesary.
            // The enviroment is taken from the execing process.
            int Exec(uint8_t* data, size_t length, char** argv, int arc);

            // Create a task running in kernel space.
            int CreateKernelTask(void (*start)(void*), void* arg, uint64_t stack_size);

            // Wait on a IRQ
            void WaitOnIRQ(int irq);

            // IRQ Handler, for waiting tasks
            void IRQHandler(Interrupts::ISRRegisters* regs);

            IPCNamedPipe* FindNamedPipe(const char* name) {
                for(size_t i = 0; i < named_pipes.size(); i++) {
                    IPCNamedPipe* curr = named_pipes.at(i);
                    if(strcmp(curr->name, name) == 0) { return curr; }
                }
                return NULL;
            }

            // Get a process by PID
            inline Process* GetProcessByPid(int64_t pid) {
                for(size_t i = 0; i < processes.size(); i++) {
                    if(processes.at(i)->pid == pid) { return processes.at(i); }
                }
                return NULL;
            }

            inline Process* CurrentProcess() { return processes.at(curr_proc); }

            static Scheduler& the() {
                static Scheduler instance;
                return instance;
            }

            size_t curr_proc;
            size_t curr_thread;

        private:
            // Implementation of CreateProcess.
            int64_t CreateProcessImpl(uint8_t* data, size_t length, char** argv, int arc, char** envp, int envc, const char* working_dir, bool init = false);

            // Get the next available PID
            int64_t GetNextPid() {
                if(processes.size() == 0) { return 1; }
                int64_t curr = 1;
                for(size_t i = 0; i < processes.size();) {
                    if(!processes.at(i)->is_kernel && processes.at(i)->pid == curr) { curr++; i = 0; continue; }
                    i++;
                }
                return curr;
            }

            // List of IRQs we have been waiting on
            uint64_t irq_wait_states[64];

            const int timer_switch = 5;
            int timer_curr = 0;


            Vector<IPCNamedPipe*> named_pipes;
            Vector<Process*> processes;
            bool first_schedule_init_done = false;
            bool first_schedule_complete = false;

            bool running_proc_killed = false;
            bool running_thread_killed = false;

            bool init_spawned = false;

            mutex_t mutex = 0;
        };
        void SchedulerIRQCallbackWrapper(Interrupts::ISRRegisters* regs);
    }
}

#endif