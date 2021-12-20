#include <mem.h>
#include <mem/PM/physalloc.h>

#include <panic.h>

namespace Kernel {
    namespace PM {
        void Manager::Init(physmem_ll* memmap) {
            memory_map = memmap;
            // Traverse the list
            physmem_ll* curr = memmap;
            while(curr != NULL) {
                // If the memory region is usable, create a PMObject for it
                if(curr->type != PHYSMEM_LL_TYPE_USEABLE) { curr = curr->next; continue; }
                PMObject* object = new PMObject;
                object->allocated = false;
                object->base = curr->base;
                object->size = (curr->size & 0xFFFFFFFFFFFFF000);
                physicalmemory.push_back(object);
                curr = curr->next;
            }
        }

        uint64_t Manager::AllocatePages(int count) {
            // Acquire physical memory mutex
            acquire(&mutex);
            size_t byte_count = count * 4096;
            // Look through the vector to find unallocated pages
            for(size_t i = 0; i < physicalmemory.size(); i++) {
                PMObject* curr = physicalmemory.at(i);
                if(curr->allocated == false) {
                    // This is an unallocated page, check if its big enough
                    if(curr->size == byte_count) {
                        // We got the exact size, change it to allocated and return its base
                        curr->allocated = true;
                        // Relase the physical memory mutex
                        release(&mutex);
                        return curr->base;
                    }
                    if(curr->size < byte_count) {
                        // Less, continue
                        continue;
                    }
                    // Bigger then, we need to split it
                    PMObject* new_object = new PMObject;
                    new_object->allocated = true;
                    new_object->base = curr->base;
                    new_object->size = byte_count;
                    // Change curr's base and size
                    curr->base += byte_count;
                    curr->size -= byte_count;
                    // Insert new_object at current position
                    physicalmemory.insert(i, new_object);
                    // Relase the physical memory mutex
                    release(&mutex);
                    return new_object->base;
                }
            }
            Debug::Panic("PM::Manager: no memory left!");
        }

        bool Manager::CheckIOSpace(uint64_t phys, uint64_t size) {
            // TODO: is the physmem mutex needed here?
            // Iterate through all physical memory mapping
            uint64_t phys_high = phys + size;
            physmem_ll* curr = memory_map;
            while(curr != NULL) {
                if(curr->type == PHYSMEM_LL_TYPE_USEABLE) {
                    uint64_t mapping_high = curr->base + curr->size;
                    if((phys < mapping_high) && (phys_high > curr->base)) { return false; }
                }
            }
            return true;
        }

        void Manager::FreePages(uint64_t object) {
            (void)object;
        }
    }
}