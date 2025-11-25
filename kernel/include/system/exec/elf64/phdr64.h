#ifndef PHDR64_H
#define PHDR64_H

#include <stdint.h>

typedef __attribute__((packed)) struct {
    uint32_t p_type;    /* Type of segment */
    uint32_t p_flags;   /* Segment attributes */
    uint64_t p_offset;  /* Offset in file */
    uint64_t p_vaddr;   /* Virtual address in memory */
    uint64_t p_paddr;   /* Reserved */
    uint64_t p_filesz;  /* Size of segment in file */
    uint64_t p_memsz;   /* Size of segment in memory */
    uint64_t p_align;   /* Alignment of segment */
    uint8_t  reserved[24]; /* Reserved for future use */
} ELF64_Phdr_t;

#endif // PHDR64_H