#ifndef ELF_LOADER_H
#define ELF_LOADER_H

#include <stdint.h>
#include <stddef.h>

#include <system/exec/elf_file.h>
#include <system/exec/elf_enums.h>
#include <system/exec/elf64/ehdr64.h>
#include <system/exec/elf64/phdr64.h>
#include <system/exec/elf64/shdr64.h>

#define EI_OSABI 7

ELF_File_t* loadRawELF(void* elfData);
void loadELF(ELF_File_t* elfFile);
void unloadELF(ELF_File_t* elfFile);
void* getELFEntryPoint(ELF_File_t* elfFile);


#endif // ELF_LOADER_H