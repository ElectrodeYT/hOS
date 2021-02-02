#ifndef MEMORYMANAGER_H
#define MEMORYMANAGER_H

#include <stdint.h>

namespace Kernel {
    namespace MemoryManager {
        // Virtual address base of the kernel.
        const uint32_t KernelVirtualBase = 0xC0000000;
        // Page frame allocator.
        // We need to be super-duper-carefull here.
        // This gets initialize before constructors get called,
        // and while we do have access to kmalloc(), we dont want to flood it.
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

        namespace VirtualMemory {
            // Virtual memory mapping
            struct Mapping {
                uint32_t phys;
                uint32_t virt;
            };

            int CheckIfPageIsMapped(uint32_t adr);
            int InitVM();
            Mapping GetPage();



            int MapPage(uint32_t phys, uint32_t virt, uint32_t pagedirectory);
            int MapPage(uint32_t phys, uint32_t virt);
            int UnmapPage(uint32_t virt);
            
            void SetPageDirectory();
            // Creates a new page directory.
            Mapping CreatePageDirectory();
        }
    }
}

#endif