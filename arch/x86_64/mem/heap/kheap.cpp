#include <mem/heap/kheap.h>
#include <mem/VM/virtmem.h>
#include <stdint.h>
#include <panic.h>
#include <CPP/mutex.h>

mutex_t mutex;

int liballoc_lock() {
    acquire(&mutex);
    return 0;
}

int liballoc_unlock() {
    release(&mutex);
    return 0;
}

void* liballoc_alloc(size_t count) {
    return Kernel::VM::AllocatePages(count);
}

int liballoc_free(void* ptr, size_t count) {
    Kernel::VM::FreePages(ptr);
    (void)count;
    return 0;
}