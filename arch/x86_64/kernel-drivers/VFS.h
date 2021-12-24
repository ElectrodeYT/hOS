#ifndef VFS_H
#define VFS_H

// The VFS is inspired by James Molloy's VFS

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
        uint32_t mask; // Permissions mask
        uint32_t uid; // User ID
        uint32_t gid; // Group ID
        uint32_t inode; // File Inode
        uint32_t length; // File length
        uint32_t flags; // FS Flags
        #define FS_NODE_FILE 0x1
        #define FS_NODE_DIR 0x2
        #define FS_NODE_CHARDEVICE 0x3
        #define FS_NODE_BLOCKDEVICE 0x4
        #define FS_NODE_PIPE 0x5
        #define FS_NODE_SYMLINK 0x6
        #define FS_NODE_MOUNT 0x80
        VFSDriver* driver; // Driver instance for this node
        size_t open_count; // Amount of times this file has been opened
    };

    // POSIX dirent
    struct dirent {
        char name[128];
        uint32_t inode;
    };
private:
    // Root file node.
    // Is checked by the end of the kernelmain() function to see if its NULL.
    fs_node* root_node = NULL;
};


class VFSDriver {
public:
    virtual int read(VFS::fs_node* node, void* buf, size_t size, size_t offset) { return -ENOSYS; (void)node; (void)buf; (void)size; (void)offset; }
    virtual int write(VFS::fs_node* node, void* buf, size_t size, size_t offset)  { return -ENOSYS; (void)node; (void)buf; (void)size; (void)offset; }
    virtual int open(VFS::fs_node* node, bool read, bool write) { return -ENOSYS; (void)node; (void)read; (void)write; }
    virtual int close(VFS::fs_node* node)  { return -ENOSYS; (void)read; (void)write; }
    virtual VFS::dirent* readdir(VFS::fs_node* node, size_t num) { return NULL; (void)node; }
    virtual VFS::fs_node* finddir(VFS::fs_node* node, const char* name) { return NULL; (void)name; }
private:
};


}

#endif