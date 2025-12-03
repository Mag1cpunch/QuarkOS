#include <stdint.h>

#include <arch/x86_64/pic.h>

#include <hardware/devices/io.h>

/* reinitialize the PIC controllers, giving them specified vector offsets
   rather than 8h and 70h, as configured by default */
/* Helper func */
static uint16_t __pic_get_irq_reg(int ocw3)
{
    /* OCW3 to PIC CMD to get the register values.  PIC2 is chained, and
     * represents IRQs 8-15.  PIC1 is IRQs 0-7, with 2 being the chain */
    IoWrite8(PIC1_COMMAND, ocw3);
    IoWrite8(PIC2_COMMAND, ocw3);
    return (IoRead8(PIC2_COMMAND) << 8) | IoRead8(PIC1_COMMAND);
}
 
/* Returns the combined value of the cascaded PICs irq request register */
uint16_t pic_get_irr(void)
{
    return __pic_get_irq_reg(PIC_READ_IRR);
}
 
/* Returns the combined value of the cascaded PICs in-service register */
uint16_t pic_get_isr(void)
{
    return __pic_get_irq_reg(PIC_READ_ISR);
}

void sendEOIPIC(uint8_t irq)
{
    if(irq >= 8)
        IoWrite8(PIC2_COMMAND,PIC_EOI);

    IoWrite8(PIC1_COMMAND,PIC_EOI);
}

void disableIRQ(uint8_t IRQline) {
    uint8_t irqBit, picMask;

    if (IRQline < 8)
        picMask = IoRead8(PIC1_DATA);
    else
        picMask = IoRead8(PIC2_DATA);

    irqBit = 1 << (IRQline % 8);
    picMask = irqBit | picMask;

    if (IRQline < 8)
        IoWrite8(PIC1_DATA, picMask);
    else
        IoWrite8(PIC2_DATA, picMask);  
}
 
void enableIRQ(uint8_t IRQline) {
    uint8_t irqBit, picMask;

    if (IRQline < 8)
        picMask = IoRead8(PIC1_DATA);
    else
        picMask = IoRead8(PIC2_DATA);

    irqBit = ~(1 << (IRQline % 8));
    picMask = irqBit & picMask;

    if (IRQline < 8)
        IoWrite8(PIC1_DATA, picMask);
    else
        IoWrite8(PIC2_DATA, picMask);    
}

void disableAllIRQs() {
    for (uint8_t i = 0; i < 16; i++) {
        disableIRQ(i);
    }
}

void enableAllIRQs() {
    for (uint8_t i = 0; i < 16; i++) {
        enableIRQ(i);
    }
}

void remapPIC(int masterOff, int slaveOff)
{
    IoWrite8(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);  // starts the initialization sequence (in cascade mode)
    IoWait();
    IoWrite8(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    IoWait();
    IoWrite8(PIC1_DATA, masterOff);                 // ICW2: Master PIC vector offset
    IoWait();
    IoWrite8(PIC2_DATA, slaveOff);                 // ICW2: Slave PIC vector offset
    IoWait();
    IoWrite8(PIC1_DATA, 4);                       // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
    IoWait();
    IoWrite8(PIC2_DATA, 2);                       // ICW3: tell Slave PIC its cascade identity (0000 0010)
    IoWait();
 
    IoWrite8(PIC1_DATA, ICW4_8086);
    IoWait();
    IoWrite8(PIC2_DATA, ICW4_8086);
    IoWait();

    IoWrite8(PIC1_DATA, 0);
    IoWrite8(PIC2_DATA, 0);
}

void PIC_init()
{
    __asm__ __volatile__("cli");
    remapPIC(0x20, 0x28);
    __asm__ __volatile__("sti");
}