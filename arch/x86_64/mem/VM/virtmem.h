#ifndef VIRTMEM_H
#define VIRTMEM_H

#include <stddef.h>
#include <CPP/vector.h>
#include <CPP/mutex.h>
#include <debug/serial.h>
#include <stivale2.h>

constexpr uint64_t round_to_page_up(uint64_t addr) {
    return ((addr + 4096 - 1) & ~(4096 - 1));
}

#define VM_DEBUG_PAGETABLES 0

// Set the page table to a specific table.
#if defined(VM_DEBUG_PAGETABLES) && VM_DEBUG_PAGETABLES
#define SwitchPageTables(table) \
    Kernel::Debug::SerialPrintf("switch page tables from %s: %x\n\r", __builtin_FUNCTION(), (uint64_t)table); \
    asm volatile("mov %0, %%cr3" : : "r"((uint64_t)table));
#else
#define SwitchPageTables(table) \
    asm volatile("mov %0, %%cr3" : : "r"((uint64_t)table));

#endif

// Various debug defines to quickly patch out / replace VMM features
// These print to debugserial, as klog might not be initialized yet
#define VM_FREE 1
#define VM_LOG_FREE 0

#define VM_LOG_ALLOC 0

namespace Kernel {
    namespace VM {
        // Initialize virtual memory.
        // Also creates the page tables, and changes to them.
        void Init(stivale2_struct_tag_memmap* memmap, uint64_t hhdm_offset);

        // Set the page table to a specific table.
        //[[maybe_unused]] static void SwitchPageTables(uint64_t table) {
        //    asm volatile("mov %0, %%cr3" : : "r"(table));
        //}

        

        // Gets the value of CR3
        uint64_t CurrentPageTable();

        // Allocate a virtual memory page. Allocated into Kernel memory.
        void* AllocatePages(size_t count = 1);

        // Frees x pages.
        void FreePages(void* adr, size_t pages);

        // Maps addresses.
        void MapPage(unsigned long phys, unsigned long virt, unsigned long options = 0b11);
        void MapPage(unsigned long phys, unsigned long virt, unsigned long options, uint64_t* table);
        
        // Create a new page table. Returns physical address. Maps the kernel stuff into the new page table.
        uint64_t CreateNewPageTable();
        // Tag a level 3 table as global
        void TagLevel3Global(unsigned long entry);
        // Gets a physical address
        uint64_t GetPhysical(uint64_t virt);

        // Get the virtual-physical offset
        uint64_t GetVirtualOffset();

        // Virtual class for other VM objects
        struct VMObject {
            VMObject(bool a, bool s) : _allocated(a), _shared(s) { }
            bool isAllocated() { return _allocated; }
            bool isShared() { return _shared; }

            // Make this VMObject shared.
            // A Region cannot be converted from shared to private, so this is
            // the only way to modify _shared after the fact.
            void makeShared() { _shared = true; }

            bool read = true;
            bool write = true;
            bool execute = true;

            uint64_t base;
            uint64_t size;

            uint8_t checkCopyOnWrite(uint64_t page) {
                if(!ref_counts) { return 0; }
                return ref_counts[page];
            }

            void decrementCopyOnWrite(uint64_t page) {
                if(ref_counts) {
                    if(ref_counts[page] > 0) { ref_counts[page]--; }
                }
            }

            ~VMObject() {
                // Decrement all CoW
                if(ref_counts) {
                    bool all_are_zero = true;
                    for(size_t i = 0; i < size; i += 4096) {
                        if(ref_counts[i / 4096] > 0) { ref_counts[i / 4096]--; }
                        if(ref_counts[i / 4096] != 0 ) { all_are_zero = false; }
                    }
                    if(all_are_zero) {
                        // Check if all the refcounts are gone
                        all_are_zero = true;
                        for(size_t i = 0; i < ref_count_org_base_len; i++) {
                            if(ref_count_org_base[i] != 0) { all_are_zero = false; break; }
                        }
                        if(all_are_zero) {
                            // Debug::SerialPrintf("ref_count_org_base for mapping at %x has depleted, deleteing ref_counts_org_base\r\n", base);
                            delete ref_count_org_base;
                        }
                    }
                }
            }

            // This creates a copy of this mapping as a CoW
            // copy.
            // This marks all pages as CoW. It is assumed that
            // the correct page table for this VMObject is currently
            // in use. The page table mappings for the new VMObject
            // must be created manually. All pages should be mapped
            // as read-only.
            // This can only be used on user VMObjects.
            VMObject* copyAsCopyOnWrite(uint64_t* phys_addresses) {
                if(!ref_counts) {
                    ref_counts = new uint8_t[round_to_page_up(size) / 4096];
                    memset(ref_counts, 1, round_to_page_up(size) / 4096);
                    ref_count_org_base = ref_counts;
                    ref_count_org_base_len = round_to_page_up(size) / 4096;
                }
                for(size_t i = 0; i < size; i += 4096) {
                    ref_counts[i / 4096]++;
                    uint64_t phys = VM::GetPhysical(base + i);
                    if(phys_addresses) { phys_addresses[i / 4096] = phys; }
                    // VM::MapPage(phys, base + i, 0b101);
                }
                VMObject* new_obj = new VMObject(_allocated, _shared);
                new_obj->base = base;
                new_obj->size = size;
                new_obj->ref_counts = ref_counts;
                new_obj->read = read;
                new_obj->write = write;
                new_obj->execute = execute;
                return new_obj;
            }

            // Refcount for this region.
            // Only for shared regions.
            uint64_t refcount = 1;
        private:
            bool _allocated;
            bool _shared;

            // CoW ref counters
            uint8_t* ref_counts = NULL;
            // Original base of the allocated ref counts
            // This is needed for munmap, as it might split
            // a ref counted vmobject and it might be able to
            // reallocate this
            uint8_t* ref_count_org_base = NULL;
            uint64_t ref_count_org_base_len = 0;
        };
    }
}

#endif