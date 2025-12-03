#ifndef APIC_H
#define APIC_H

#include <stdint.h>
#include <stddef.h>

#define LAPIC_ID        0x020
#define LAPIC_VERSION   0x030
#define LAPIC_EOI       0x0B0

#define APIC_TIMER_VEC 0x40

typedef void (*timer_callback_t)(void* user);

typedef struct TimerEvent {
    uint64_t        fire_time;
    timer_callback_t callback;
    void*           user_data;
    struct TimerEvent* next;
} TimerEvent;

extern TimerEvent* g_timer_list;

void cpuGetMSR(uint32_t msr, uint32_t *lo, uint32_t *hi);
void cpuSetMSR(uint32_t msr, uint32_t lo, uint32_t hi);
uint32_t readAPICRegister(uint32_t reg);
void writeAPICRegister(uint32_t reg, uint32_t value);
void enableAPIC();
void timer_register(uint64_t delay_ms, void (*callback)(void*), void* user_data);
void enableAPICTimer(uint32_t frequency);
void apic_timer_sleep_ms(uint32_t ms);
void apic_init();
void apic_timer_simple_test(void);
void send_eoi_isr(uint8_t vector, int using_apic, int x2apic_enabled);

#endif