#ifndef USER_H
#define USER_H

#include <stdint.h>
#include <stddef.h>

#define USER_CODE_BASE  0x0000000040000000ULL
#define USER_STACK_TOP  0x000000004000F000ULL
#define USER_STACK_PAGES 4

typedef struct {
    uint64_t cr3_phys;
    uint64_t entry;
    uint64_t user_stack_top;
} UserImage;

UserImage load_user_elf(uint8_t* data, size_t size);
void enter_userspace(UserImage const *img);

#endif // USER_H