#include <mem/heap/kheap.h>
#include <mem/VM/virtmem.h>
#include <stdint.h>
#include <panic.h>
#include <CPP/mutex.h>

// This heap implementation is slow and bad but it works for now
// TODO: implemnt heap expansion, prefarably in the megabyte range


// Define space for the initial heap
// Doesnt need to be the biggest thing ever yet
// TODO: Define the minimum size for this that would boot the kernel into a state that allows expansion of the heap
static uint8_t heap[1 * 1024 * 1024] __attribute__((section(".heap")));
bool heap_initialized = false;

// Linked list on the heap
// Should allow for future expansion
struct kheap_ll {
    size_t size;
    #define KHEAP_LL_TYPE_FREE 0
    #define KHEAP_LL_TYPE_USED 1
    uint8_t type;
    kheap_ll* next;
} __attribute__((packed));

mutex_t kheap_mutex;

kheap_ll* spare;

// TODO: use space better
void* kheap_alloc(size_t size) {
    bool must_allocate_new_spare = false;
    acquire(&kheap_mutex);
    if(heap_initialized == false) { for(;;); }
    // Traverse the ll to find a free block
    kheap_ll* curr = (kheap_ll*)&heap;
    while(true) {
        if(curr->type == KHEAP_LL_TYPE_USED) {
            fail:
            // If curr->next == NULL no more space is available
            // Use the spare area, and attempt to allocate a new spare area
            // If we cant, just crash
            if(curr->next == 0) {
                if(spare->size < size) { Kernel::Debug::Panic("spare area too small"); }
                if(spare == NULL) { Kernel::Debug::Panic("no kheap spare area"); }
                curr->next = spare;
                spare = NULL;
                must_allocate_new_spare = true;
            }
            curr = curr->next;
        } else {
            // Check size
            if(curr->size < size) {
                // People will hate me for this
                goto fail;
            }
            // This place should work, calculate pointer
            void* return_val = (void*)((uint64_t)curr + sizeof(kheap_ll));
            // Create next linked list
            curr->type = KHEAP_LL_TYPE_USED;
            // Check if its even worth or possible to create the next linked list
            if(curr->size < (size + sizeof(kheap_ll) + 10)) { // Arbitraly chosen number
                // Just return now
                release(&kheap_mutex);
                // Check if we need to reallocate a new spare area first
                if(must_allocate_new_spare) { kheap_init_spare_area(); }
                return return_val;
            }
            // Calculate the location of the next linked list
            kheap_ll* next_ll = (kheap_ll*)((void*)((uint64_t)curr + size + sizeof(kheap_ll))); // Convert curr to void* to avoid a nasty bug
            next_ll->next = curr->next;
            next_ll->size = curr->size - size - sizeof(kheap_ll);
            next_ll->type = KHEAP_LL_TYPE_FREE;
            // Update size of curr and next
            curr->next = next_ll;
            curr->size = size;
            release(&kheap_mutex);
            // Check if we need to reallocate a new spare area first
            if(must_allocate_new_spare) { kheap_init_spare_area(); }
            // Return now
            return return_val;
        }
    }
}

// TODO: combine backwards
//       Probably needs adjustmants to kheap_ll for that
void kheap_free(void* adr) {
    acquire(&kheap_mutex);
    if(heap_initialized == false) { for(;;); }
    // Calculate ll address
    kheap_ll* ll = (kheap_ll*)((uint64_t)adr - sizeof(kheap_ll));
    // Set type to free
    ll->type = KHEAP_LL_TYPE_FREE;
    // Check wether the next block is directly after it (allows for heap expansion)
    combine:
    if((uint64_t)(ll->next) == ((uint64_t)ll + ll->size + sizeof(kheap_ll))) {
        if(ll->next->type == KHEAP_LL_TYPE_FREE) {
            // Add the next one to the size of the ll
            ll->size += ll->next->size + sizeof(kheap_ll);
            ll->next = ll->next->next;
            // Repeat as much as possible
            goto combine;
        }
    }
    release(&kheap_mutex);
}

void kheap_init() {
    // Create initial linked list
    kheap_ll* ll = (kheap_ll*)heap;
    ll->size = sizeof(heap) - sizeof(kheap_ll);
    ll->type = KHEAP_LL_TYPE_FREE;
    ll->next = NULL;

    heap_initialized = true;
}


void kheap_init_spare_area() {
    // Allocate spare ll entry
    spare = NULL;
    spare = (kheap_ll*)Kernel::VM::AllocatePages((1 * 1024 * 1024) / 4096);
    if(!spare) { Kernel::Debug::Panic("failed to allocate kheap spare area"); }
    spare->next = NULL;
    spare->size = (1 * 1024 * 1024) - sizeof(kheap_ll);
    spare->type = KHEAP_LL_TYPE_FREE;
}