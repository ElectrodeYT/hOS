#ifndef KHEAP_H
#define KHEAP_H

#include <stddef.h>

// We leave the heap allocator in global space, it should be accesible everywhere

void* kheap_alloc(size_t size);
void kheap_free(void* adr);
void kheap_init();

#endif