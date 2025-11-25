static inline long syscall1(long n, long a1) {
    long ret;
    __asm__ volatile("syscall" : "=a"(ret) : "a"(n), "D"(a1) : "rcx", "r11", "memory");
    return ret;
}

static inline long syscall3(long n, long a1, long a2, long a3) {
    long ret;
    __asm__ volatile("syscall" : "=a"(ret) : "a"(n), "D"(a1), "S"(a2), "d"(a3) : "rcx", "r11", "memory");
    return ret;
}

static int strlen(const char* str) {
    int len = 0;
    while (str[len]) len++;
    return len;
}

static void print(const char* str) {
    syscall3(1, 1, (long)str, strlen(str));
}

void _start(void) {
    for (int i = 0; i < 5; i++) {
        print("Hello from module!\n");
    }
    
    syscall1(60, 0);
    
    while(1) __asm__ volatile("pause");
}