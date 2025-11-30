#ifndef XHCI_H
#define XHCI_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    volatile uint8_t  CAPLENGTH;      // 0x00: Capability Register Length
    volatile uint8_t  RSVD;           // 0x01: Reserved
    volatile uint16_t HCIVERSION;     // 0x02: Interface Version Number
    volatile uint32_t HCSPARAMS1;     // 0x04: Structural Parameters 1
    volatile uint32_t HCSPARAMS2;     // 0x08: Structural Parameters 2
    volatile uint32_t HCSPARAMS3;     // 0x0C: Structural Parameters 3
    volatile uint32_t HCCPARAMS1;     // 0x10: Capability Parameters 1
    volatile uint32_t DBOFF;          // 0x14: Doorbell Offset
    volatile uint32_t RTSOFF;         // 0x18: Runtime Registers Space Offset
    volatile uint32_t HCCPARAMS2;     // 0x1C: Capability Parameters 2
} xhci_cap_regs_t;

typedef struct {
    volatile uint32_t USBCMD;      // 0x00: USB Command
    volatile uint32_t USBSTS;      // 0x04: USB Status
    volatile uint32_t PAGESIZE;    // 0x08: Page Size
    uint32_t _reserved0[1];        // 0x0C: Reserved
    volatile uint32_t DNCTRL;      // 0x14: Device Notification Control
    volatile uint32_t CRCR;        // 0x18: Command Ring Control
    uint32_t _reserved1[4];        // 0x1C-0x2C: Reserved
    volatile uint64_t DCBAAP;      // 0x30: Device Context Base Address Array Pointer
    volatile uint32_t CONFIG;      // 0x38: Configure
} xhci_op_regs_t;

// xHCI Port Registers (per port, at offset 0x400 + N*0x10)
typedef struct {
    volatile uint32_t PORTSC;     // 0x00: Port Status and Control
    volatile uint32_t PORTPMSC;   // 0x04: Port Power Management Status and Control
    volatile uint32_t PORTLI;     // 0x08: Port Link Info
    uint32_t reserved;            // 0x0C: Reserved
} xhci_port_regs_t;

// xHCI Runtime Registers (at RTSoFF)
typedef struct {
    volatile uint32_t MFINDEX;    // 0x00: Microframe Index
    uint32_t reserved[7];         // 0x04-0x1C: Reserved
} xhci_runtime_regs_t;

// xHCI Interrupter Register Set (at RTSoFF + 0x20 + N*0x20)
typedef struct {
    volatile uint32_t IMAN;       // 0x00: Interrupter Management
    volatile uint32_t IMOD;       // 0x04: Interrupter Moderation
    volatile uint32_t ERSTSZ;     // 0x08: Event Ring Segment Table Size
    uint32_t reserved;            // 0x0C: Reserved
    volatile uint64_t ERSTBA;     // 0x10: Event Ring Segment Table Base Address
    volatile uint64_t ERDP;       // 0x18: Event Ring Dequeue Pointer
} xhci_interrupter_regs_t;

// xHCI Doorbell Registers (array, at DBOFF)
typedef struct {
    volatile uint32_t DB;         // 0x00: Doorbell register (one per slot/endpoint)
} xhci_doorbell_t;

void xhci_init();

#endif // XHCI_H