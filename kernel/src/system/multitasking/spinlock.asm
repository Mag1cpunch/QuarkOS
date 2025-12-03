global spinlock_acquire
global spinlock_release

section .text

; void spinlock_acquire(spinlock_t *lock)
spinlock_acquire:
    mov rax, rdi            ; rdi = pointer to spinlock_t
.acquire_retry:
    lock bts dword [rax], 0
    jc .spin_with_pause
    ret
.spin_with_pause:
    pause
    test dword [rax], 1
    jnz .spin_with_pause
    jmp .acquire_retry

; void spinlock_release(spinlock_t *lock)
spinlock_release:
    mov rax, rdi
    mov dword [rax], 0
    ret