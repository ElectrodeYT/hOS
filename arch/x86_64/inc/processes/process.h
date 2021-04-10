#ifndef PROCESS_H
#define PROCESS_H

#include <CPP/vector.h>
#include <mem/VM/virtmem.h>

namespace Kernel {
    namespace Processes {
        // Actual execution thing
        struct Thread {
            int tid;
            Hardware::Registers regs;
        };

        // Container of Threads and memory
        struct Process {
            int pid;
            uint64_t page_table;
            Vector<Thread*> threads;
            // Store the base address of all mappings.
            // TODO: find a way to deallocate memory
            Vector<VM::Manager::VMObject*> mappings;
        };
    }
}

#endif