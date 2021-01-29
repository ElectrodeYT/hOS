#include <stdint.h>
#include <debug/debug_print.h>
#include <memory/heap.h>

#define HEAP_MEMORY_POOL (2 * 1024 * 1024)
#define HEAP_BLOCK_SIZE 8

#define HEAP_BITMAP_SIZE (HEAP_MEMORY_POOL / HEAP_BLOCK_SIZE) / 8
#define HEAP_BITMAP_BITS_COUNT (HEAP_MEMORY_POOL / HEAP_BLOCK_SIZE)

#define BITMAP_GET_BIT(bitmap, bit) ( (bitmap[bit / 8] >> (bit % 8)) & 1 )
#define BITMAP_SET_BIT(bitmap, bit) ( (bitmap[bit / 8] |= (1 << (bit % 8))) )
#define BITMAP_CLEAR_BIT(bitmap, bit) ( (bitmap[bit / 8] &= ~(1 << (bit % 8))) )

// The heap pool is where the allocated memory lives.
uint8_t heap_pool[HEAP_MEMORY_POOL] __attribute__((section(".heap")));


uint8_t heap_bitmap[HEAP_BITMAP_SIZE] __attribute__((section(".heap_metadata")));


int heap_init() {
    // Zero out the heap mem
    for(int i = 0; i < HEAP_MEMORY_POOL; i++) {
        heap_pool[i] = 0;
    }

    Kernel::debug_puts("Zeroed pool\n\r");

    // Zero out the bitmap data
    for(int i = 0; i < HEAP_BITMAP_SIZE; i++) {
        heap_bitmap[i] = 0;
    }

    Kernel::debug_puts("Zeroed metadata\n\r");
    return 0;
}

void* kmalloc_imp(uint32_t size) {
    Kernel::debug_puts("kmalloc: allocating 0x"); Kernel::debug_puti(size, 16); Kernel::debug_puts("bytes\n\r");
    if(size == 0) { Kernel::debug_puts("kmalloc: attempted allocation of 0 area!\n\r"); return nullptr; }
    // Calculate chunk count
    int chunk_count = size / HEAP_BLOCK_SIZE;
    if(size % HEAP_BLOCK_SIZE) { chunk_count++; }
    // Loop through the chunks
    for(int i = 0; i < HEAP_BITMAP_BITS_COUNT; i++) {
        int bit = BITMAP_GET_BIT(heap_bitmap, i);
        // Is it free?
        if(bit != 0) { continue; }
        // Check if there are enough free chunks afterwards
        int free_chunk_count = 1;
        while(BITMAP_GET_BIT(heap_bitmap, i + free_chunk_count) == 0 && free_chunk_count != chunk_count) {
            free_chunk_count++;
        }
        if(free_chunk_count != chunk_count) { i += free_chunk_count - 1; continue; } // Not enough free chunks available

        // Enough free chunks are available
        // Allocate the chunks
        for(int x = 0; x < chunk_count; x++) {
            BITMAP_SET_BIT(heap_bitmap, i + x);
        }

        // Return pointer
        return (void*)(heap_bitmap + (i * HEAP_BLOCK_SIZE));
    }

    // Allocation failed, return null
    return nullptr;
}
