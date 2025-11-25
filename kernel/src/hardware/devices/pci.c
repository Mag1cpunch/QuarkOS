#include <stdio.h>
#include <stdbool.h>

#include <arch/x86_64/isr.h>
#include <arch/x86_64/apic/ioapic.h>

#include <hardware/devices/io.h>
#include <hardware/devices/pci.h>

uint16_t pciConfigReadWord(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) 
{
    uint32_t lbus  = (uint32_t)bus;
    uint32_t ldevice = (uint32_t)device;
    uint32_t lfunc = (uint32_t)func;
    uint16_t tmp = 0;
 
    uint32_t address = (uint32_t)((lbus << 16) | (ldevice << 11) | (lfunc << 8) | (offset & 0xFC) | ((uint32_t)0x80000000));
 
    out(0xCF8, address);
    tmp = (uint16_t)((in(0xCFC) >> ((offset & 2) * 8)) & 0xFFFF);
    return tmp;
}

uint16_t getVendorID(uint16_t bus, uint16_t device, uint16_t function) 
{
    return pciConfigReadWord(bus, device, function, 0);
}

uint16_t getHeaderType(uint16_t bus, uint16_t device, uint16_t function) 
{
    return pciConfigReadWord(bus, device, function, (0xC + 2) & 0xFF);
}

bool checkFunction(uint8_t bus, uint8_t device, uint8_t function) 
{
    return getHeaderType(bus, device, function) != 0xFFFF;
}

void checkDevice(uint8_t bus, uint8_t device) 
{
    uint8_t function = 0;

    if (!checkFunction(bus, device, 0)) return;

    uint16_t headerType = getHeaderType(bus, device, function);
    if ((headerType & 0x80) != 0) 
    {
        for (function = 1; function < 8; function++) 
        {
            if (getVendorID(bus, device, function) != 0xFFFF)
            {
                printf("PCI Device found at %02x:%02x.%x - Vendor ID: %04x\n", 
                       bus, device, function, 
                       getVendorID(bus, device, function));
            }
        }
    }
}

void checkAllBuses(void) 
{
    for (size_t bus = 0; bus < 256; bus++) 
    {
        for (size_t device = 0; device < 32; device++) 
        {
            checkDevice(bus, device);
        }
    }
}

static void pciInterruptHandler(InterruptFrame* frame) 
{
    printf("%d\n", frame->int_no);
}

void enablePCIInterrupts(uint8_t bus, uint8_t device, uint8_t function, size_t ioapicaddr) 
{
    uint16_t interrupt = pciConfigReadWord(bus, device, function, 0x3E);
    uint16_t interrupt_line = interrupt & 0xFF;
    uint16_t interrupt_pin = interrupt >> 8;

    // TODO: use AML to figure out which interrupt line to use

    writeIOAPIC(ioapicaddr, interrupt_line * 2 + 0x10, 0x20 + interrupt_line);
    registerInterruptHandler(0x20 + interrupt_line, &pciInterruptHandler);
}

void checkMSI(uint8_t bus, uint8_t device, uint8_t func)
{
    uint16_t status = pciConfigReadWord(bus, device, func, 0x4);
    printf("%d\n", pciConfigReadWord(bus, device, func, 0x20));
    printf("%d\n", pciConfigReadWord(bus, device, func, 0x22));
    printf("%d\n", pciConfigReadWord(bus, device, func, 0x24));
    printf("%d\n", pciConfigReadWord(bus, device, func, 0x26));
    if (status & 0x10) 
    {
        uint16_t capabilities_pointer = pciConfigReadWord(bus, device, func, 0x36);
        uint16_t capability = pciConfigReadWord(bus, device, func, capabilities_pointer + 0x2);
        if ((capability & 0xFF) == 5) 
        {
            printf("MSI Enabled!\n");
        }
        else 
        {
            printf("%d\n", capability >> 8);
        }
    }
    else
    {
        printf("No MSI!");
    }
}