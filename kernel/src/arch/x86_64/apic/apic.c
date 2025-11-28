#include <stdint.h>
#include <stdio.h>

#include <arch/x86_64/cpu.h>
#include <arch/x86_64/apic/apic.h>
#include <arch/x86_64/pic.h>
#include <arch/x86_64/isr.h>
#include <arch/x86_64/cpu.h>

#include <hardware/memory/paging.h>
#include <hardware/memory/heap.h>
#include <hardware/memory/tss.h>

#include <system/multitasking/tasksched.h>

#define IA32_APIC_BASE_MSR 0x1B
#define IA32_APIC_BASE_MSR_BSP 0x100 // Processor is a BSP
#define IA32_APIC_BASE_MSR_ENABLE 0x800

#define APIC_EOI_REGISTER 0xB0
#define X2APIC_EOI_REGISTER 0x80B

#define LAPIC_VIRT  0xFFFFFFFFFEE00000ULL

enum 
{
    CPUID_FEAT_ECX_SSE3         = 1 << 0, 
    CPUID_FEAT_ECX_PCLMUL       = 1 << 1,
    CPUID_FEAT_ECX_DTES64       = 1 << 2,
    CPUID_FEAT_ECX_MONITOR      = 1 << 3,  
    CPUID_FEAT_ECX_DS_CPL       = 1 << 4,  
    CPUID_FEAT_ECX_VMX          = 1 << 5,  
    CPUID_FEAT_ECX_SMX          = 1 << 6,  
    CPUID_FEAT_ECX_EST          = 1 << 7,  
    CPUID_FEAT_ECX_TM2          = 1 << 8,  
    CPUID_FEAT_ECX_SSSE3        = 1 << 9,  
    CPUID_FEAT_ECX_CID          = 1 << 10,
    CPUID_FEAT_ECX_SDBG         = 1 << 11,
    CPUID_FEAT_ECX_FMA          = 1 << 12,
    CPUID_FEAT_ECX_CX16         = 1 << 13, 
    CPUID_FEAT_ECX_XTPR         = 1 << 14, 
    CPUID_FEAT_ECX_PDCM         = 1 << 15, 
    CPUID_FEAT_ECX_PCID         = 1 << 17, 
    CPUID_FEAT_ECX_DCA          = 1 << 18, 
    CPUID_FEAT_ECX_SSE4_1       = 1 << 19, 
    CPUID_FEAT_ECX_SSE4_2       = 1 << 20, 
    CPUID_FEAT_ECX_X2APIC       = 1 << 21, 
    CPUID_FEAT_ECX_MOVBE        = 1 << 22, 
    CPUID_FEAT_ECX_POPCNT       = 1 << 23, 
    CPUID_FEAT_ECX_TSC          = 1 << 24, 
    CPUID_FEAT_ECX_AES          = 1 << 25, 
    CPUID_FEAT_ECX_XSAVE        = 1 << 26, 
    CPUID_FEAT_ECX_OSXSAVE      = 1 << 27, 
    CPUID_FEAT_ECX_AVX          = 1 << 28,
    CPUID_FEAT_ECX_F16C         = 1 << 29,
    CPUID_FEAT_ECX_RDRAND       = 1 << 30,
    CPUID_FEAT_ECX_HYPERVISOR   = 1 << 31,
 
