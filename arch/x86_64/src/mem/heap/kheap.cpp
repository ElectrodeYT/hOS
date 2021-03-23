#include <mem/heap/kheap.h>
#include <stdint.h>

// Define space for the heap
uint8_t heap[4 * 1024 * 1024] __attribute__((section(".heap")));
bool heap_initialized = false;

void* kheap_alloc(size_t size) {
    if(heap_initialized == false) { for(;;); }
}

void kheap_free(void* adr) {
    if(heap_initialized == false) { for(;;); }
    
}

void kheap_init() {
    heap_initialized = true;
}