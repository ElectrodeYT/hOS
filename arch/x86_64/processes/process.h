#ifndef PROCESS_H
#define PROCESS_H

#include <CPP/vector.h>
#include <mem/VM/virtmem.h>
#include <hardware/cpu.h>

namespace Kernel {
    namespace Processes {
        // Actual execution thing
        struct Thread {
            int tid;
            Hardware::Registers regs;
            enum class BlockState {
                Running = 0,
                WaitingOnMessage,
                ShouldDestroy
            };
            BlockState blocked;
            // Syscall stack mapping.
            // This is specific to each thread, and is not in process mappings.
            // It will be deallocated when a thread is killed.
            VM::Manager::VMObject* syscall_stack_map;
        };

        // Container of Threads and memory
        struct Process {
            int pid;
            bool is_kernel;
            bool accepts_ipc;
            char* name;
            uint64_t page_table;
            Vector<Thread*> threads;
            // Store the base address of all mappings.
            // TODO: find a way to deallocate memory
            Vector<VM::Manager::VMObject*> mappings;

            // IPC Info
            VM::Manager::VMObject* ipc_map;
            uint64_t ipc_left;
            uint64_t ipc_right;

            bool attemptCopyFromUser(uint64_t user_pointer, size_t size, void* destination);
            bool attemptMessageSend(void* msg, size_t size);
        };
    }
}

#endif