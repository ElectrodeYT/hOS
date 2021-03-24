#ifndef AGNOSTIC_MEM_H
#define AGNOSTIC_MEM_H

// Agnostic memory define, defines new and stuff
// Basically maps it to the correct heap allocator

#ifndef NKERNEL
#include <mem/heap/kheap.h>
inline static void __init_heap() { kheap_init(); }
#endif

void* operator new(size_t size);
 
void* operator new[](size_t size);
 
void operator delete(void* p);
 
void operator delete[](void* p);

#endif