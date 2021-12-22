#ifndef VFS_H
#define VFS_H
#include <stddef.h>
#include <kernel-drivers/Block.h>
#include <errno.h>

namespace Kernel {

class VFSDriver;

class VFS {
public:
    


    static VFS& the() {
        static VFS instance;
        return instance;
    }

    struct fs_node {
        char name[128];
        uint32_t mask;
        uint32_t uid;
        uint32_t gid;
        uint32_t flags;
        uint32_t inode;
        uint32_t length;
        uint32_t flags;
        #define FS_NODE_FILE 0x1
        #define FS_NODE_DIR 0x2
        #define FS_NODE_CHARDEVICE 0x3
        #define FS_NODE_BLOCKDEVICE 0x4
        #define FS_NODE_PIPE 0x5
        #define FS_NODE_SYMLINK 0x6
        #define FS_NODE_MOUNT 0x80
        VFSDriver* driver;
    };

    // POSIX dirent
    struct dirent {
        char name[128];
        uint32_t inode;
    };
private:

};


class VFSDriver {
public:
    virtual int read(VFS::fs_node* node, void* buf, size_t size, size_t offset) { return -ENOSYS; }
    virtual int write(VFS::fs_node* node, void* buf, size_t size, size_t offset)  { return -ENOSYS; }
    virtual int open(VFS::fs_node* node, bool read, bool write) { return -ENOSYS; }
    virtual int close(VFS::fs_node* node)  { return -ENOSYS; }
    // virtual int 
private:
};


}

#endif