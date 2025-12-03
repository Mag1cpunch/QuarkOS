#ifndef FADT_H
#define FADT_H

#include <stdint.h>

#include <hardware/acpi/acpi.h>

typedef struct FADT {
    struct ACPISDTHeader h;
    uint32_t firmware_control;
    uint32_t dsdt;

    uint8_t reserved;

    uint8_t profile;
    uint16_t sci_irq;
    uint32_t smi_command_port;
    uint8_t acpi_enable;
    uint8_t acpi_disable;
    uint8_t s4bios_req;
    uint8_t pstate_control;
    uint32_t pm1a_event_block;
    uint32_t pm1b_event_block;
    uint32_t pm1a_control_block;
    uint32_t pm1b_control_block;
    uint32_t pm2_control_block;
    uint32_t pm_timer_block;
    uint32_t gpe0_block;
    uint32_t gpe1_block;
    uint8_t pm1_event_length;
    uint8_t pm1_control_length;
    uint8_t pm2_control_length;
    uint8_t pm_timer_length;
    uint8_t gpe0_length;
    uint8_t gpe1_length;
    uint8_t gpe1_base;
    uint8_t cstate_control;
    uint16_t worst_c2_latency;
    uint16_t worst_c3_latency;
    uint16_t flush_size;
    uint16_t flush_stride;
    uint8_t duty_offset;
    uint8_t duty_width;

    uint8_t day_alarm;
    uint8_t month_alarm;
    uint8_t century;

    uint16_t iapc_boot_flags;
    uint8_t reserved2;
    uint32_t flags;

    GenericAddressStructure reset_register;
    uint8_t reset_command;
    uint16_t arm_boot_flags;
    uint8_t minor_version;
    uint8_t reserved3[3];

    uint64_t x_firmware_control;
    uint64_t x_dsdt;

    GenericAddressStructure x_pm1a_event_block;
    GenericAddressStructure x_pm1b_event_block;
    GenericAddressStructure x_pm1a_control_block;
    GenericAddressStructure x_pm1b_control_block;
    GenericAddressStructure x_pm2_control_block;
    GenericAddressStructure x_pm_timer_block;
    GenericAddressStructure x_gpe0_block;
    GenericAddressStructure x_gpe1_block;
    GenericAddressStructure sleep_control_reg;
    GenericAddressStructure sleep_status_reg;
} __attribute__((packed)) FADT;

// Global pointer to the FADT (set by ACPI init)
extern struct FADT *fadt;

#endif /* FADT_H */