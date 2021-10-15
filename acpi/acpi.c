/*
 * 86Box	A hypervisor and IBM PC system emulator that specializes in
 *		running old operating systems and software designed for IBM
 *		PC systems and compatibles from 1981 through fairly recent
 *		system designs based on the PCI bus.
 *
 *		This file is part of the 86Box Probing Tools distribution.
 *
 *		ACPI probing tool.
 *
 *
 *
 * Authors:	RichardG, <richardg867@gmail.com>
 *
 *		Copyright 2021 RichardG.
 *
 * ┌──────────────────────────────────────────────────────────────┐
 * │ This file is UTF-8 encoded. If this text is surrounded by    │
 * │ garbage, please tell your editor to open this file as UTF-8. │
 * └──────────────────────────────────────────────────────────────┘
 */
#include <inttypes.h>
#include <stdio.h>
#include "clib.h"


static char	dummy_buf[256];


void
probe_intel(uint8_t dev, uint8_t func)
{
    uint16_t acpi_base;
    int type;

    /* Read and print ACPI I/O base. */
    acpi_base = pci_readw(0, dev, func, 0x40);
    printf("ACPI base register = %04X\n", acpi_base);
    acpi_base &= 0xff80;

    /* Print contents of both PM1a control registers. */
    printf("PMCNTRL 04=%04X 40=%04X\n", inw(acpi_base | 0x04), inw(acpi_base | 0x40));

    /* Prompt for sleep type. */
    printf("Enter hex sleep type to try: ");
    scanf("%x%*c", &type);

    /* Try sleep through alternate port. */
    printf("Press ENTER to try sleep %02X through register 40...", type);
    gets(dummy_buf);
    outw(acpi_base | 0x40, 0x2000 | (type << 10));

    /* Try sleep through main port. */
    printf("Nothing?\nPress ENTER to try sleep %02X through register 04...", type);
    gets(dummy_buf);
    outw(acpi_base | 0x04, 0x2000 | (type << 10));
    printf("Nothing still?\n");
}


void
probe_via(uint8_t dev, uint8_t func, uint8_t is586)
{
    uint16_t acpi_base;
    int type;

    /* Read and print ACPI I/O base. */
    acpi_base = pci_readw(0, dev, func, is586 ? 0x20 : 0x48);
    printf("ACPI base register = %04X\n", acpi_base);
    acpi_base &= 0xff00;

    /* Print contents of both PM1a control registers. */
    printf("PMCNTRL 04=%04X F0=%04X\n", inw(acpi_base | 0x04), inw(acpi_base | 0xf0));

    /* Prompt for sleep type. */
    printf("Enter hex sleep type to try: ");
    scanf("%x%*c", &type);

    /* Try sleep through alternate port. */
    printf("Press ENTER to try sleep %02X through register F0...", type);
    gets(dummy_buf);
    outw(acpi_base | 0xf0, 0x2000 | (type << 10));

    /* Try sleep through main port. */
    printf("Nothing?\nPress ENTER to try sleep %02X through register 04...", type);
    gets(dummy_buf);
    outw(acpi_base | 0x04, 0x2000 | (type << 10));
    printf("Nothing still?\n");
}


int
main(int argc, char **argv)
{
    uint8_t dev, func;
    uint16_t ven_id, dev_id;
    uint32_t cf8;

    /* Disable stdout buffering. */
    term_unbuffer_stdout();

    /* Initialize PCI, and exit in case of failure. */
    if (!pci_init())
	return 1;

    /* Scan PCI bus 0. */
    for (dev = 0; dev < 32; dev++) {
    	/* Single-function devices are definitely not the southbridge. */
	if (!(pci_readb(0, dev, 0, 0x0e) & 0x80))
		continue;

	/* Determine and execute southbridge-specific function. */
	ven_id = pci_readw(0, dev, 3, 0);
	dev_id = pci_readw(0, dev, 3, 2);
	if ((ven_id == 0x8086) && (dev_id == 0x7113)) {
		printf("Found PIIX4 ACPI at device %02X function %d\n", dev, 3);
		probe_intel(dev, 3);
		return 0;
	} else if ((ven_id == 0x1055) && (dev_id == 0x9463)) {
		printf("Found SLC90E66 ACPI at device %02X function %d\n", dev, 3);
		probe_intel(dev, 3);
		return 0;
	} else if ((ven_id == 0x1106) && (dev_id == 0x3040)) {
		printf("Found VT82C586 ACPI at device %02X function %d\n", dev, 3);
		probe_via(dev, 3, 1);
		return 0;
	} else if ((ven_id == 0x1106) && (dev_id == 0x3050)) {
		printf("Found VT82C596 ACPI at device %02X function %d\n", dev, 3);
		probe_via(dev, 3, 0);
		return 0;
	} else {
		ven_id = pci_readw(0, dev, 4, 0);
		dev_id = pci_readw(0, dev, 4, 2);
		if ((ven_id == 0x1106) && (dev_id == 0x3057)) {
			printf("Found VT82C686 ACPI at device %02X function %d\n", dev, 4);
			probe_via(dev, 4, 0);
			return 0;
		}
	}
    }

    /* Nothing of interest found. */
    printf("No ACPI device of interest found!\n");
    return 1;
}
