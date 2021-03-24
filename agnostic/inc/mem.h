#ifndef AGNOSTIC_MEM_H
#define AGNOSTIC_MEM_H

// Agnostic memory define, defines new and stuff
// Basically maps it to the correct heap allocator

#ifndef NKERNEL
#include <mem/heap/kheap.h>
inline static void __init_heap() { kheap_init(); }
#endif

#include <stddef.h>
#include <stdint.h>

void* operator new(size_t size);
 
void* operator new[](size_t size);
 
void operator delete(void* p);
 
void operator delete[](void* p);

void operator delete(void* p, unsigned long idk);

void operator delete[](void* p, unsigned long idk);

void memset(void* dst, uint8_t val, unsigned long size);

#endif