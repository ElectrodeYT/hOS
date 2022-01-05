#include <mem/heap/kheap.h>
#include <mem/VM/virtmem.h>
#include <stdint.h>
#include <panic.h>
#include <CPP/mutex.h>
#include <debug/klog.h>
#include <debug/serial.h>

mutex_t mutex;

int liballoc_lock() {
    acquire(&mutex);
    return 0;
}

int liballoc_unlock() {
    release(&mutex);
    return 0;
}

// We use DebugSerial here, as klog will be caught in a mutex otherwise

void* liballoc_alloc(size_t count) {
    #if defined(KHEAP_LOG_PAGE_ALLOC) && KHEAP_LOG_PAGE_ALLOC
    Kernel::Debug::SerialPrintf("[kheap]: requesting %i pages\n\r", count);
    #endif
    return Kernel::VM::AllocatePages(count);
}

int liballoc_free(void* ptr, size_t count) {
    #if defined(KHEAP_LOG_PAGE_FREE) && KHEAP_LOG_PAGE_FREE
    Kernel::Debug::SerialPrintf("[kheap]: deallocating %i pages at %x\n\r", count, (uint64_t)ptr);
    #endif
    Kernel::VM::FreePages(ptr, count);
    return 0;
}