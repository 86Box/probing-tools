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


char dummy_buf[256];


void
probe_intel(uint8_t dev, uint8_t func)
{
    uint16_t acpi_base;
    int type;

    acpi_base = pci_readw(0, dev, func, 0x40);
    printf("ACPI base register = %04X\n", acpi_base);
    acpi_base &= 0xff80;

    printf("PMCNTRL 04=%04X 40=%04X\n", inw(acpi_base | 0x04), inw(acpi_base | 0x40));

    printf("Enter hex sleep type to try: ");
    scanf("%x%*c", &type);

    printf("Press ENTER to try sleep %02X through 40...", type);
    gets(dummy_buf);
    outw(acpi_base | 0x40, 0x2000 | (type << 10));

    printf("Nothing?\nPress ENTER to try sleep %02X through 04...", type);
    gets(dummy_buf);
    outw(acpi_base | 0x04, 0x2000 | (type << 10));
    printf("Nothing still?\n");
}


void
probe_via(uint8_t dev, uint8_t func, uint8_t is586)
{
    uint16_t acpi_base;
    int type;

    acpi_base = pci_readw(0, dev, func, is586 ? 0x20 : 0x48);
    printf("ACPI base register = %04X\n", acpi_base);
    acpi_base &= 0xff00;

    printf("PMCNTRL 04=%04X F0=%04X\n", inw(acpi_base | 0x04), inw(acpi_base | 0xf0));

    printf("Enter hex sleep type to try: ");
    scanf("%x%*c", &type);

    printf("Press ENTER to try sleep %02X through F0...", type);
    gets(dummy_buf);
    outw(acpi_base | 0xf0, 0x2000 | (type << 10));

    printf("Nothing?\nPress ENTER to try sleep %02X through 04...", type);
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

#ifdef __WATCOMC__
    /* Disable stdout buffering. */
    setbuf(stdout, NULL);
#endif

#ifndef DEBUG
    /* Test for PCI presence. */
    outl(0xcf8, 0x80000000);
    cf8 = inl(0xcf8);
    if (cf8 == 0xffffffff) {
	printf("Port CF8h is not responding. Does this system even have PCI?\n");
	return 1;
    }
#endif

    for (dev = 0; dev < 32; dev++) {
    	if (!(pci_readb(0, dev, 0, 0x0e) & 0x80))
    		continue;

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

    printf("No interesting ACPI device found\n");
    return 1;
}