    CPUID_FEAT_EDX_FPU          = 1 << 0,  
    CPUID_FEAT_EDX_VME          = 1 << 1,  
    CPUID_FEAT_EDX_DE           = 1 << 2,  
    CPUID_FEAT_EDX_PSE          = 1 << 3,  
    CPUID_FEAT_EDX_TSC          = 1 << 4,  
    CPUID_FEAT_EDX_MSR          = 1 << 5,  
    CPUID_FEAT_EDX_PAE          = 1 << 6,  
    CPUID_FEAT_EDX_MCE          = 1 << 7,  
    CPUID_FEAT_EDX_CX8          = 1 << 8,  
    CPUID_FEAT_EDX_APIC         = 1 << 9,  
    CPUID_FEAT_EDX_SEP          = 1 << 11, 
    CPUID_FEAT_EDX_MTRR         = 1 << 12, 
    CPUID_FEAT_EDX_PGE          = 1 << 13, 
    CPUID_FEAT_EDX_MCA          = 1 << 14, 
    CPUID_FEAT_EDX_CMOV         = 1 << 15, 
    CPUID_FEAT_EDX_PAT          = 1 << 16, 
    CPUID_FEAT_EDX_PSE36        = 1 << 17, 
    CPUID_FEAT_EDX_PSN          = 1 << 18, 
    CPUID_FEAT_EDX_CLFLUSH      = 1 << 19, 
    CPUID_FEAT_EDX_DS           = 1 << 21, 
    CPUID_FEAT_EDX_ACPI         = 1 << 22, 
    CPUID_FEAT_EDX_MMX          = 1 << 23, 
    CPUID_FEAT_EDX_FXSR         = 1 << 24, 
    CPUID_FEAT_EDX_SSE          = 1 << 25, 
    CPUID_FEAT_EDX_SSE2         = 1 << 26, 
    CPUID_FEAT_EDX_SS           = 1 << 27, 
    CPUID_FEAT_EDX_HTT          = 1 << 28, 
    CPUID_FEAT_EDX_TM           = 1 << 29, 
    CPUID_FEAT_EDX_IA64         = 1 << 30,
    CPUID_FEAT_EDX_PBE          = 1 << 31
};

const uint32_t CPUID_FLAG_MSR = 1 << 5;
volatile uint32_t *lapic;
 
static int cpuHasMSR()
{
    uint32_t eax, edx;
    cpuid(1, &eax, &edx);
    return edx & CPUID_FLAG_MSR;
}
 
void cpuGetMSR(uint32_t msr, uint32_t *lo, uint32_t *hi)
{
    __asm__ volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}
 
void cpuSetMSR(uint32_t msr, uint32_t lo, uint32_t hi)
{
    __asm__ volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}

static int getModel(void)
{
    uint32_t eax = 0, edx = 0;
    cpuid(0, &eax, &edx);
    return eax;
}

static int checkAPIC(void)
{
    uint32_t eax = 1, edx = 0;
    cpuid(1, &eax, &edx);
    return edx & CPUID_FEAT_EDX_APIC;
}

static inline uint32_t lapic_read(uint32_t reg)
{
    return lapic[reg >> 2];
}

static inline void lapic_write(uint32_t reg, uint32_t val)
{
    lapic[reg >> 2] = val;
    (void)lapic[0];
}

static inline void lapic_send_eoi_mmio(void)
{
    lapic[LAPIC_EOI >> 2] = 0; 
    (void)lapic[0];
}

static inline void lapic_send_eoi_x2apic(void)
{
    const uint32_t MSR_X2APIC_EOI = 0x80B;
    uint32_t lo = 0, hi = 0;
    __asm__ volatile("wrmsr" : : "c"(MSR_X2APIC_EOI), "a"(lo), "d"(hi));
}

uint32_t readAPICRegister(uint32_t reg)
{
    return lapic[reg >> 2];
} 

void writeAPICRegister(uint32_t reg, uint32_t value)
{
    lapic[reg >> 2] = value;
    (void)lapic[0];
}

void send_eoi_isr(uint8_t vector, int using_apic, int x2apic_enabled)
{
    if (using_apic) {
        if (x2apic_enabled) lapic_send_eoi_x2apic();
        else                lapic_send_eoi_mmio();
        return;
    }
    uint8_t irq = (uint8_t)(vector - 0x20);
    sendEOIPIC(irq);
}

