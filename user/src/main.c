static inline long syscall0(long num) {
    long ret;
    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "a"(num)
                     : "rcx", "r11", "memory");
    return ret;
}

static inline long syscall1(long num, long a1) {
    long ret;
    __asm__ volatile("syscall"
                     : "=a"(ret)
                     : "a"(num), "D"(a1)
                     : "rcx", "r11", "memory");
    return ret;
}

static inline long syscall3(long n, long a1, long a2, long a3) {
    long ret;
    asm volatile("syscall" 
                 : "=a"(ret) 
                 : "a"(n), "D"(a1), "S"(a2), "d"(a3)
                 : "rcx", "r11", "memory");
    return ret;
}

void delay(unsigned long count) {
    for (volatile unsigned long i = 0; i < count; i++) {
        __asm__ volatile("nop");
    }
}

void itoa(long n, char *buf) {
    if (n == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    
    int i = 0;
    int is_neg = 0;
    if (n < 0) {
        is_neg = 1;
        n = -n;
    }
    
    while (n > 0) {
        buf[i++] = '0' + (n % 10);
        n /= 10;
    }
    
    if (is_neg) buf[i++] = '-';
    buf[i] = '\0';
    
    for (int j = 0; j < i/2; j++) {
        char tmp = buf[j];
        buf[j] = buf[i-1-j];
        buf[i-1-j] = tmp;
    }
}

int strlen(const char *s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

void _start(void) {
    long tid = syscall1(88, 0);
    
    const char *msg1 = "[ TID 1 ] Hello from process 1!\n";
    const char *msg2 = "[ TID 2 ] Hello from process 2!\n";
    const char *msg3 = "[ TID 3 ] Hello from process 3!\n";
    
    const char *msg = msg1;
    int len = 31;
    
    if (tid == 2) {
        msg = msg2;
    } else if (tid == 3) {
        msg = msg3;
    }
    
    for (int i = 0; i < 5; i++) {
        syscall3(1, 1, (long)msg, len);
        delay(5000000);
    }
    
    syscall1(60, 0);
    
    while(1) __asm__ volatile("pause");
}