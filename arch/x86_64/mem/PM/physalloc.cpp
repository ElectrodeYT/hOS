#include <mem.h>
#include <mem/PM/physalloc.h>
#include <mem/VM/virtmem.h>
#include <CPP/string.h>
#include <debug/klog.h>
#include <panic.h>
#include <early-boot.h>

#define BIT(b, x) ((b[(x)/8] & (1ULL<<((x)%8))))
#define BIT_CLEAR(b, x) (b[(x)/8] &= ~(1ULL<<((x)%8)))

#define BIT_I(i, x) (i & (1ULL << x))

namespace Kernel {

namespace PM {
    mutex_t mutex;
    uint64_t used_pages = 0;
    uint64_t free_pages = 0;

    struct descriptors {
        uint64_t base;
        uint64_t size;
        uint64_t hint_offset; // Used to speed up most allocs
        descriptors* next;
    }__attribute__((packed));

    descriptors* pages;
    uint64_t virtual_offset = 0;

    void Init(stivale2_struct_tag_memmap* memmap, uint64_t hddm_offset) {
        virtual_offset = hddm_offset;
        // Get the biggest contigous segemnt
        uint64_t biggest_base = 0;
        uint64_t biggset_size = 0;
        for(size_t i = 0; i < memmap->entries; i++) {
            if(memmap->memmap[i].type == STIVALE2_MMAP_USABLE) {
                // Check if this is bigger
                if(memmap->memmap[i].length > biggset_size) {
                    // New biggest
                    biggest_base = memmap->memmap[i].base;
                    biggset_size = memmap->memmap[i].length;
                }
            }
        }
        if(!biggset_size) {
            stivale2_term_write("failed to initialize pm: memmap does not contain any valid usabale entries\n");
            for(;;);
        }
        // Great, make this the base of the descriptors
        // Add the virtual offset first though
        pages = (descriptors*)(biggest_base + virtual_offset);
        descriptors* curr = NULL;
        uint64_t current_descriptor_length = 0;
        for(size_t i = 0; i < memmap->entries; i++) {
            if(memmap->memmap[i].type == STIVALE2_MMAP_USABLE) {
                if(memmap->memmap[i].base == biggest_base) { continue; }
                if(curr == NULL) { curr = pages; } else { curr = curr->next; }
                // Create a descriptor for this one
                curr->base = memmap->memmap[i].base;
                curr->size = memmap->memmap[i].length;
                free_pages += curr->size / 4096;
                // Calculate bitmap size
                uint64_t bitmap_size = curr->size / 4096;
                // Zero the bitmap
                memset(curr + 1, 0, bitmap_size);
                // Set hint to 0
                curr->hint_offset = 0;
                // Calculate next position
                curr->next = (descriptors*)((uint64_t)(curr) + sizeof(descriptors) + bitmap_size);
                current_descriptor_length += sizeof(descriptors) + bitmap_size;
            }
        }
        // We went through all of them, now do the biggest one
        curr = curr->next;
        curr->base = biggest_base + (((current_descriptor_length) + 4095) & (~(4095)));
        curr->size = biggset_size - (((current_descriptor_length) + 4095) & (~(4095)));
        curr->next = 0;
        curr->hint_offset = 0;
        free_pages += curr->size / 4096;
    }

    void MapPhysical() {
        descriptors* curr = pages;
        // Basically just loop through each of the usable memory areas and map them
        while(curr) {
            for(uint64_t curr_base = curr->base; curr_base < (curr->base + curr->size); curr_base += 4096) {
                VM::MapPage(curr_base, curr_base + virtual_offset);
            }
            curr = curr->next;
        }
    }

    inline int64_t CheckAndAllocate(uint64_t* bitmap_entry, descriptors* curr, uint64_t offset, int count, bool hint) {
        if(__builtin_popcountl(~(*bitmap_entry)) >= count) {
            // We might be able to squeeze this in, check if we can get this in
            int current_block = 0;
            int current_block_pos = 0;
            for(size_t i = 0; i < 64; i++) {
                if(BIT_I(*bitmap_entry, i)) {
                    current_block = 0;
                    current_block_pos = i + 1;
                } else {
                    current_block++;
                }
                // Check if we have a block thats big enough
                if(current_block >= count) {
                    // Ladies and gentlemen, we got em
                    // Set these bits as used
                    *bitmap_entry |= ((1 << current_block) - 1) << current_block_pos;
                    // Calculate offset
                    uint64_t addr = curr->base + (((offset * 64) + current_block_pos) * 4096);
                    // Check if the hint is now full
                    if(hint && *bitmap_entry == 0xFFFFFFFFFFFFFFFF) {
                        // This hint is full, check if we can move it
                        if(curr->size < ((curr->hint_offset + 1) * 64 * 4096)) {
                            curr->hint_offset++;
                        }
                    }
                    used_pages += count;
                    free_pages -= count;
                    return addr;
                }
            }
        }
        return -1;
    }

    uint64_t AllocatePages(int count) {
        // Acquire physical memory mutex
        acquire(&mutex);
        if(count >= 64) { Debug::Panic("PM: TODO: allocate more than 64 pages at once"); }
        // We have a slightly faster but less space efficent method to allocate less than 64 pages using POPCNT
        // Since most of the memory will be allocated with VM::AllocatePages, which allocates single pages,
        // this wont really be a problem.
        descriptors* curr = pages;
        while(curr) {
            uint64_t* bitmap = (uint64_t*)(curr + 1);
            // Check the hint
            int64_t hint_ret = CheckAndAllocate((bitmap + curr->hint_offset), curr, curr->hint_offset, count, true);
            if(hint_ret != -1) { release(&mutex); return (uint64_t)hint_ret; }
            // Hint has now failed us, check each uint64_t after the hint first
            for(size_t i = (curr->hint_offset + 1); i < ((curr->size / 4096) / 64); i++) {
                int64_t ret = CheckAndAllocate((bitmap + i), curr, i, count, false);
                if(ret != -1) { release(&mutex); return (uint64_t)ret; }
            }
            // Now check everything before the hint
            for(size_t i = 0; i < curr->hint_offset; i++) {
                int64_t ret = CheckAndAllocate((bitmap + i), curr, i, count, false);
                if(ret != -1) { release(&mutex); return (uint64_t)ret; }
            }
            // Completley failed us, advance to the next entry in the LL
            curr = curr->next;
        }
        release(&mutex);
        Debug::Panic("PM: no memory left!");
    }

    bool CheckIOSpace(uint64_t phys, uint64_t size) {
        return true;
        (void)phys; (void)size;
    }

    void FreePages(uint64_t object, int count) {
        descriptors* curr = pages;
        while(curr) {
            uint64_t* bitmap = (uint64_t*)(curr + 1);
            // Check if the object is in this page list
            if(object >= curr->base && object <= (curr->base + curr->size)) {
                // Got it, set the bits to 0
                size_t page_offset = (object - curr->base) / 4096;
                for(int i = 0; i < count; i++) {
                    BIT_CLEAR(bitmap, page_offset + 1);
                }
                free_pages += count;
                used_pages -= count;
                release(&mutex);
                return;
            }
            curr = curr->next;
        }
        KLog::the().printf("PM: couldnt deallocate page %x, count %i, ignoring\n\r", object, count);
    }

    void PrintMemUsage() {
        KLog::the().printf("PM: used pages %i, free pages %i, used mem %iMb\n\r", used_pages, free_pages, (used_pages * 4096) / (1024 * 1024));
    }

    uint64_t PageCount() { return free_pages + used_pages; }
}

}