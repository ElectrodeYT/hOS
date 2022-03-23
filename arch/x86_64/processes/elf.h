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

    // Performs ELF Relocations.
    void relocate(void* load_base);

    enum ObjectFileType {
        ET_NONE = 0,
        ET_REL,
        ET_EXEC,
        ET_DYN,
        ET_CORE
    };

    enum ProgramHeaderType {
        PT_NULL = 0,
        PT_LOAD,
        PT_DYNAMIC,
        PT_INTERP,
        PT_NOTE,
        PT_SHLIB,
        PT_PHDR
    };

    enum SectionHeaderType {
        SHT_NULL = 0,
        SHT_PROGBITS,
        SHT_SYMTAB,
        SHT_STRTAB,
        SHT_RELA,
        SHT_HASH,
        SHT_DYNAMIC,
        SHT_NOTE,
        SHT_NOBITS,
        SHT_REL,
        SHT_SHLIB,
        SHT_DYNSYM
    };

    enum ElfAuxVector {
        AT_NULL = 0,
        AT_PHDR = 3,
        AT_PHENT = 4,
        AT_PHNUM = 5,
        AT_ENTRY = 9,
        AT_EXECPATH = 15,
        AT_RANDOM = 25,
        AT_EXECFN = 31
    };

    enum ElfRelocationTypes {
        R_AMD64_NONE = 0,
        R_AMD64_64,
        R_AMD64_PC32,
        R_AMD64_GOT32,
        R_AMD64_PLT32,
        R_AMD64_COPY,
        R_AMD64_GLOB_DAT,
        R_AMD64_JUMP_SLOT,
        R_AMD64_RELATIVE,
        R_AMD64_GOTPCREL,
        R_AMD64_32,
        R_AMD64_32S,
        R_AMD64_16,
        R_AMD64_PC16,
        R_AMD64_8,
        R_AMD64_PC8,
        R_AMD64_PC64 = 24,
        R_AMD64_GOTOFF64,
        R_AMD64_GOTPC32,
        R_AMD64_SIZE32 = 32,
        R_AMD64_SIZE64
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

    uint64_t file_base = 0;

    bool isDynamic = false;
    char* interpreterPath = NULL;

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
            memcopy((void*)((uint64_t)data + file_begin), dst, size);
        }

    private:
        void* data;
    };
    
    struct SymbolTable {
        uint32_t st_name;
        uint8_t st_info;
        uint8_t st_other;
        uint16_t st_shndx;
        uint64_t st_value;
        uint64_t st_size;
    } __attribute__((packed));

    // These defines are straight from the ELF64 spec
    #define ELF64_R_SYM(i)((i) >> 32)
    #define ELF64_R_TYPE(i)((i) & 0xffffffffL)
    #define ELF64_R_INFO(s, t)(((s) << 32) + ((t) & 0xffffffffL))
    #define SHN_UNDEF (0x00) // Undefined/Not present
    
    struct Rel {
        uint64_t r_offset;
        uint64_t r_info;
    } __attribute__((packed));

    struct Rela {
        uint64_t r_offset;
        uint64_t r_info;
        uint64_t r_addend;
    } __attribute__((packed));
  
    struct SectionHeader {
        uint64_t file_begin;
        uint64_t size;
        uint64_t type;
        uint32_t link;
        uint32_t info;
        uint64_t ent_size;

        SymbolTable* toSymbols(void* data) { return (SymbolTable*)((uint8_t*)(data) + file_begin); }
        Rel* toRel(void* data) { return (Rel*)((uint8_t*)(data) + file_begin); }
        Rela* toRela(void* data) { return (Rela*)((uint8_t*)(data) + file_begin); }
        
    };


    Vector<Section*> sections;
    Vector<SectionHeader*> section_headers;

private:
    
    // TODO: handle, well, everything correctly
    // (tl;dr: dont just treat everything as a virtual address)
    void relocateSingleTable(Rela* rela, SectionHeader* relocation, void* load_base);
    void relocateSingleTable(Rel* rel, SectionHeader* relocation, void* load_base);
    
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

    
    uint64_t read64(uint64_t offset) { return *((uint64_t*)((uint64_t)data + offset)); }
    int64_t read64s(uint64_t offset) { return *((int64_t*)((uint64_t)data + offset)); }
    uint32_t read32(uint64_t offset) { return *((uint32_t*)((uint64_t)data + offset)); }
    int32_t read32s(uint64_t offset) { return *((int32_t*)((uint64_t)data + offset)); }
    uint32_t read16(uint64_t offset) { return *((uint16_t*)((uint64_t)data + offset)); }
    uint32_t read8(uint64_t offset) { return *((uint8_t*)((uint64_t)data + offset)); }
    



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