global acquireLock
global releaseLock

section .bss
    align 4
    spinlock_var resd 1

section .text

acquireLock:
    lock bts dword [rel spinlock_var], 0
    jc .spin_with_pause
    ret

.spin_with_pause:
    pause
    test dword [rel spinlock_var], 1
    jnz .spin_with_pause
    jmp acquireLock

releaseLock:
    mov dword [rel spinlock_var], 0
    ret