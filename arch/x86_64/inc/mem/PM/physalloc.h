#ifndef PHYSALLOC_H
#define PHYSALLOC_H

#include <stddef.h>
#include <stdint.h>
#include <CPP/vector.h>

namespace Kernel {
    namespace PM {

        // Memory map in non-stivale2 form. Passed to PhysMem to allow for easier porting.
        struct physmem_ll {
            // Base address
            uint64_t base;
            // Size of this segment
            size_t size;
            // Types of physical memory
            #define PHYSMEM_LL_TYPE_USEABLE 0       // Usable memory
            #define PHYSMEM_LL_TYPE_RESERVED 1      // Memory reserved for a reason not available
            #define PHYSMEM_LL_TYPE_ACPI_REC 2      // Reclaimable with ACPI
            #define PHYSMEM_LL_TYPE_ACPI_NVS 3      // ACPI NVS
            #define PHYSMEM_LL_TYPE_BAD 4           // Existing, but bad memory
            #define PHYSMEM_LL_TYPE_KERNEL 5        // Kernel location
            #define PHYSMEM_LL_TYPE_BOOTLOADER 6    // Bootloader data
            #define PHYSMEM_LL_TYPE_LOWMEM 7        // Low memory area
            uint8_t type;
            // Next entry in the list
            physmem_ll* next;
        } __attribute__((packed));

        class Manager {
        public:

            // Denotes a region of physical memory.
            // Used by the physical memory manager.
            struct PMObject {
                bool allocated;
                uint64_t base;
                uint64_t size;

                uint64_t end() { return base + size; }

                // Returns true if the passed PMObject is contigous in memory.
                bool isContigous(const PMObject& b) {
                    return (b.base == (base + size)) || (base = (b.base + b.size));
                }
            };

            void Init(physmem_ll* memorymap);
            uint64_t AllocatePages(int count = 1);
            void FreePages(uint64_t pm);

            static Manager& the() {
                static Manager instance;
                return instance;
            }
        private:
            

            Vector<PMObject*> physicalmemory;
        };
    }
}
#endif