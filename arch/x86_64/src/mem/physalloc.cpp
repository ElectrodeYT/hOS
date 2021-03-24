#include <mem.h>
#include <mem/physalloc.h>

static physmem_ll* memmap;

void __init_physical_allocator(physmem_ll* memory_map) {
    memmap = memory_map;
    // Calculate size of bitmap and allocate it
    physmem_ll* curr = memory_map;
    while(curr != NULL) {
        if(curr->type == PHYSMEM_LL_TYPE_USEABLE) {
            // Calculate size of the bitmap
            size_t bitmap_size = (curr->size / 4096);
            bitmap_size /= 8; // 8 bits in a byte
            // Allocate it
            curr->bitmap = new uint8_t[bitmap_size];
            memset((void*)curr->bitmap, 0x00, bitmap_size);
        } else {
            // Set it to null to indicate that none was allocated
            curr->bitmap = NULL;
        }
        curr = curr->next;
    }
}

void* __physmem_allocate_page(int count) {
    // Current linked list to check
    physmem_ll* curr = memmap;
    while(curr != NULL) {
        if(curr->type != PHYSMEM_LL_TYPE_USEABLE) { curr = curr->next; continue; }
        // Go through the bitmap
        if(curr->bitmap == NULL) { for(;;); } // TODO: panic
        for(size_t page = 0; page < (curr->size / 4096); page++) {
            // Calculate byte and bit position
            int byte = page / 8;
            int bit = page % 8;

            // Check the bit
            if(!((curr->bitmap[byte] >> bit) & 1)) {
                // Check if there are enough consecutive pages available
                bool enough = true;
                int page_failed = 0;
                for(size_t check_page = page; check_page < (page + count); check_page++) {
                    // Really fancy shit
                    if((curr->bitmap[check_page / 8] >> (check_page % 8)) & 1) { enough = false; page_failed = check_page; break; }
                }
                if(!enough) {
                    // Not enough consecutive pages available
                    page = page_failed + 1;
                    continue;
                }
                // Enough pages are available
                for(size_t page_set = page; page_set < (page + count); page_set++) {
                    curr->bitmap[page_set / 8] |= 1 << (page_set % 8);
                }
                // Calculate return address and return
                return (void*)(curr->base + (page * 4096));
            }
        }
    }
    return NULL; // No memory available
}

void __physmem_free_page(void* page, int count) {
    // Get memory map where this page is set in
    physmem_ll* curr = memmap;
    while(curr != NULL) {
        if(curr->base <= (uint64_t)page && (curr->base + curr->size) > (uint64_t)page) {
            // This is the memory mapping in which this page sits
            if(curr->bitmap != NULL) {
                for(int i = 0; i < count; i++) {
                    int page_set = (((uint64_t)page - curr->base) / 4096);
                    curr->bitmap[(page_set + i) / 8] &= ~(1 << ((page_set + i) % 8));
                }
            }
            return;
        }
        curr = curr->next;
    }
}