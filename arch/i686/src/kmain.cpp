#include <stdint.h>
#include <memory/heap.h>
#include <memory/memorymanager.h>
#include <multiboot/multiboot.h>
#include <debug/debug_print.h>

typedef void (*ctor_constructor)();
extern "C" ctor_constructor start_ctors;
extern "C" ctor_constructor end_ctors;


namespace Kernel {


extern "C" void kmain(void* info, uint32_t multiboot_magic) {
	// Initialize heap
	// We want to do this before the constructors are called!
	heap_init();

	// Call the global constructors
	for (ctor_constructor* ctor = &start_ctors; ctor < &end_ctors; ctor++)
		(*ctor)();

	// Init Page frame allocator.
	MemoryManager::PageFrameAllocator::InitPageFrameAllocator(info);
	

	// Init serial port
	debug_serial_init();
	debug_puts("Hello from kmain\n\r");
	debug_puti(0xABCDEF, 16);

	while(true) { } // hang here for a bit
}

}