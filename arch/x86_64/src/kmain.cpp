#include <mem.h>
#include <kmain.h>
#include <panic.h>
#include <debug/serial.h>
#include <timer.h>
#include <mem/VM/virtmem.h>
#include <processes/scheduler.h>
#include <processes/vfs.h>

extern "C" char testa_start;
extern "C" char testa_end;

extern "C" char testb_start;
extern "C" char testb_end;

namespace Kernel {
    void KernelMain() {
        // The main job of KernelMain() is to initalize other important parts of the OS that require a decent enviroment to run in.
        // Alot of stuff here will be in agnostic, with calls into the arch code
        Hardware::Timer::InitTimer();

        // We can now enable interrupts
        asm volatile("sti");

        // TODO: Read modules passed into the kernel by stivale2
        
        // Initialize scheduler
        Processes::Scheduler::the().Init();

        // Spawn VFS service
        Processes::Scheduler::the().CreateService((uint64_t)Services::VFS::entryPoint, "vfs");

        // Copy out the testa into a new thing
        uint64_t size_of_testa_page_aligned = round_to_page_up((uint64_t)&testa_end - (uint64_t)&testa_start);
        uint8_t* test_a = (uint8_t*)VM::Manager::the().AllocatePages(size_of_testa_page_aligned / 4096);
        memcopy((void*)((uint64_t)&testa_start), test_a, (uint64_t)&testa_end - (uint64_t)&testa_start);

        uint64_t size_of_testb_page_aligned = round_to_page_up((uint64_t)&testb_end - (uint64_t)&testb_start);
        uint8_t* test_b = (uint8_t*)VM::Manager::the().AllocatePages(size_of_testb_page_aligned / 4096);
        memcopy((void*)((uint64_t)&testb_start), test_b, (uint64_t)&testb_end - (uint64_t)&testb_start);


        // Create test processes
        Processes::Scheduler::the().CreateProcess(test_a, size_of_testa_page_aligned, 0x8000, "testa", false);
        Processes::Scheduler::the().CreateProcess(test_b, size_of_testb_page_aligned, 0x8000, "testb", false);

        // Schedule
        Processes::Scheduler::the().FirstSchedule();
        for(;;);
    }
}