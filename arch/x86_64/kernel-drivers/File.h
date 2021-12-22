#ifndef FILE_H
#define FILE_H
#include <stdint.h>

namespace Kernel {

class File {
public:
    virtual int read(void* buf, uint64_t offset) { return -1; }
    virtual int write(void* buf, uint64_t size, uint64_t offset) { return -1; }
private:

}

}

#endif