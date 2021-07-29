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


// (void)(idk) is to supress unused parameter warnings
void operator delete(void* p, unsigned long idk) {
    (void)(idk);
    kheap_free(p);
}

void operator delete[](void* p, unsigned long idk) {
    (void)(idk);
    kheap_free(p);
}

void* malloc(unsigned long size) {
    return kheap_alloc(size);
}

void free(void* p) {
    kheap_free(p);
}

#endif

void memset(void* dst, uint8_t val, unsigned long size) {
    for(unsigned long i = 0; i < size; i++) {
        ((uint8_t*)dst)[i] = val;
    }
}

void memcopy(void* src, void* dst, unsigned long size) {
    for(unsigned long i = 0; i < size; i++) {
        ((uint8_t*)dst)[i] = ((uint8_t*)src)[i];
    }
}