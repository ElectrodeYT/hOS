#ifndef ELF_H
#define ELF_H

#include <CPP/vector.h>

// Elf64_Addr -> uint64_t
// Elf64_Off -> uint64_t as offset
// Elf64_Half -> uint16_t
// Elf64_Word -> uint32_t
// Elf64_Sword -> int32_t
// Elf64_Xword -> uint64_t
// Elf64_Sxword -> int64_t
// unsigned char -> uint8_t

class ELF {
public:
    ELF(void* d, size_t s) : data(d), file_size(s) { }
    ~ELF() {
        for(size_t i = 0; i < sections.size(); i++) {
            delete sections.at(i);
        }
    }

    // Reads the header.
    // Returns true if valid and the read succeded, false if it failed.
    bool readHeader();

    enum ObjectFileType {
        ET_NONE = 0,
        ET_REL,
        ET_EXEC,
        ET_DYN,
        ET_CORE
    };

    ObjectFileType file_object_type;
    uint16_t file_machine_type;
    uint32_t file_object_version;
    uint64_t file_entry;
    uint64_t file_program_header_offset;
    uint64_t file_section_header_offset;
    uint32_t file_processor_flags;
    uint16_t file_header_size;
    uint16_t file_program_header_entry_size;
    uint16_t file_program_header_count;
    uint16_t file_section_header_entry_size;
    uint16_t file_section_header_count;
    uint16_t file_section_name_string_table_index;

    struct Section {
        uint64_t file_begin;
        uint64_t size;

        uint64_t vaddr;
        uint64_t segment_size;

        bool read;
        bool write;
        bool execute;

        bool loadable;

        Section(void* d) : data(d) { }

        void copy_out(void* dst) {
            memcopy((data + file_begin), dst, size);
        }

    private:
        void* data;
    };
    
    Vector<Section*> sections;

private:
    
    
    
    // Elf file endianness
    // true is big endian, false is little endian
    bool endianness;

    // Elf file version
    uint8_t file_version;

    // Elf OS ABI identification
    uint8_t file_abi;

    // Elf ABI Version
    uint8_t file_abi_version;
    
    // Padding byte start
    uint8_t file_padding_byte_start;

    
    uint64_t read64(uint64_t offset) { return *((uint64_t*)(data + offset)); }
    int64_t read64s(uint64_t offset) { return *((int64_t*)(data + offset)); }
    uint32_t read32(uint64_t offset) { return *((uint32_t*)(data + offset)); }
    int32_t read32s(uint64_t offset) { return *((int32_t*)(data + offset)); }
    uint32_t read16(uint64_t offset) { return *((uint16_t*)(data + offset)); }
    uint32_t read8(uint64_t offset) { return *((uint8_t*)(data + offset)); }
    



    void* data;
    size_t file_size;

    char* getABI() {
        static char sysv[] = "SysV";
        static char hpux[] = "HP-UX";
        static char standalone[] = "Standalone";
        static char invalid[] = "Invalid";
        switch(file_abi) {
            case 0: return sysv;
            case 1: return hpux;
            case 255: return standalone;
            default: return invalid;
        }
    }

};

#endif