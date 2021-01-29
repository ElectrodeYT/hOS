#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>
#include <stddef.h>

// The kernel heap is _not_ allocated in free memory. It is allocated into the .heap section.
// This is not really designed for large allocations.

// This should be called before the global constructors, so that they have a malloc() function to work with.
int heap_init();

[[gnu::malloc, gnu::returns_nonnull, gnu::alloc_size(1)]] void* kmalloc_imp(uint32_t size);
int kfree(void* point);

// C++ stuff
inline void* operator new(size_t, void* p) { return p; }
inline void* operator new[](size_t, void* p) { return p; }
inline void* operator new(size_t size)          { return kmalloc_imp(size); }
inline void* operator new[](size_t size)          { return kmalloc_imp(size); }

[[gnu::malloc, gnu::returns_nonnull, gnu::alloc_size(1)]] inline void* kmalloc(uint32_t size) {
    return kmalloc_imp(size);
}


#endif