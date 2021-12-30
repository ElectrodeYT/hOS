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
#include <kernel-drivers/VFS.h>

namespace Kernel {
    void kInit2(void* arg) {
        KLog::the().printf("kInit2: started init");
        // We mount the first partition we find
        for(uint64_t mount_device = 0; true; mount_device++) {
            // Attempt to get the base device
            // If we can get this, then the device exists
            BlockDevice* base_dev = BlockManager::the().GetBlockDevice(mount_device, 0);
            if(base_dev) {
                // Device exists, try to mount every partition here
                bool mounted = false;
                for(size_t part_id = 0; true; part_id++) {
                    BlockDevice* part_dev = BlockManager::the().GetBlockDevice(mount_device, part_id);
                    // We only support EchFS Root partitions
                    EchFSDriver* driver = new EchFSDriver;
                    driver->block = part_dev;
                    if(!VFS::the().attemptMountRoot(driver)) {
                        // Didnt work
                        delete driver;
                    } else {
                        // Did work, break out
                        mounted = true;
                        break;
                    }
                }
                if(mounted) { break; }
            } else {
                // We have no device left!
                Debug::Panic("Unable to mount root");
            }
        }
        // We have mounted root, try to read hello.txt lol
        VFS::fs_node* root = VFS::the().getRootNode();
        if(!root) { Debug::Panic("what the fuck 1"); }
        VFS::fs_node* hello = root->finddir("hello.txt");
        if(!hello) { Debug::Panic("what the fuck 2"); }
        char hello_text[50];
        memset(hello_text, 0, 50);
        hello->open(true, false);
        hello->read(hello_text, 50, 0);
        hello->close();
        KLog::the().printf("%s\n\r", hello_text);
        for(;;);
        (void)arg;
    }

    void KernelMain() {
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

        // Read partition tables
        BlockManager::the().ParsePartitions();

        // Spawn bootstrap processes
        //for(size_t i = 0; i < mod_count; i++) {
        //    Processes::Scheduler::the().CreateProcess(modules[i], module_sizes[i], module_names[i]);
        //}
        // Spawn kernel tasks
        Processes::Scheduler::the().CreateKernelTask(kInit2, NULL, 4);
        // Schedule
        Processes::Scheduler::the().FirstSchedule();
        for(;;);
    }
}
