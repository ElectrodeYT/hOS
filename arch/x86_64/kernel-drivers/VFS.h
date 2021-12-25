#ifndef VFS_H
#define VFS_H

// The VFS is inspired by James Molloy's VFS

#include <stddef.h>
#include <kernel-drivers/BlockDevices.h>
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
    virtual int open(VFS::fs_node* node, bool _read, bool _write) { return -ENOSYS; (void)node; (void)_read; (void)_write; }
    virtual int close(VFS::fs_node* node)  { return -ENOSYS; (void)node; }
    virtual VFS::dirent* readdir(VFS::fs_node* node, size_t num) { return NULL; (void)node; (void)num; }
    virtual VFS::fs_node* finddir(VFS::fs_node* node, const char* name) { return NULL; (void)node; (void)name; }

    virtual bool mount();

    BlockDevice* block;
protected:
    bool mounted = false;
private:
};


class EchFSDriver : public VFSDriver {
    int read(VFS::fs_node* node, void* buf, size_t size, size_t offset) override;
    int write(VFS::fs_node* node, void* buf, size_t size, size_t offset) override;
    int open(VFS::fs_node* node, bool read, bool write) override;
    int close(VFS::fs_node* node) override;
    VFS::dirent* readdir(VFS::fs_node* node, size_t num) override;
    VFS::fs_node* finddir(VFS::fs_node* node, const char* name) override;

    bool mount() override;

private:
    inline uint64_t blockOffset(uint64_t block) { return block * block_size; }

    uint64_t block_count = 0;
    uint64_t block_size;
    uint64_t allocation_table_block_size;
    uint64_t* allocation_table;

    bool is_allocation_table_on_heap;
};

}

#endif