#ifndef ELF_FILE_H
#define ELF_FILE_H

#include <stddef.h>
#include <stdint.h>

#include <system/exec/elf_enums.h>
#include <system/exec/elf64/ehdr64.h>
#include <system/exec/elf64/phdr64.h>
#include <system/exec/elf64/shdr64.h>

typedef struct {
    void           *data;     /* pointer to raw ELF image */
    size_t          size;     /* size of the image (optional) */
    ELF64_Hdr_t    *ehdr;     /* pointer to the ELF header (within data) */
    ELF64_Phdr_t   *phdrs;    /* pointer to program headers */
    ELF64_Shdr_t   *shdrs;    /* pointer to section headers */
    const char     *shstrtab; /* section header string table */
} ELF_File_t;

#endif /* ELF_FILE_H */