#include <hardware/acpi/description_tables/fadt.h>
#include <hardware/devices/io.h>

FADT* fadt;

void initFADT() 
{
    fadt = (FADT*) findHeader("FACP");
}

void enableACPI() 
{
    IoWrite8(fadt->SMI_command_port, fadt->ACPI_enable);
    while ((IoRead16(fadt->PM1a_control_block) & 1) == 0);
}