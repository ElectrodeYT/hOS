#include <processes/vfs.h>
#include <debug/serial.h>

namespace Kernel {

namespace Services {

void VFS::mainLoop() {
    Debug::SerialPrintf("VFS: main loop begun\r\n");
    for(;;);
}

}

}
