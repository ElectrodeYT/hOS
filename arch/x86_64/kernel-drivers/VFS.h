#ifndef VFS_H
#define VFS_H

// The VFS is inspired by James Molloy's VFS
#include <stddef.h>
#include <CPP/vector.h>
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
    // POSIX dirent
    struct dirent {
        char name[128];
        uint32_t inode;
    };
    struct fs_node {
        char* name;
        uint32_t mask; // Permissions mask
        uint32_t uid; // User ID
        uint32_t gid; // Group ID
        uint64_t inode; // File Inode
        uint64_t length; // File length
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

        // TODO: make these inline
        // Thanks C++ compiler
        int read(void* buf, size_t size, size_t offset);
        int write(void* buf, size_t size, size_t offset);
        int open(bool _read, bool _write);
        int close();
        VFS::dirent* readdir(size_t num);
        VFS::fs_node* finddir(const char* name);
    };

    bool attemptMountRoot(VFSDriver* driver);
    fs_node* getNodeFromPath(const char* str);
    fs_node* getRootNode() { return root_node; }
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

    virtual VFS::fs_node* mount() { return NULL; };

    virtual ~VFSDriver() = default;

    BlockDevice* block;
protected:
    VFS::fs_node* root_node;
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

    VFS::fs_node* mount() override;

private:
    void dumpBlockStates();
    void dumpMainDirectory();

    inline uint64_t blockOffset(uint64_t block) { return block * block_size; }

    struct echfs_dir_entry {
        uint64_t dir_id;
        uint8_t type;
        char name[201];
        uint64_t a_time;
        uint64_t m_time;
        uint16_t permissions;
        uint16_t owner;
        uint16_t group;
        uint64_t c_time;
        uint64_t starting_block;
        uint64_t file_size; 
    } __attribute__((packed));

    struct echfs_file {
        VFS::fs_node* node;
        uint64_t dir_entry;
        Vector<uint64_t> known_blocks;
        uint64_t dir_id; // If this is a directory, this is the id of that dir
        bool opened;
    };

    struct echfs_file_ll {
        echfs_file* file;
        echfs_file_ll* next;
    };

    inline uint64_t main_dir_entry_len() {
        return (main_directory_block_size * block_size) / sizeof(echfs_dir_entry);
    }

    uint64_t block_count = 0;
    uint64_t block_size;
    uint64_t allocation_table_block_size;
    uint64_t main_directory_block_size;
    uint64_t* allocation_table;
    echfs_dir_entry* main_directory_table;

    bool is_allocation_table_on_heap;
    bool is_main_directory_table_on_heap;

    echfs_file_ll* cached_file_entries = NULL;
};

}

#endif