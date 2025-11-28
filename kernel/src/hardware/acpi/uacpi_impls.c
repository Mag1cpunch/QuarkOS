#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <limine.h>

#include <arch/x86_64/cpu.h>

#include <hardware/acpi/acpi.h>
#include <hardware/acpi/uacpi/uacpi.h>
#include <hardware/acpi/descriptor_tables/fadt.h>
#include <hardware/requests.h>
#include <hardware/memory/paging.h>
#include <hardware/devices/io.h>
#include <arch/x86_64/apic/apic.h>

#include <system/multitasking/spinlock.h>
#include <system/multitasking/tasksched.h>

uacpi_status uacpi_kernel_get_rsdp(uacpi_phys_addr *out_rsdp_address) {
    struct limine_rsdp_response *rsdp_response = get_rsdp();

    if (rsdp_response == NULL)
        return UACPI_STATUS_NOT_FOUND;

    *out_rsdp_address = (uacpi_phys_addr)rsdp_response->address;
    return UACPI_STATUS_OK;
}

void *uacpi_kernel_map(uacpi_phys_addr addr, uacpi_size len) {
    uintptr_t aligned_phys = addr & ~(PAGE_SIZE - 1);
    uintptr_t offset = addr - aligned_phys;
    uacpi_size aligned_len = len + offset;
    if (aligned_len % PAGE_SIZE != 0) {
        aligned_len += PAGE_SIZE - (aligned_len % PAGE_SIZE);
    }

    void *aligned_virt = malloc(aligned_len);
    if (aligned_virt == NULL) {
        return NULL;
    }

    for (uacpi_size i = 0; i < aligned_len; i += PAGE_SIZE) {
        void *virt_page = (void *)((uintptr_t)aligned_virt + i);
        void *phys_page = (void *)(aligned_phys + i);
        mapPage(virt_page, phys_page, PG_WRITABLE | PG_NX);
    }

    return (void *)((uintptr_t)aligned_virt + offset);
}

void uacpi_kernel_unmap(void *addr, uacpi_size len) {
    uintptr_t aligned_virt = (uintptr_t)addr & ~(PAGE_SIZE - 1);
    uintptr_t offset = (uintptr_t)addr - aligned_virt;
    uacpi_size aligned_len = len + offset;
    if (aligned_len % PAGE_SIZE != 0) {
        aligned_len += PAGE_SIZE - (aligned_len % PAGE_SIZE);
    }

    for (uacpi_size i = 0; i < aligned_len; i += PAGE_SIZE) {
        void *virt_page = (void *)(aligned_virt + i);
        unmapPage(virt_page);
    }

    free((void *)aligned_virt);
}

void uacpi_kernel_log(uacpi_log_level log_level, const uacpi_char* msg) {
    printf("[uACPI] %s\n", msg);
}

void *uacpi_kernel_alloc(uacpi_size size) {
    return malloc(size);
}

void uacpi_kernel_free(void *mem) {
    free(mem);
}

uacpi_handle uacpi_kernel_create_mutex(void) {
    spinlock_t *lock = malloc(sizeof(spinlock_t));
    if (!lock) return NULL;
    spinlock_init(lock);
    return (uacpi_handle)lock;
}

void uacpi_kernel_free_mutex(uacpi_handle handle) {
    spinlock_t *lock = (spinlock_t *)handle;
    free(lock);
}

uacpi_thread_id uacpi_kernel_get_thread_id(void) {
    return (uacpi_thread_id)current_thread->tid;
}

uacpi_status uacpi_kernel_acquire_mutex(uacpi_handle handle, uacpi_u16 timeout __attribute__((unused))) {
    spinlock_t *lock = (spinlock_t *)handle;
    spinlock_acquire(lock);
    return UACPI_STATUS_OK;
}

void uacpi_kernel_release_mutex(uacpi_handle handle) {
    spinlock_t *lock = (spinlock_t *)handle;
    spinlock_release(lock);
}

uacpi_handle uacpi_kernel_create_event(void) {
    uacpi_event_t *ev = malloc(sizeof(uacpi_event_t));
    if (!ev) return NULL;
    spinlock_init(&ev->lock);
    ev->signaled = 0;
    return (uacpi_handle)ev;
}

