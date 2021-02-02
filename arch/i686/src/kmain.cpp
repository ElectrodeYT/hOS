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

		// Call the global constructors
		for (ctor_constructor* ctor = &start_ctors; ctor < &end_ctors; ctor++)
			(*ctor)();

		// Init Page frame allocator.
		MemoryManager::PageFrameAllocator::InitPageFrameAllocator(info);
		// Init GDT
		GDT::InitGDT();
		// Init Virtual memory.
		MemoryManager::VirtualMemory::InitVM();
		// Init TSS
		TSS::InitTSS();
		// Initialize interrupts
		Interrupts::SetupInterrupts();

		MemoryManager::VirtualMemory::Mapping new_pd = MemoryManager::VirtualMemory::CreatePageDirectory();
		debug_puti(new_pd.phys, 16);
		while(true) { } // hang here for a bit
	}
}