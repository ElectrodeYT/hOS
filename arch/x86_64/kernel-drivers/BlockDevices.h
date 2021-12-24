#ifndef BLOCKDEVICES_H
#define BLOCKDEVICES_H
#include <stddef.h>
#include <stdint.h>
#include <CPP/vector.h>

namespace Kernel {

struct BlockDevice {
    virtual int read(void* buf, size_t len, size_t offset);
    virtual int write(void* buf, size_t len, size_t offset);

    int block_device_id;

    uint64_t len;
};

// TODO: implement a hda1,2,3,4,.... style thing

class BlockManager {
public:
    int RegisterBlockDevice(BlockDevice* device);
    BlockDevice* GetBlockDevice(int block_id);

    static BlockManager& the() {
        static BlockManager instance;
        return instance;
    }

private:
    Vector<BlockDevice*> devices;
    int next_id = 0;
};

}

#endif