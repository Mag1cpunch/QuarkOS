#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include <kernel.h>

#include <system/exec/elf_loader.h>

static bool validateELF64(ELF64_Hdr_t* ehdr) {
    if (ehdr->e_ident[0] != EI_MAG0 || ehdr->e_ident[1] != EI_MAG1 ||
        ehdr->e_ident[2] != EI_MAG2 || ehdr->e_ident[3] != EI_MAG3) {
        panic("Invalid ELF magic");
        return false;
    }

    if (ehdr->e_type != 2) {
        panic("32 bit ELF files are not supported");
        return false;
    }

    if (ehdr->e_machine != ELF_MACHINE_X86_64) {
        panic("Unsupported machine type in ELF file");
        return false;
    }

    if (ehdr->e_version != 1) {
        panic("Unsupported ELF version");
        return false;
    }

    if (ehdr->e_ident[EI_OSABI] != ELF_ABI_LINUX) {
        panic("Unsupported ELF OS/ABI");
        return false;
    }

    if (ehdr->e_type != ELF_TYPE_EXEC && ehdr->e_type != ELF_TYPE_DYN) {
        panic("Unsupported ELF file type");
        return false;
    }

    if (ehdr->e_phoff == 0 || ehdr->e_phnum == 0) {
        panic("ELF file has no program headers");
        return false;
    }

    if (ehdr->e_shoff == 0 || ehdr->e_shnum == 0) {
        panic("ELF file has no section headers");
        return false;
    }

    return true;
}

ELF_File_t* loadRawELF(void* elfData) {
    ELF64_Hdr_t* ehdr = (ELF64_Hdr_t*)elfData;
    validateELF64(ehdr);

    ELF_File_t* elfFile = (ELF_File_t*)malloc(sizeof(ELF_File_t));
    elfFile->data = elfData;
    elfFile->size = 0;
    elfFile->ehdr = ehdr;
    elfFile->phdrs = (ELF64_Phdr_t*)((uint8_t*)elfData + ehdr->e_phoff);
    elfFile->shdrs = (ELF64_Shdr_t*)((uint8_t*)elfData + ehdr->e_shoff);
    elfFile->shstrtab = (const char*)((uint8_t*)elfData +
                                     elfFile->shdrs[ehdr->e_shstrndx].sh_offset);

    for (uint16_t i = 0; i < ehdr->e_phnum; i++) {
        ELF64_Phdr_t* phdr = &elfFile->phdrs[i];
        if (phdr->p_type == ELF_SEGMENT_TYPE_LOAD) {
            void* segmentMemory = malloc(phdr->p_memsz);
            if (segmentMemory == NULL) {
                panic("Failed to allocate memory for ELF segment");
                return NULL;
            }

            memcpy(segmentMemory,
                   (uint8_t*)elfData + phdr->p_offset,
                   phdr->p_filesz);

            if (phdr->p_memsz > phdr->p_filesz) {
                memset((uint8_t*)segmentMemory + phdr->p_filesz,
                       0,
                       phdr->p_memsz - phdr->p_filesz);
            }

            phdr->p_vaddr = (uint64_t)segmentMemory;
        }
        else if (phdr->p_type == ELF_SEGMENT_TYPE_INTERP) {
            panic("ELF interpreting not supported");
        }
        else {
            panic("Unsupported ELF segment type");
        }
    }

    return elfFile;
}

void loadELF(ELF_File_t* elfFile) {
    loadRawELF(elfFile->data);
}

void unloadELF(ELF_File_t* elfFile) {
    for (uint16_t i = 0; i < elfFile->ehdr->e_phnum; i++) {
        ELF64_Phdr_t* phdr = &elfFile->phdrs[i];
        if (phdr->p_type == ELF_SEGMENT_TYPE_LOAD) {
            free((void*)phdr->p_vaddr);
        }
    }

    free(elfFile);
}