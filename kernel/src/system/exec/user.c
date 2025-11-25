#include <string.h>
#include <stdint.h>

#include <hardware/memory/paging.h>
#include <hardware/memory/pmm.h>
#include <hardware/memory/gdt.h>

#include <system/exec/user.h>
#include <system/exec/elf64/ehdr64.h>
#include <system/exec/elf64/phdr64.h>
#include <system/exec/elf_enums.h>

extern uint16_t gdt_user_data_selector;
extern uint16_t gdt_user_code_selector;

extern uintptr_t hhdm;

UserImage load_user_elf(uint8_t *data, size_t size)
{
    ELF64_Hdr_t* eh = (ELF64_Hdr_t*)data;
    ELF64_Phdr_t* ph = (ELF64_Phdr_t*)(data + eh->e_phoff);

    //printf("[ELF] Entry point: %#llx\n", eh->e_entry);
    //printf("[ELF] Program headers: %d\n", eh->e_phnum);

    //printf("[ELF] All segments:\n");
    for (int i = 0; i < eh->e_phnum; ++i) {
        //printf("  [%d] type=%d vaddr=%#llx memsz=%#llx filesz=%#llx offset=%#llx\n",
        //    i, ph[i].p_type, ph[i].p_vaddr, ph[i].p_memsz, 
        //    ph[i].p_filesz, ph[i].p_offset);
    }

    uint64_t user_cr3 = create_user_address_space();
    
    // Switch to user PML4 for all mappings
    PageTable *kernel_pml4 = pml4;
    pml4 = (PageTable *)virt_addr(user_cr3);

    for (int i = 0; i < eh->e_phnum; ++i) {
        if (ph[i].p_type != ELF_SEGMENT_TYPE_LOAD) continue;

        uint64_t vaddr   = ph[i].p_vaddr;
        uint64_t memsz   = ph[i].p_memsz;
        uint64_t filesz  = ph[i].p_filesz;
        uint64_t offset  = ph[i].p_offset;

        //printf("[ELF] Segment %d: vaddr=%#llx memsz=%#llx filesz=%#llx\n",
        //       i, vaddr, memsz, filesz);

        uint64_t page_base = vaddr & ~0xFFFULL;
        uint64_t page_off  = vaddr & 0xFFFULL;
        uint64_t total     = page_off + memsz;
        uint64_t pages     = (total + 0xFFF) / 0x1000;

        //printf("[ELF] Mapping %llu pages starting at vaddr=%#llx\n", pages, page_base);

        uintptr_t phys_base = alloc_pages(pages);
        //printf("[ELF] Allocated physical base: %#lx\n", phys_base);

        uint8_t page_flags = PG_PRESENT | PG_USER;

        if (ph[i].p_flags & ELF_SEGMENT_FLAG_WRITABLE) {
            page_flags |= PG_WRITABLE;
        }
        
        // CRITICAL: Don't set NX if executable
        if (!(ph[i].p_flags & ELF_SEGMENT_FLAG_EXECUTABLE)) {
            page_flags |= PG_NX;
        }

        //printf("[ELF] Segment flags: W=%d X=%d NX=%d\n",
        //       !!(ph[i].p_flags & ELF_SEGMENT_FLAG_WRITABLE),
        //       !!(ph[i].p_flags & ELF_SEGMENT_FLAG_EXECUTABLE),
        //       !!(page_flags & PG_NX));

        for (uint64_t p = 0; p < pages; ++p) {
            // Now call mapPage directly, not mapPage_in_pml4
            mapPage((void *)(page_base + p * 0x1000),
                    (void *)(phys_base + p * 0x1000),
                    page_flags | PG_USER);
        }

        uint8_t* kbase = (uint8_t*)(hhdm + phys_base);
        memcpy(kbase + page_off, data + offset, filesz);
        memset(kbase + page_off + filesz, 0, memsz - filesz);
    }
    
    // Flush TLB for all mapped pages
    __asm__ volatile("mov %%cr3, %%rax; mov %%rax, %%cr3" ::: "rax", "memory");

    // Stack setup...
    uint64_t stack_top   = 0x00007FFFFFFF0000ULL;
    uint64_t stack_pages = 4;
    uintptr_t stack_phys = alloc_pages(stack_pages);

    // Stack mapping
    for (uint64_t p = 0; p < stack_pages; ++p) {
        mapPage((void *)(stack_top - (p + 1) * 0x1000),
                (void *)(stack_phys + p * 0x1000),
                PG_PRESENT | PG_WRITABLE | PG_USER | PG_NX);
    }
    
    // Final TLB flush to ensure all mappings are visible
    __asm__ volatile("mov %%cr3, %%rax; mov %%rax, %%cr3" ::: "rax", "memory");

    // Restore kernel PML4 after all user mappings are done
    pml4 = kernel_pml4;

    UserImage img = {
        .cr3_phys       = user_cr3,
        .entry          = eh->e_entry,
        .user_stack_top = stack_top,
    };
    return img;
}

void enter_userspace(const UserImage *img)
{
    uint64_t ss = (uint64_t)gdt_user_data_selector;
    uint64_t cs = (uint64_t)gdt_user_code_selector;

    asm volatile(
        // Allocate space for 5 qwords (RIP, CS, RFLAGS, RSP, SS)
        "sub $40, %%rsp\n\t"

        // iretq pops in this order:
        //  RIP, CS, RFLAGS, RSP, SS
        // so we store them at [RSP + 0..32] in that order

        // RIP
        "mov %[rip], %%rax\n\t"
        "mov %%rax, 0(%%rsp)\n\t"

        // CS
        "mov %[cs], %%rax\n\t"
        "mov %%rax, 8(%%rsp)\n\t"

        // RFLAGS (IF=1)
        "mov $0x202, %%rax\n\t"
        "mov %%rax, 16(%%rsp)\n\t"

        // RSP (user stack top)
        "mov %[rsp_val], %%rax\n\t"
        "mov %%rax, 24(%%rsp)\n\t"

        // SS (user data selector)
        "mov %[ss], %%rax\n\t"
        "mov %%rax, 32(%%rsp)\n\t"

        // Now switch to user CR3
        "mov %[cr3], %%rax\n\t"
        "mov %%rax, %%cr3\n\t"

        // And perform the privilege switch
        "iretq\n\t"
        :
        : [cr3]"r"(img->cr3_phys),
          [rsp_val]"r"(img->user_stack_top),
          [ss]"r"(ss),
          [cs]"r"(cs),
          [rip]"r"(img->entry)
        : "memory", "rax"
    );
}