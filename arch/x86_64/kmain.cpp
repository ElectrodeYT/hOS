#include <mem.h>
#include <kmain.h>
#include <panic.h>
#include <debug/serial.h>
#include <debug/klog.h>
#include <timer.h>
#include <mem/VM/virtmem.h>
#include <mem/PM/physalloc.h>
#include <processes/scheduler.h>
#include <kernel-drivers/PCI.h>
#include <kernel-drivers/IDE.h>
#include <kernel-drivers/BlockDevices.h>
#include <kernel-drivers/CharDevices.h>
#include <kernel-drivers/VFS.h>
#include <kernel-drivers/PS2.h>
#include <kernel-drivers/Stivale2GraphicsTerminal.h>

namespace Kernel {
    void kInit2(void* arg) {
        KLog::the().printf("kInit2: started init\n\r");
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
        // Try to mount devfs
        DevFSDriver* devfs = new DevFSDriver;
        VFS::the().attemptMountOnFolder("/", "dev", devfs);

        // See if we can launch /init
        int64_t init_fd = VFS::the().open("/", "init", -1);
        if(init_fd < 0) { Debug::Panic("Unable to start init"); }
        uint64_t init_size = VFS::the().size(init_fd, -1);
        if(init_size == 0) { Debug::Panic("Init elf size == 0"); }
        uint8_t* init_elf = new uint8_t[init_size];
        VFS::the().pread(init_fd, init_elf, init_size, 0, -1);
        VFS::the().close(init_fd, -1);
        PM::PrintMemUsage();
        Processes::Scheduler::the().CreateProcess(init_elf, init_size, "init", "/", true);
        Processes::Scheduler::the().KillCurrentProcess();
        for(;;);
        (void)arg;
    }

    void KernelMain(stivale2_struct_tag_framebuffer* fb) {
        // The main job of KernelMain() is to initalize other important parts of the OS that require a decent enviroment to run in.
        // Alot of stuff here will be in agnostic (ideally, unlikely to happen), with calls into the arch code
        Hardware::Timer::InitTimer();

        // We can now enable interrupts
        asm volatile("sti");

        if(fb) {
            Stivale2GraphicsTerminal* stivale2_terminal = new Stivale2GraphicsTerminal();
            if(stivale2_terminal && stivale2_terminal->init(fb)) {
                stivale2_terminal->Clear();
                CharDeviceManager::the().RegisterCharDevice(stivale2_terminal);
            }
        }

        KLog::the().printf("Starting hOS Kernel\n\r");
        KLog::the().printf("Physical RAM available: %iMb\n\r", (PM::PageCount() * 4096) / (1024 * 1024));

        // Initialize scheduler
        Processes::Scheduler::the().Init();

        // Probe PCI devices
        PCI::the().probe();

        // Read partition tables
        BlockManager::the().ParsePartitions();

        // Init PS2
        // We have to do this here because of our shitty timing and scheduler
        // TODO: improve timer/add scheduler sleep support
        PS2::the().Init();
        
        // Spawn kernel tasks
        Processes::Scheduler::the().CreateKernelTask(kInit2, NULL, 4);
        // Schedule
        Processes::Scheduler::the().FirstSchedule();
        for(;;);
    }
}
