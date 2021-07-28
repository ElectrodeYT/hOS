#ifndef VFS_H
#define VFS_H

#include <mem.h>
#include <panic.h>

// Virtual file system server
// This handles files.

namespace Kernel {

namespace Services {

class VFS {
public:

    static void entryPoint() {
        static VFS instance;
        instance.mainLoop();
        Debug::Panic("Critical service (VFS) loop exited");
    }

    void mainLoop();

private:
    VFS() = default;
};

}

}

#endif