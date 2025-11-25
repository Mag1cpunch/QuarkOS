global exit_to_thread

extern gdt_user_data_selector
extern gdt_user_code_selector
extern gdt_kernel_code_selector
extern gdt_kernel_data_selector

section .text

; ---------------------------------------------------------
; void exit_to_thread(Thread *thread) __attribute__((noreturn))
; rdi = Thread*
; ---------------------------------------------------------
; Never returns to the exiting thread
; Jumps directly to the next thread without saving old context
exit_to_thread:
    ; Check if thread has entered userspace by checking if user_rip is set
    ; user_rip at Thread + 16 + 80 = 96
    mov     rax, [rdi + 16 + 80]    ; context.user_rip
    test    rax, rax
    jz      .kernel_mode            ; If user_rip is 0, thread is in kernel mode
    
    ; Context starts at Thread+16, so offsets are: Thread + 16 + context_offset
    mov     rbx, [rdi + 16 + 88]    ; context.user_rsp (user_rsp at 88 in context)
    mov     rcx, [rdi + 16 + 72]    ; context.rflags (rflags at 72 in context)
    mov     r8,  [rdi + 16 + 64]    ; context.cr3 (cr3 at 64 in context)
    
    test    r8, r8
    jz      .no_cr3
    mov     cr3, r8
.no_cr3:
    
    movzx   rdx, word [rel gdt_user_data_selector]
    movzx   rsi, word [rel gdt_user_code_selector]
    
    ; Build IRETQ frame (rax already contains user_rip)
    push    rdx                     ; SS
    push    rbx                     ; RSP
    push    rcx                     ; RFLAGS
    push    rsi                     ; CS
    push    rax                     ; RIP
    
    ; Clear registers
    xor     rax, rax
    xor     rbx, rbx
    xor     rcx, rcx
    xor     rdx, rdx
    xor     rsi, rsi
    xor     rdi, rdi
    xor     rbp, rbp
    xor     r8, r8
    xor     r9, r9
    xor     r10, r10
    xor     r11, r11
    xor     r12, r12
    xor     r13, r13
    xor     r14, r14
    xor     r15, r15
    
    iretq

.kernel_mode:
    mov     rsp, [rdi + 16 + 8]     ; context.rsp (context at 16, rsp at 8 = 24)
    mov     rax, [rdi + 16 + 0]     ; context.rip (context at 16, rip at 0 = 16)
    mov     r8,  [rdi + 16 + 64]    ; context.cr3
    
    test    r8, r8
    jz      .no_cr3_kernel
    mov     cr3, r8
.no_cr3_kernel:
    sti
    jmp     rax ; call trampoline
