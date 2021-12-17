#include <mem.h>
#include <kmain.h>
#include <panic.h>
#include <debug/serial.h>
#include <timer.h>
#include <mem/VM/virtmem.h>
#include <processes/scheduler.h>
#include <kernel-drivers/PCI.h>
namespace Kernel {
    void KernelMain(size_t mod_count, uint8_t** modules, uint64_t* module_sizes, char** module_names) {
        ASSERT(mod_count > 0, "No bootstrap elf loaded as module");
        // The main job of KernelMain() is to initalize other important parts of the OS that require a decent enviroment to run in.
        // Alot of stuff here will be in agnostic, with calls into the arch code
        Hardware::Timer::InitTimer();

        // We can now enable interrupts
        asm volatile("sti");

        // Initialize scheduler
        Processes::Scheduler::the().Init();

        // Probe PCI devices
        PCI::the().probe();

        // Spawn bootstrap processes
        for(size_t i = 0; i < mod_count; i++) {
            Processes::Scheduler::the().CreateProcess(modules[i], module_sizes[i], module_names[i]);
        }
        
        // Schedule
        Processes::Scheduler::the().FirstSchedule();
        for(;;);
    }
}
