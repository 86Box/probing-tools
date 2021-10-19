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
    uint8_t status;
    uint16_t acpi_base, devctl;
    int type;

    /* Read and print ACPI I/O base. */
    pci_writew(0, dev, func, 0x40, 0x4001);
    acpi_base = pci_readw(0, dev, func, 0x40);
    printf("ACPI base register = %04X\n", acpi_base);
    acpi_base &= 0xffc0;

    /* Read and print SMI traps. */
    devctl = inl(acpi_base + 0x2e);
    status = pci_readb(0, dev, func, 0x62);
    printf("SMI traps: Dev9   %04X+%X  Decode[%c] Trap[%c]\n", pci_readw(0, dev, func, 0x60), status & 0x0f,
	   (status & 0x20) ? '√' : ' ', (devctl & 0x0008) ? '√' : ' ');
    status = pci_readb(0, dev, func, 0x66);
    printf("           Dev10  %04X+%X  Decode[%c] Trap[%c]\n", pci_readw(0, dev, func, 0x64), status & 0x0f,
	   (status & 0x20) ? '√' : ' ', (devctl & 0x0020) ? '√' : ' ');
    status = pci_readb(0, dev, func, 0x6a);
    printf("           Dev12  %04X+%X  Decode[%c] Trap[%c]\n", pci_readw(0, dev, func, 0x68), status & 0x0f,
	   (status & 0x10) ? '√' : ' ', (devctl & 0x0100) ? '√' : ' ');
    status = pci_readb(0, dev, func, 0x72);
    printf("           Dev13  %04X+%X  Decode[%c] Trap[%c]\n", pci_readw(0, dev, func, 0x70), status & 0x0f,
	   (status & 0x10) ? '√' : ' ', (devctl & 0x0200) ? '√' : ' ');

    /* Print contents of both PM1a control registers. */
    printf("PMCNTRL: %04X=%04X %04X=%04X\n", acpi_base + 0x04, inw(acpi_base + 0x04), acpi_base + 0x40, inw(acpi_base + 0x40));

    /* Prompt for sleep type. */
    printf("Enter hex sleep type to try: ");
    scanf("%x%*c", &type);

    /* Try sleep through alternate port. */
    printf("Press ENTER to try sleep %02X through register %04X...", type, acpi_base + 0x40);
    gets(dummy_buf);
    outw(acpi_base + 0x40, 0x2000 | (type << 10));

    /* Try sleep through main port. */
    printf("Nothing?\nPress ENTER to try sleep %02X through register %04X...", type, acpi_base + 0x04);
    gets(dummy_buf);
    outw(acpi_base + 0x04, 0x2000 | (type << 10));
    printf("Nothing still?\n");
}


void
probe_via(uint8_t dev, uint8_t func, uint16_t dev_id)
{
    uint8_t mask;
    uint16_t acpi_base, glben, status;
    int type;

    /* Read and print ACPI I/O base. */
    acpi_base = pci_readw(0, dev, func, (dev_id == 0x3040) ? 0x20 : 0x48);
    printf("ACPI base register = %04X\n", acpi_base);
    acpi_base &= 0xff00;

    /* Print contents of both PM1a control registers. */
    printf("PMCNTRL: %04X=%04X %04X=%04X\n", acpi_base + 0x04, inw(acpi_base + 0x04), acpi_base + 0xf0, inw(acpi_base + 0xf0));

    /* Read and print SMI traps. */
    if (dev_id >= 0x3050) {
	mask = pci_readb(0, dev, 0, 0x80);
	glben = inw(acpi_base + 0x2a);
	if (dev_id == 0x3050) {
		status = pci_readw(0, dev, 0, 0x76);
		printf("SMI traps: PCS0  %04X+%X  Decode[%c] IntIO[%c] Trap[%c]\n", pci_readw(0, dev, 0, 0x78), mask & 0x0f,
		       (status & 0x0010) ? '√' : ' ', (status & 0x1000) ? '√' : ' ', (glben & 0x4000) ? '√' : ' ');
		printf("           PCS1  %04X+%X  Decode[%c] IntIO[%c] Trap[%c]\n", pci_readw(0, dev, 0, 0x7a), (mask >> 4) & 0x0f,
		       (status & 0x0020) ? '√' : ' ', (status & 0x2000) ? '√' : ' ', (glben & 0x8000) ? '√' : ' ');
	} else {
		status = pci_readw(0, dev, 0, 0x8b);
		printf("SMI traps: PCS0  %04X+%X  Decode[%c] IntIO[%c] Trap[%c]\n", pci_readw(0, dev, 0, 0x78), mask & 0x0f,
		       (status & 0x0100) ? '√' : ' ', (status & 0x1000) ? '√' : ' ', (glben & 0x4000) ? '√' : ' ');
		printf("           PCS1  %04X+%X  Decode[%c] IntIO[%c] Trap[%c]\n", pci_readw(0, dev, 0, 0x7a), (mask >> 4) & 0x0f,
		       (status & 0x0200) ? '√' : ' ', (status & 0x2000) ? '√' : ' ', (glben & 0x8000) ? '√' : ' ');
		mask = pci_readb(0, dev, 0, 0x8a);
		glben = inw(acpi_base + 0x42); /* extended I/O trap */
		printf("           PCS2  %04X+%X  Decode[%c] IntIO[%c] Trap[%c]\n", pci_readw(0, dev, 0, 0x8c), mask & 0x0f,
		       (status & 0x0400) ? '√' : ' ', (status & 0x4000) ? '√' : ' ', (glben & 0x0001) ? '√' : ' ');
		printf("           PCS3  %04X+%X  Decode[%c] IntIO[%c] Trap[%c]\n", pci_readw(0, dev, 0, 0x8e), (mask >> 4) & 0x0f,
		       (status & 0x0800) ? '√' : ' ', (status & 0x8000) ? '√' : ' ', (glben & 0x0002) ? '√' : ' ');
	}
    }

    /* Prompt for sleep type. */
    printf("Enter hex sleep type to try: ");
    scanf("%x%*c", &type);

    /* Try sleep through alternate port. */
    printf("Press ENTER to try sleep %02X through register %04X...", type, acpi_base + 0xf0);
    gets(dummy_buf);
    outw(acpi_base + 0xf0, 0x2000 | (type << 10));

    /* Try sleep through main port. */
    printf("Nothing?\nPress ENTER to try sleep %02X through register %04X...", type, acpi_base + 0x04);
    gets(dummy_buf);
    outw(acpi_base + 0x04, 0x2000 | (type << 10));
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
		probe_via(dev, 3, dev_id);
		return 0;
	} else if ((ven_id == 0x1106) && (dev_id == 0x3050)) {
		printf("Found VT82C596 ACPI at device %02X function %d\n", dev, 3);
		probe_via(dev, 3, dev_id);
		return 0;
	} else {
		ven_id = pci_readw(0, dev, 4, 0);
		dev_id = pci_readw(0, dev, 4, 2);
		if ((ven_id == 0x1106) && (dev_id == 0x3057)) {
			printf("Found VT82C686 ACPI at device %02X function %d\n", dev, 4);
			probe_via(dev, 4, dev_id);
			return 0;
		}
	}
    }

    /* Nothing of interest found. */
    printf("No ACPI device of interest found!\n");
    return 1;
}
