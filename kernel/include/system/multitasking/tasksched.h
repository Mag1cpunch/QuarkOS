#ifndef TASKSCHED_H
#define TASKSCHED_H

#include <stdint.h>

#define BASE_TIME_QUANTUM 100

typedef struct {
    uint64_t rip;       //  0
    uint64_t rsp;       //  8
    uint64_t rbp;       // 16
    uint64_t rbx;       // 24
    uint64_t r12;       // 32
    uint64_t r13;       // 40
    uint64_t r14;       // 48
    uint64_t r15;       // 56

    uint64_t cr3;       // 64
    uint64_t rflags;    // 72

    uint64_t user_rip;  // 80
    uint64_t user_rsp;  // 88
    
    uint64_t rax;       // 96
    uint64_t rcx;       // 104
    uint64_t rdx;       // 112
    uint64_t rsi;       // 120
    uint64_t rdi;       // 128
    uint64_t r8;        // 136
    uint64_t r9;        // 144
    uint64_t r10;       // 152
    uint64_t r11;       // 160
} ThreadContext;

typedef enum {
    THREAD_STATE_READY,
    THREAD_STATE_RUNNING,
    THREAD_STATE_BLOCKED,
    THREAD_STATE_DONE
} ThreadState;

typedef enum {
    THREAD_PRIORITY_LOW = 1,
    THREAD_PRIORITY_MEDIUM = 2,
    THREAD_PRIORITY_HIGH = 3,
    THREAD_PRIORITY_REALTIME = 4
} ThreadPriority;

typedef struct Thread {
    uint64_t    tid;           // Thread ID
    ThreadState   state;
    ThreadContext context;
    
    uint64_t    remaining_time;
    int         priority;
    int         in_userspace;
    
    void       *kernel_stack;
    void       *kernel_stack_top;
    
    struct Process *process;   // Parent process
    struct Thread  *next;      // Next thread in system
    struct Thread  *next_in_process; // Next thread in same process
} Thread;

typedef struct Process {
    uint64_t    pid;
    uint64_t    cr3;           // Address space (shared by all threads)
    
    struct Thread *main_thread;
    struct Thread *thread_list;
    int           thread_count;
    
    struct Process *next;
} Process;

extern Thread *current_thread;
extern Thread *thread_list;
extern Process *process_list;

void scheduler_init(void);
Thread *schedule(void);
Process* create_process(void* elf_data);
Thread *create_thread(Process* proc, void* user_stack);
void finalize_thread_list(void);
void terminate_process(Process* proc, int exit_code);
void task_trampoline(void);

#endif