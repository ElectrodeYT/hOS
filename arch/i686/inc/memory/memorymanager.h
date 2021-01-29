#ifndef MEMORYMANAGER_H
#define MEMORYMANAGER_H

#include <stdint.h>

namespace Kernel {
    namespace MemoryManager {
        // Virtual address base of the kernel.
        const uint32_t KernelVirtualBase = 0xC0000000;

        namespace PageFrameAllocator {
            struct PhysMemSegment {
                uint32_t adr; // Address of the memory
                uint32_t len; // Length of memory
                uint8_t type; // Type of memory
                uint8_t* page_bitmap; // Page allocation bitmap
            };

            int InitPageFrameAllocator(void* multiboot_info);

            void* AllocatePage();
            int FreePage(void* page);
        }
    }
}

#endif