void uacpi_kernel_free_event(uacpi_handle handle) {
    uacpi_event_t *ev = (uacpi_event_t *)handle;
    free(ev);
}

uacpi_bool uacpi_kernel_wait_for_event(uacpi_handle handle, uacpi_u16 timeout) {
    uacpi_event_t *ev = (uacpi_event_t *)handle;
    if (timeout == 0xFFFF) {
        for (;;) {
            spinlock_acquire(&ev->lock);
            if (ev->signaled) {
                ev->signaled = 0;
                spinlock_release(&ev->lock);
                return UACPI_TRUE;
            }
            spinlock_release(&ev->lock);
        }
    } else {
        return UACPI_FALSE;
    }
}

void uacpi_kernel_signal_event(uacpi_handle handle) {
    uacpi_event_t *ev = (uacpi_event_t *)handle;
    spinlock_acquire(&ev->lock);
    ev->signaled = 1;
    spinlock_release(&ev->lock);
}

void uacpi_kernel_reset_event(uacpi_handle handle) {
    uacpi_event_t *ev = (uacpi_event_t *)handle;
    spinlock_acquire(&ev->lock);
    ev->signaled = 0;
    spinlock_release(&ev->lock);
}

uacpi_status uacpi_kernel_handle_firmware_request(uacpi_firmware_request *req) {
    switch (req->type) {
        case UACPI_FIRMWARE_REQUEST_TYPE_BREAKPOINT:
            printf("[ uACPI ] Breakpoint firmware request received.\n");
            hcf();
            return UACPI_STATUS_OK;
        case UACPI_FIRMWARE_REQUEST_TYPE_FATAL:
            printf("[ uACPI ] Fatal firmware request received.\n");
            hcf();
            return UACPI_STATUS_OK;
        default:
            return UACPI_STATUS_UNIMPLEMENTED;
    }
}

uacpi_status uacpi_kernel_install_interrupt_handler(
    uacpi_u32 irq, uacpi_interrupt_handler, uacpi_handle ctx,
    uacpi_handle *out_irq_handle
) { return UACPI_STATUS_UNIMPLEMENTED; }

uacpi_status uacpi_kernel_uninstall_interrupt_handler(
    uacpi_interrupt_handler, uacpi_handle irq_handle
) { return UACPI_STATUS_UNIMPLEMENTED; }

uacpi_handle uacpi_kernel_create_spinlock(void) {
    return uacpi_kernel_create_mutex();
}

void uacpi_kernel_free_spinlock(uacpi_handle handle) {
    uacpi_kernel_free_mutex(handle);
}

uacpi_cpu_flags uacpi_kernel_lock_spinlock(uacpi_handle handle) {
    spinlock_t *lock = (spinlock_t *)handle;
    spinlock_acquire(lock);
    return 0;
}

void uacpi_kernel_unlock_spinlock(uacpi_handle handle, uacpi_cpu_flags flags __attribute__((unused))) {
    spinlock_t *lock = (spinlock_t *)handle;
    spinlock_release(lock);
}

uacpi_status uacpi_kernel_schedule_work(
    uacpi_work_type, uacpi_work_handler, uacpi_handle ctx
) {
    return UACPI_STATUS_UNIMPLEMENTED;
}

uacpi_status uacpi_kernel_wait_for_work_completion(void) {
    return UACPI_STATUS_UNIMPLEMENTED;
}

extern volatile uint64_t apic_timer_ticks;
uacpi_u64 uacpi_kernel_get_nanoseconds_since_boot(void) {
    return apic_timer_ticks * 1000000ULL; // ms to ns
}

void uacpi_kernel_stall(uacpi_u8 usec) {
    for (volatile uacpi_u64 i = 0; i < usec * 100; i++);
}

void uacpi_kernel_sleep(uacpi_u64 msec) {
    apic_timer_sleep_ms(msec);
}

