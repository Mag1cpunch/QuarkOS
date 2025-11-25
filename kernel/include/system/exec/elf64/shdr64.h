#ifndef SHDR64_H
#define SHDR64_H

typedef __attribute__((packed)) struct {
    uint32_t sh_name;       /* Section name (string tbl index) */
    uint32_t sh_type;       /* Section type */
    uint64_t sh_flags;      /* Section flags */
    uint64_t sh_addr;       /* Section virtual addr at execution */
    uint64_t sh_offset;     /* Section file offset */
    uint64_t sh_size;       /* Section size in bytes */
    uint32_t sh_link;       /* Link to another section */
    uint32_t sh_info;       /* Additional section information */
    uint64_t sh_addralign;  /* Section alignment */
    uint64_t sh_entsize;    /* Entry size if section holds table */
    uint8_t  reserved[24];  /* Reserved for future use */
} ELF64_Shdr_t;

#endif // SHDR64_H