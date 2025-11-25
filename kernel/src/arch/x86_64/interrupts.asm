extern exceptionHandler
extern irqHandler
extern kernel_cr3_phys
extern syscall_handler

extern tss

global syscall_entry_fast
global syscall_entry

extern gdt_user_code_selector
extern gdt_user_data_selector

%macro pushad 0
    push rax      
    push rcx
    push rdx
    push rdi
    push rsi
    push r8
    push r9
    push r10
    push r11
%endmacro

%macro popad 0
    pop r11
    pop r10
    pop r9
    pop r8
    pop rsi
    pop rdi
    pop rdx      
    pop rcx
    pop rax
%endmacro

section .bss
    align 16
    temp_user_rsp: resq 1
    temp_rax:      resq 1
section .text

syscall_entry_fast:
    ; On entry: RCX=RIP, R11=RFLAGS, RSP=UserStack, RAX=SyscallNum
    
    mov [rel temp_user_rsp], rsp
    mov [rel temp_rax], rax ; Save original RAX (syscall number)

    mov rsp, [rel tss + 4] ; load kernel stack from TSS
    
    xor rax, rax
    mov ax, [rel gdt_user_data_selector] ; Load User Data Selector
    push rax 
    
    push qword [rel temp_user_rsp] ; Push User RSP
    
    push r11 ; Push RFLAGS (from R11)    
    
    xor rax, rax
    mov ax, [rel gdt_user_code_selector] ; Load User Code Selector
    push rax       
    
    push rcx ; Push User RIP

    push qword 0          ; Err code
    push qword 0x80       ; Int no

    push qword [rel temp_rax] 
    
    push rcx
    push rdx
    push rdi
    push rsi
    push r8
    push r9
    push r10
    push r11
    push qword 0          ; cr3 placeholder

    mov rax, cr3
    mov [rsp], rax        ; Save old CR3
    mov rax, [rel kernel_cr3_phys]
    mov cr3, rax

    mov rdi, rsp
    cld
    call syscall_handler

    mov rax, [rsp]
    mov cr3, rax

    add rsp, 8            ; Skip CR3
    pop r11
    pop r10
    pop r9
    pop r8
    pop rsi
    pop rdi
    pop rdx
    pop rcx
    pop rax
    
    add rsp, 16           ; Skip IntNo, ErrCode

    ; The stack now contains exactly [RIP, CS, RFLAGS, RSP, SS]
    iretq

syscall_entry:
    push 0x80
    push 0
    pushad
    mov rdi, cr3
    push rdi
    mov rdi, [rel kernel_cr3_phys]
    mov cr3, rdi
    cld
    lea rdi, [rsp]
    call syscall_handler
    pop rdi
    mov cr3, rdi
    popad
    add rsp, 16
    iretq

isr_common_stub:
    pushad
    mov rdi, cr3
    push rdi
    mov rdi, [rel kernel_cr3_phys]
    mov cr3, rdi
    cld 
    lea rdi, [rsp]
    call exceptionHandler
    pop rdi
    mov cr3, rdi
    popad
    add rsp, 0x10 
    iretq

%macro isr_err_stub 1
isr_stub_%+%1:
    push %1
    jmp isr_common_stub
%endmacro

%macro isr_no_err_stub 1
isr_stub_%+%1:
    push 0
    push %1
    jmp isr_common_stub
%endmacro

isr_no_err_stub 0
isr_no_err_stub 1
isr_no_err_stub 2
isr_no_err_stub 3
isr_no_err_stub 4
isr_no_err_stub 5
isr_no_err_stub 6
isr_no_err_stub 7
isr_err_stub    8
isr_no_err_stub 9
isr_err_stub    10
isr_err_stub    11
isr_err_stub    12
isr_err_stub    13
isr_err_stub    14
isr_no_err_stub 15
isr_no_err_stub 16
isr_err_stub    17
isr_no_err_stub 18
isr_no_err_stub 19
isr_no_err_stub 20
isr_no_err_stub 21
isr_no_err_stub 22
isr_no_err_stub 23
isr_no_err_stub 24
isr_no_err_stub 25
isr_no_err_stub 26
isr_no_err_stub 27
isr_no_err_stub 28
isr_no_err_stub 29
isr_err_stub    30
isr_no_err_stub 31

irq_common_stub:
    pushad
    mov rdi, cr3
    push rdi
    mov rdi, [rel kernel_cr3_phys]
    mov cr3, rdi
    cld
    lea rdi, [rsp]
    call irqHandler
    pop rdi
    mov cr3, rdi
    popad
    add rsp, 0x10
    iretq

%assign i 32
%rep 	223
irq_stub_%+i:
    cli
    push 0
    push i
    jmp irq_common_stub
    %assign i i+1
%endrep

global isr_stub_table
isr_stub_table:
%assign i 0 
%rep    32 
    DQ isr_stub_%+i 
    %assign i i+1 
%endrep

global irq_stub_table
irq_stub_table:
%assign i 32
%rep 	223
    DQ irq_stub_%+i
    %assign i i+1
%endrep