void enableAPIC(void) 
{
    if (!checkAPIC()) {
        printf("[ ERROR ] No APIC present!\n");
        return;
    }

    __asm__ volatile ("mov $0xff, %al;"
		  "out %al, $0xa1;"
		  "out %al, $0x21;");
    disableAllIRQs();

    uint32_t lo, hi;
    cpuGetMSR(IA32_APIC_BASE_MSR, &lo, &hi);
    uint64_t msr = ((uint64_t)hi << 32) | lo;

    // bit 11 – EN (xAPIC global enable), bit 10 – EXTD (x2APIC) – not touching it
    msr |= (1ULL << 11);
    lo = (uint32_t)(msr & 0xFFFFFFFF);
    hi = (uint32_t)(msr >> 32);
    cpuSetMSR(IA32_APIC_BASE_MSR, lo, hi);
    uint32_t svr = lapic_read(0xF0);
    svr &= ~0xFF;
    svr |= 0x100 | 0xFF;       // APIC Software Enable + vector 0xFF
    lapic_write(0xF0, svr);
}

void unmask_all_lapic_interrupts(void)
{
    // LVT Timer (0x320) - bit 16 is mask
    uint32_t timer = lapic_read(0x320);
    lapic_write(0x320, timer & ~(1 << 16));
    
    // LVT Thermal Sensor (0x330)
    uint32_t thermal = lapic_read(0x330);
    lapic_write(0x330, thermal & ~(1 << 16));
    
    // LVT Performance Counter (0x340)
    uint32_t perf = lapic_read(0x340);
    lapic_write(0x340, perf & ~(1 << 16));
    
    // LVT LINT0 (0x350)
    uint32_t lint0 = lapic_read(0x350);
    lapic_write(0x350, lint0 & ~(1 << 16));
    
    // LVT LINT1 (0x360)
    uint32_t lint1 = lapic_read(0x360);
    lapic_write(0x360, lint1 & ~(1 << 16));
    
    // LVT Error (0x370)
    uint32_t error = lapic_read(0x370);
    lapic_write(0x370, error & ~(1 << 16));
}

// APIC TIMER

volatile uint64_t apic_timer_ticks = 0;

extern Thread* current_thread;

uint64_t timer_now_ms(void) {
    return apic_timer_ticks;
}

void timer_tick(void)
{
    uint64_t now = apic_timer_ticks;

    while (g_timer_list && g_timer_list->fire_time <= now) {
        TimerEvent* ev = g_timer_list;
        g_timer_list = ev->next;

        ev->callback(ev->user_data);
        kfree(ev);
    }
}

void timer_register(uint64_t delay_ms, timer_callback_t cb, void* user_data)
{
    uint64_t now = timer_now_ms();
    uint64_t fire = now + delay_ms;

    TimerEvent* ev = kmalloc(sizeof(TimerEvent));
    ev->fire_time = fire;
    ev->callback  = cb;
    ev->user_data = user_data;
    ev->next      = NULL;

    if (!g_timer_list || fire < g_timer_list->fire_time) {
        ev->next = g_timer_list;
        g_timer_list = ev;
        return;
    }

    TimerEvent* cur = g_timer_list;
    while (cur->next && cur->next->fire_time <= fire) {
        cur = cur->next;
    }
    ev->next = cur->next;
    cur->next = ev;
}

extern int scheduler_running;

