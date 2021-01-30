#include <stdint.h>
#include <hardware/tables.h>
#include <hardware/interrupts.h>
#include <memory/heap.h>
#include <memory/memorymanager.h>
#include <multiboot/multiboot.h>
#include <debug/debug_print.h>

typedef void (*ctor_constructor)();
extern "C" ctor_constructor start_ctors;
extern "C" ctor_constructor end_ctors;


namespace Kernel {


extern "C" void kmain(void* info, uint32_t multiboot_magic) {
	// Init kmalloc
	heap_init();
	// Init Page frame allocator.
	MemoryManager::PageFrameAllocator::InitPageFrameAllocator(info);
	// Init Virtual memory.
	MemoryManager::VirtualMemory::InitVM();

	// Call the global constructors
	for (ctor_constructor* ctor = &start_ctors; ctor < &end_ctors; ctor++)
		(*ctor)();


	for(int i = 0; i < 10; i++) {
		uint32_t page = (uint32_t)MemoryManager::PageFrameAllocator::AllocatePage();
		debug_puti(page, 16); debug_puts("\n\r");
	}


	while(true) { } // hang here for a bit
}

}