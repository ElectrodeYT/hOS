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
                WaitingOnIRQ, // A Driver this thread has called into is waiting on a IRQ
                ShouldDestroy
            };
            BlockState blocked;

            uint64_t irq; // The IRQ this thread is waiting on if it is waiting.

            // Syscall stack mapping.
            // This is specific to each thread, and is not in process mappings.
            // It will be deallocated when a thread is killed.
            VM::VMObject* syscall_stack_map;

            // Thread stack location.
            // If this thread is destroyed, it will be unmaped with a (fake) call to munmap.
            // If the process this thread belongs to is killed, this is ignored.
            uint64_t stack_base;
            uint64_t stack_size;
        };

        // Container of Threads and memory
        struct Process {
            int pid;
            int parent;
            bool is_kernel;
            bool accepts_ipc;
            char* name;
            uint64_t page_table;
            Vector<Thread*> threads;
            // Store the base address of all mappings.
            // TODO: find a way to deallocate memory
            Vector<VM::VMObject*> mappings;

            // Buffer for IPC
            // When a readipc_msg or waitipc_msg is done,
            // It reads of the top of this buffer
            uint64_t* buffer;
            size_t buffer_size;
            uint64_t msg_count;
            uint64_t currentBufferOffset() {
                uint64_t curr = 0;
                for(size_t i = 0; i < msg_count; i++) {
                    curr += 8; // Skip pid
                    int64_t len = *(volatile int64_t*)((uint64_t)(buffer) + curr);
                    curr += 8; // sizeof(len)
                    if(len < 0) { continue; } // If it is negative, this is an event, so we dont have any data
                    curr += len; // otherwise, this is a normal IPC message
                    // Skip pid again
                    curr += 8;
                }
                return curr;
            }

            uint64_t nextMessageSize() {
                return *(volatile int64_t*)((uint64_t)(buffer) + 8) + 16;
            }
            int64_t uid;
            int64_t gid;

            int64_t true_uid;
            int64_t true_gid;

            bool attemptCopyFromUser(uint64_t user_pointer, size_t size, void* destination);
            bool attemptCopyToUser(uint64_t user_pointer, size_t size, void* source);
        };
    }
}

#endif