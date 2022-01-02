#ifndef CHARDEVICES_H
#define CHARDEVICES_H

#include <kernel-drivers/VFS.h>
#include <CPP/vector.h>
#include <errno.h>

namespace Kernel {

// A character device is a device that communicates by sending and receiving characters.
// They are not addressable by offsets. They do not have lengths.
struct CharDevice {
    int read(char* buf, size_t len);
    int write(const char* buf, size_t len);

    virtual int read_drv(char* buf, size_t len) { return -ENOSYS; (void)buf; (void)len; }
    virtual int write_drv(const char* buf, size_t len) { return -ENOSYS; (void)buf; (void)len; }
    virtual int ioctl(uint64_t command, void* arg) { return -ENOSYS; (void)command; (void)arg; }
    int char_device_id;

    virtual ~CharDevice() = default;

    bool line_buffered = false;
    
};

class CharDeviceManager {
public:
    int RegisterCharDevice(CharDevice* device);
    void DestroyCharDevice(int id);
    CharDevice* get(int id);

    void KLog_puts(const char* s);

    CharDeviceManager();

    static CharDeviceManager& the() {
        static CharDeviceManager instance;
        return instance;
    }
private:
    Vector<CharDevice*> devices;

    CharDevice* klog_device = NULL;

    // TODO: better id allocation
    int next_id = 0;
};

}

#endif