static void APIC_timer_callback(InterruptFrame* frame)
{
    (void)frame;
    apic_timer_ticks++;
    timer_tick();

    if (lapic) {
        writeAPICRegister(APIC_EOI_REGISTER, 0);
    }

    if (!scheduler_running) 
        return;
    
    static int check_counter = 0;
    if (++check_counter >= 10) {
        check_counter = 0;
        
        extern Thread* thread_list;
        Thread* t = thread_list;
        int has_ready = 0;
        int ready_count = 0;
        int done_count = 0;
        int safety = 0;
        
        if (t) {
            do {
                if (t->tid != 0) {
                    if (t->state == THREAD_STATE_READY) {
                        has_ready = 1;
                        ready_count++;
                    } else if (t->state == THREAD_STATE_DONE) {
                        done_count++;
                    }
                }
                t = t->next;
                if (++safety > 20) break;
            } while (t != thread_list);
        }
        
        // Debug: print status every 50 checks (every ~500ms)
        //static int debug_counter = 0;
        //if (++debug_counter >= 50) {
        //    debug_counter = 0;
        //    printf("[DEBUG] User threads: %d READY, %d DONE\n", ready_count, done_count);
        //}
        
        if (!has_ready && scheduler_running && done_count > 0) {
            printf("\n");
            printf("==============================================\n");
            printf("[ KERNEL ] All user threads completed!\n");
            printf("[ KERNEL ] System halting...\n");
            printf("==============================================\n");
            printf("\n");
            
            for (volatile int i = 0; i < 10000000; i++);
            
            for(;;) __asm__ volatile("cli; hlt");
        }
    }

    if (current_thread && current_thread->state == THREAD_STATE_DONE) {
        return;
    }

    if (current_thread && current_thread->state == THREAD_STATE_RUNNING) {
        if (current_thread->remaining_time > 0) {
            current_thread->remaining_time--;
        }
    }

    int should_switch = 0;
    
    if (current_thread && 
        current_thread->remaining_time == 0 && 
        current_thread->state == THREAD_STATE_RUNNING) {
        should_switch = 1;
    }
    
    if (current_thread && 
        current_thread->state != THREAD_STATE_DONE && 
        (frame->cs & 3) == 0 && !should_switch) {
        return;
    }
    
    if (!should_switch) {
        return;
    }

    Thread *next = schedule();
    if (!next || next == current_thread || next->state == THREAD_STATE_DONE) {
        if (next && next->state == THREAD_STATE_DONE) {
            printf("[ KERNEL ] All threads completed. System halting.\n");
            for(;;) __asm__ volatile("cli; hlt");
        }
        return;
    }

    Thread *old = current_thread;
    
    if (old && old->state == THREAD_STATE_RUNNING) {
        if (old->context.user_rip != 0 && (frame->cs & 3) == 3) {
            old->context.user_rip = frame->rip;
            old->context.user_rsp = frame->rsp;
            old->context.rflags = frame->rflags;
        }
        old->state = THREAD_STATE_READY;
    }

    next->state = THREAD_STATE_RUNNING;
    current_thread = next;

    if (next->remaining_time == 0) {
        next->remaining_time = BASE_TIME_QUANTUM * next->priority;
    }
    
    tss.rsp0 = (uint64_t)next->kernel_stack_top;
    
    extern uint16_t gdt_user_code_selector;
    extern uint16_t gdt_user_data_selector;
    extern uint16_t gdt_kernel_code_selector;
    extern uint16_t gdt_kernel_data_selector;
    
    if (next->in_userspace) {
        // Return to user mode
        frame->rip = next->context.user_rip;
        frame->rsp = next->context.user_rsp;
        frame->cs = gdt_user_code_selector;
        frame->ss = gdt_user_data_selector;
        frame->rflags = next->context.rflags;
        
        if (next->context.cr3) {
            __asm__ volatile("mov %0, %%cr3" : : "r"(next->context.cr3) : "memory");
        }
    } else {
        
        frame->rip = next->context.rip;  // task_trampoline
        frame->rsp = next->context.rsp;  // kernel_stack_top  
        frame->cs = gdt_kernel_code_selector;
        frame->ss = gdt_kernel_data_selector;
        frame->rflags = 0x202;  // IF=1
    }
}
 
static int apic_timer_initialized = 0;

