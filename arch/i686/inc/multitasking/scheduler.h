#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

#include <memory/memorymanager.h>

namespace Kernel {
    namespace Multitasking {
        struct CPUState {
            uint32_t eax;
            uint32_t ecx;
            uint32_t edx;
            uint32_t ebx;
            uint32_t ebp;
            uint32_t esi;
            uint32_t edi;

            uint32_t esp;
            uint32_t eip;
        };

        // A single task
        struct Task {
            CPUState state;
            MemoryManager::VirtualMemory::Mapping* mappings;
        };


        int InitMultitasking();
        int CreateTask();
    }
}

#endif