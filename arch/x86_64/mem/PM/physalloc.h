#ifndef PHYSALLOC_H
#define PHYSALLOC_H

#include <stddef.h>
#include <stdint.h>
#include <CPP/vector.h>
#include <CPP/mutex.h>
#include <stivale2.h>

namespace Kernel {
    namespace PM {
        void Init(stivale2_struct_tag_memmap* memmap, uint64_t hddm_offset);
        void MapPhysical();
        uint64_t AllocatePages(int count = 1);
        void FreePages(uint64_t pm, int count = 1);
        // Check if the memory space passed is a IO area.
        // Basically this returns false if this is a ram region
        bool CheckIOSpace(uint64_t phys, uint64_t size);

        // Dump the current amount of used pages to KLog.
        void PrintMemUsage();
    }
}
#endif