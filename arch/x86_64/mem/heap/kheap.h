#ifndef KHEAP_H
#define KHEAP_H

#include <stddef.h>
#include <mem/heap/liballoc/liballoc_1_1.h>

// TODO: not steal liballoc
// Im too lazy for this right now, im just using liballoc to get everything working
void* kheap_alloc(size_t size);
void kheap_free(void* adr);

// Heap debug defines
#define KHEAP_LOG_PAGE_ALLOC 0
#define KHEAP_LOG_PAGE_FREE 0

#endif