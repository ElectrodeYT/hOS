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
	// Init GDT
	GDT::InitGDT();
	// Init Virtual memory.
	MemoryManager::VirtualMemory::InitVM();


	int test1 = MemoryManager::VirtualMemory::CheckIfPageIsMapped(MemoryManager::KernelVirtualBase);
	int test2 = MemoryManager::VirtualMemory::CheckIfPageIsMapped(0);

	debug_puti(test1); debug_puts("\n\r");
	debug_puti(test2); debug_puts("\n\r");

	// Call the global constructors
	for (ctor_constructor* ctor = &start_ctors; ctor < &end_ctors; ctor++)
		(*ctor)();


	// Initialize interrupts
	Interrupts::SetupInterrupts();

	for(int i = 0; i < 10; i++) {
		uint32_t page = (uint32_t)MemoryManager::VirtualMemory::GetPage();
		debug_puti(page, 16); debug_puts("\n\r");
	}

	// Trigger page fault
	uint32_t* page_fault = (uint32_t*)0x41414141;
	debug_puti(*page_fault, 16);
	while(true) { } // hang here for a bit
}

}