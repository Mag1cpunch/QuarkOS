global context_switch
global enter_userspace_from_task

extern tss
extern gdt_user_data_selector
extern gdt_user_code_selector

section .text

; ---------------------------------------------------------
; void context_switch(TaskContext *old, TaskContext *new)
; rdi = old, rsi = new
; ---------------------------------------------------------
context_switch:
    ; save current context into *old

    lea     rax, [rel .return_here]
    mov     [rdi + 0], rax          ; old->rip

    mov     [rdi + 8],  rsp         ; old->rsp
    mov     [rdi + 16], rbp         ; old->rbp
    mov     [rdi + 24], rbx         ; old->rbx
    mov     [rdi + 32], r12         ; old->r12
    mov     [rdi + 40], r13         ; old->r13
    mov     [rdi + 48], r14         ; old->r14
    mov     [rdi + 56], r15         ; old->r15

    pushfq
    pop     rax
    mov     [rdi + 72], rax         ; old->rflags
    
    ; Save additional registers for fork support
    ; (rcx, rdx, r8-r11 normally caller-saved, but preserve for fork)
    mov     [rdi + 104], rcx        ; old->rcx
    mov     [rdi + 112], rdx        ; old->rdx
    mov     [rdi + 136], r8         ; old->r8
    mov     [rdi + 144], r9         ; old->r9
    mov     [rdi + 152], r10        ; old->r10
    mov     [rdi + 160], r11        ; old->r11

    ; -------- load new context --------
    mov     rax, [rsi + 64]         ; new->cr3
    mov     cr3, rax

    mov     rsp, [rsi + 8]          ; new->rsp
    mov     rbp, [rsi + 16]
    mov     rbx, [rsi + 24]
    mov     r12, [rsi + 32]
    mov     r13, [rsi + 40]
    mov     r14, [rsi + 48]
    mov     r15, [rsi + 56]
    
    ; Load additional registers
    mov     rcx, [rsi + 104]        ; new->rcx
    mov     rdx, [rsi + 112]        ; new->rdx
    mov     r8,  [rsi + 136]        ; new->r8
    mov     r9,  [rsi + 144]        ; new->r9
    mov     r10, [rsi + 152]        ; new->r10
    mov     r11, [rsi + 160]        ; new->r11

    mov     rax, [rsi + 72]         ; new->rflags
    push    rax
    popfq

    mov     rax, [rsi + 0]          ; new->rip
    jmp     rax                     ; never returns here until switched back

.return_here:
    ret



; ---------------------------------------------------------
; void enter_userspace_from_task(struct Thread *thread)
; rdi = Thread*
; ---------------------------------------------------------
; Thread structure offsets:
;   tid          = 0
;   state        = 8  
;   context      = 16 (ThreadContext is 168 bytes, ends at 184)
;   remaining    = 184
;   priority     = 192
;   in_userspace = 196
;   kernel_stack = 200 (aligned to 208)
;   kernel_stack_top = 208
;   process      = 216
;   next         = 224
enter_userspace_from_task:
    ; Thread.kernel_stack_top at offset 208
    mov     rax, [rdi + 208]
    mov     [rel tss + 4], rax      ; tss.rsp0 is at offset 4
    
    ; CR3 already loaded by trampoline, don't load again
    
    ; load user_rip, user_rsp, rflags from context
    ; context.user_rip is at Thread + 16 + 80 = 96
    ; context.user_rsp is at Thread + 16 + 88 = 104  
    ; context.rflags is at Thread + 16 + 72 = 88
    mov     rax, [rdi + 96]         ; user_rip
    mov     rbx, [rdi + 104]        ; user_rsp
    mov     rcx, [rdi + 88]         ; rflags

    movzx   rdx, word [rel gdt_user_data_selector] ; user SS (data)
    movzx   rsi, word [rel gdt_user_code_selector] ; user CS (code)

    ; build iretq frame on current kernel stack
    push    rdx                     ; SS
    push    rbx                     ; RSP
    push    rcx                     ; RFLAGS
    push    rsi                     ; CS
    push    rax                     ; RIP

    ; clear GP regs
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