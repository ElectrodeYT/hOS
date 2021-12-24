#include <mem.h>
#include <kmain.h>
#include <panic.h>
#include <debug/serial.h>
#include <debug/klog.h>
#include <timer.h>
#include <mem/VM/virtmem.h>
#include <processes/scheduler.h>
#include <kernel-drivers/PCI.h>
#include <kernel-drivers/IDE.h>
#include <kernel-drivers/BlockDevices.h>

namespace Kernel {
    void kTaskTest(void* arg) {
        KLog::the().printf("it works!\n\r");
        KLog::the().printf("Test read from ide drive 0\n\r");
        BlockDevice* dev = BlockManager::the().GetBlockDevice(0);
        if(!dev) {
            KLog::the().printf("BlockManager return null. what the fuck?\n\r");
        } else {
            uint8_t* buffer = new uint8_t[1024];
            dev->read(buffer, 1024, 0);
            if(buffer[512] == 0x45 && buffer[513] == 0x46 && buffer[514] == 0x49) {
                KLog::the().printf("EFI seems to be at the start of LBA1. Success!\n\r");
            }
        }
        for(;;);
        (void)arg;
    }

    void KernelMain(size_t mod_count, uint8_t** modules, uint64_t* module_sizes, char** module_names) {
        ASSERT(mod_count > 0, "No bootstrap elf loaded as module");
        // The main job of KernelMain() is to initalize other important parts of the OS that require a decent enviroment to run in.
        // Alot of stuff here will be in agnostic, with calls into the arch code
        Hardware::Timer::InitTimer();

        KLog::the().printf("hello world!\n\r");
        KLog::the().printf("the number 42: %i\n\r", 42);

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
        // Spawn kernel tasks
        Processes::Scheduler::the().CreateKernelTask(kTaskTest, NULL, 4);
        // Schedule
        Processes::Scheduler::the().FirstSchedule();
        for(;;);
    }
}