void enableAPICTimer(uint32_t dummy)
{
    if (!checkAPIC()) {
        printf("[ ERROR ] No APIC present!\n");
        return;
    }

    writeAPICRegister(0x3E0, 0xB);
    writeAPICRegister(0x320, 0xFF);
    writeAPICRegister(0x380, 0xFFFFFFFF);

    tsc_sleep(10);

    uint32_t current = readAPICRegister(0x390);
    
    uint32_t apic_ticks = 0xFFFFFFFF - current;
    printf("[ APIC ] Calibrated ticks = %u (0x%x)\n", apic_ticks, apic_ticks);

    if (apic_ticks == 0 || apic_ticks == 0xFFFFFFFF) {
        printf("[ ERROR ] Invalid calibration!\n");
        return;
    }

    apic_timer_initialized = 1;

    registerInterruptHandler(APIC_TIMER_VEC, &APIC_timer_callback);
    writeAPICRegister(0x80, 0);

    writeAPICRegister(0x320, APIC_TIMER_VEC | 0x20000);
    writeAPICRegister(0x3E0, 0xB);
    writeAPICRegister(0x380, apic_ticks);
}

// Sleep using APIC timer
void apic_timer_sleep_ms(uint32_t ms) {
    if (!checkAPIC()) {
        printf("[ ERROR ] No APIC present!\n");
        return;
    }

    if (!apic_timer_initialized) {
        printf("[ APIC ] Timer not initialized, fallback\n");
        tsc_sleep(ms);
        return;
    }

    uint64_t start = apic_timer_ticks;
    //printf("[APIC_SLEEP] Starting sleep for %u ms\n", ms);
    //printf("[APIC_SLEEP] Start ticks = %llu\n", start);
    
    // Check if interrupts are enabled
    //uint64_t flags;
    //__asm__ volatile("pushfq; pop %0" : "=r"(flags));
    //printf("[APIC_SLEEP] RFLAGS.IF = %d\n", (int)((flags >> 9) & 1));
    
    // Try with hlt to wait for interrupts
    //uint32_t iterations = 0;
    while ((apic_timer_ticks - start) < ms) {
        __asm__ volatile("hlt");  // Enable interrupts and halt
        //iterations++;
        //if (iterations % 1000 == 0) {
            //printf("[APIC_SLEEP] Still waiting... ticks = %llu\n", apic_timer_ticks);
        //}
    }
    
    //printf("[APIC_SLEEP] Done after %u iterations\n", iterations);
}

void apic_timer_sleep_microseconds(uint32_t us) {
    if (us == 0) return;
    uint32_t ms = us / 1000;
    uint32_t rem_us = us % 1000;
    if (ms > 0) {
        apic_timer_sleep_ms(ms);
    }
    if (rem_us > 0) {
        tsc_sleep(rem_us);
    }
}

void apic_timer_simple_test(void)
{
    printf("[ APIC ] Simple timer test...\n");

    // делитель 16 как у тебя
    writeAPICRegister(0x3E0, 0xB);

    // Маскируем таймер (бит 16 = 1), чтобы не лезли IRQ
    uint32_t lvt = 0x10000 | 0xFE; // masked, vector 0xFE
    writeAPICRegister(0x320, lvt);

    // Ставим большой счётчик
    writeAPICRegister(0x380, 100000000);

    for (int i = 0; i < 10; ++i) {
        uint32_t cur = readAPICRegister(0x390);
        printf("[APIC] cur = %u (0x%x)\n", cur, cur);
    }
}

void apic_init() {
    if (!checkAPIC()) {
        printf("No APIC present!\n");
        return;
    }
    uint64_t apic_msr = rdmsr(IA32_APIC_BASE_MSR);

    uintptr_t apic_phys = apic_msr & 0xFFFFF000ULL;

    mapPage((void*)LAPIC_VIRT, (void*)apic_phys,
            PG_PRESENT | PG_WRITABLE | PG_PWT | PG_PCD);

    lapic = (volatile uint32_t*)LAPIC_VIRT;

    enableAPIC();
}

extern void context_switch(ThreadContext *old, ThreadContext *new);