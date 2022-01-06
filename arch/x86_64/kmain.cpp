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
        // Try to mount devfs
        DevFSDriver* devfs = new DevFSDriver;
        VFS::the().attemptMountOnFolder("/", "dev", devfs);

        // We have mounted root, try to read hello.txt lol
        PM::PrintMemUsage();
        int hello_fd = VFS::the().open("/", "hello.txt", -1);
        if(hello_fd < 0) { KLog::the().printf("what the fuck 1\n\r"); for(;;); }
        size_t hello_size = VFS::the().size(hello_fd, -1);
        char* buf = new char[hello_size + 1];
        memset(buf, 0, hello_size + 1);
        if(VFS::the().pread(hello_fd, buf, hello_size, 0, -1) < 0) { KLog::the().printf("what the fuck 2\n\r"); for(;;); }
        PM::PrintMemUsage();
        // KLog::the().printf("%s\n\r", buf);
        VFS::the().close(hello_fd, -1);
        
        // Lets try to launch a elf from vfs
        PM::PrintMemUsage();
        int64_t testa_fd = VFS::the().open("/", "testa.elf", -1);
        if(testa_fd < 0) { KLog::the().printf("what the fuck 3\n\r"); for(;;); }
        size_t testa_size = VFS::the().size(testa_fd, -1);
        if(!testa_size) { KLog::the().printf("what the fuck 4\n\r"); for(;;); }
        uint8_t* testa = new uint8_t[testa_size];
        VFS::the().pread(testa_fd, testa, testa_size, 0, -1);
        Processes::Scheduler::the().CreateProcess(testa, testa_size, "testa", "/");
        PM::PrintMemUsage();
        delete testa;
        VFS::the().close(testa_fd, -1);

        // Try to write to /dev/tty1 manually lol
        int64_t tty1 = VFS::the().open("/", "dev/tty1", -1);
        if(tty1 < 0) { KLog::the().printf("what the fuck 5\n\r"); for(;;); }
        // const char* msg = "Hello World, from VFS!\n";

        for(;;) {
            VFS::the().pwrite(tty1, (void*)buf, strlen(buf), 0, -1);
        }
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

        KLog::the().printf("hello world!\n\r");
        KLog::the().printf("the number 42: %i\n\r", 42);

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