uacpi_status uacpi_kernel_io_map(
    uacpi_io_addr base, uacpi_size len, uacpi_handle *out_handle
) {
    uacpi_io_map_t *map = malloc(sizeof(uacpi_io_map_t));
    if (!map) return UACPI_STATUS_OUT_OF_MEMORY;
    map->base = base;
    map->len = len;
    *out_handle = (uacpi_handle)map;
    return UACPI_STATUS_OK;
}

void uacpi_kernel_io_unmap(uacpi_handle handle) {
    uacpi_io_map_t *map = (uacpi_io_map_t *)handle;
    free(map);
}

uacpi_status uacpi_kernel_io_read8(
    uacpi_handle handle, uacpi_size offset, uacpi_u8 *out_value
) {
    uacpi_io_map_t *map = (uacpi_io_map_t *)handle;
    if (offset >= map->len) {
        return UACPI_STATUS_INVALID_ARGUMENT;
    }
    *out_value = IoRead8((uint16_t)(map->base + offset));
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_read16(
    uacpi_handle handle, uacpi_size offset, uacpi_u16 *out_value
) {
    uacpi_io_map_t *map = (uacpi_io_map_t *)handle;
    if (offset + 1 >= map->len) {
        return UACPI_STATUS_INVALID_ARGUMENT;
    }
    *out_value = IoRead16((uint16_t)(map->base + offset));
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_read32(
    uacpi_handle handle, uacpi_size offset, uacpi_u32 *out_value
) {
    uacpi_io_map_t *map = (uacpi_io_map_t *)handle;
    if (offset + 3 >= map->len) {
        return UACPI_STATUS_INVALID_ARGUMENT;
    }
    *out_value = IoRead32((uint16_t)(map->base + offset));
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_write8(
    uacpi_handle handle, uacpi_size offset, uacpi_u8 in_value
) {
    uacpi_io_map_t *map = (uacpi_io_map_t *)handle;
    if (offset >= map->len) {
        return UACPI_STATUS_INVALID_ARGUMENT;
    }
    IoWrite8((uint16_t)(map->base + offset), in_value);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_write16(
    uacpi_handle handle, uacpi_size offset, uacpi_u16 in_value
) {
    uacpi_io_map_t *map = (uacpi_io_map_t *)handle;
    if (offset + 1 >= map->len) {
        return UACPI_STATUS_INVALID_ARGUMENT;
    }
    IoWrite16((uint16_t)(map->base + offset), in_value);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_io_write32(
    uacpi_handle handle, uacpi_size offset, uacpi_u32 in_value
) {
    uacpi_io_map_t *map = (uacpi_io_map_t *)handle;
    if (offset + 3 >= map->len) {
        return UACPI_STATUS_INVALID_ARGUMENT;
    }
    IoWrite32((uint16_t)(map->base + offset), in_value);
    return UACPI_STATUS_OK;
}

uacpi_status uacpi_kernel_pci_read8(
    uacpi_handle device, uacpi_size offset, uacpi_u8 *value
) { return UACPI_STATUS_UNIMPLEMENTED; }

uacpi_status uacpi_kernel_pci_read16(
    uacpi_handle device, uacpi_size offset, uacpi_u16 *value
) { return UACPI_STATUS_UNIMPLEMENTED; }

uacpi_status uacpi_kernel_pci_read32(
    uacpi_handle device, uacpi_size offset, uacpi_u32 *value
) { return UACPI_STATUS_UNIMPLEMENTED; }

uacpi_status uacpi_kernel_pci_write8(
    uacpi_handle device, uacpi_size offset, uacpi_u8 value
) { return UACPI_STATUS_UNIMPLEMENTED; }

uacpi_status uacpi_kernel_pci_write16(
    uacpi_handle device, uacpi_size offset, uacpi_u16 value
) { return UACPI_STATUS_UNIMPLEMENTED; }

uacpi_status uacpi_kernel_pci_write32(
    uacpi_handle device, uacpi_size offset, uacpi_u32 value
) { return UACPI_STATUS_UNIMPLEMENTED; }

uacpi_status uacpi_kernel_pci_device_open(
    uacpi_pci_address address, uacpi_handle *out_handle
) {
    return UACPI_STATUS_UNIMPLEMENTED;
}

void uacpi_kernel_pci_device_close(uacpi_handle) {
    return;
}