#ifndef EHDR64_H
#define EHDR64_H

#include <stdint.h>
#include <stddef.h>

#include <kernel.h>

#include <system/exec/elf64/phdr64.h>
#include <system/exec/elf64/shdr64.h>

#define EI_MAG0 0x7F
#define EI_MAG1 0x45
#define EI_MAG2 0x4C
#define EI_MAG3 0x46

typedef struct {
    unsigned char e_ident[16];
    uint16_t      e_type;
    uint16_t      e_machine;
    uint32_t      e_version;
    uint64_t      e_entry;
    uint64_t      e_phoff;
    uint64_t      e_shoff;
    uint32_t      e_flags;
    uint16_t      e_ehsize;
    uint16_t      e_phentsize;
    uint16_t      e_phnum;
    uint16_t      e_shentsize;
    uint16_t      e_shnum;
    uint16_t      e_shstrndx;
} ELF64_Hdr_t;

static inline ELF64_Phdr_t* getProgramHeader64(ELF64_Hdr_t* ehdr, uint16_t index) {
    if (index >= ehdr->e_phnum) {
        panic("Program header index out of bounds");
        return NULL;
    }
    return (ELF64_Phdr_t*)((uint8_t*)ehdr + ehdr->e_phoff + (index * ehdr->e_phentsize));
}

static inline ELF64_Shdr_t* getSectionHeader64(ELF64_Hdr_t* ehdr, uint16_t index) {
    if (index >= ehdr->e_shnum) {
        panic("Section header index out of bounds");
        return NULL;
    }
    return (ELF64_Shdr_t*)((uint8_t*)ehdr + ehdr->e_shoff + (index * ehdr->e_shentsize));
}

#endif // EHDR64_H