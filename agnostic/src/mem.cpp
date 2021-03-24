#include <mem.h>
#ifndef NKERNEL
#include <mem/heap/kheap.h>

void* operator new(size_t size) {
    return kheap_alloc(size);
}
 
void* operator new[](size_t size) {
    return kheap_alloc(size);
}
 
void operator delete(void* p) {
    kheap_free(p);
}
 
void operator delete[](void* p) {
    kheap_free(p);
}

#endif