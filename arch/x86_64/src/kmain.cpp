#include <mem.h>
#include <kmain.h>
#include <panic.h>
#include <timer.h>
#include <mem/VM/virtmem.h>
#include <processes/scheduler.h>

namespace Kernel {
    void KernelMain() {
        // The main job of KernelMain() is to initalize other important parts of the OS that require a decent enviroment to run in.
        // Alot of stuff here will be in agnostic, with calls into the arch code
        
        Hardware::Timer::InitTimer();
        // We can now enable interrupts
        asm volatile("sti");
        // TODO: Read modules passed into the kernel by stivale2
        // TODO: VFS
        
        // Initialize scheduler
        Processes::Scheduler::the().Init();

        for(;;);
    }
}