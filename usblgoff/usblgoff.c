/*
 * 86Box	A hypervisor and IBM PC system emulator that specializes in
 *		running old operating systems and software designed for IBM
 *		PC systems and compatibles from 1981 through fairly recent
 *		system designs based on the PCI bus.
 *
 *		This file is part of the 86Box Probing Tools distribution.
 *
 *		USB legacy emulation disable tool.
 *
 *
 *
 * Authors:	RichardG, <richardg867@gmail.com>
 *
 *		Copyright 2022 RichardG.
 *
 * ┌──────────────────────────────────────────────────────────────┐
 * │ This file is UTF-8 encoded. If this text is surrounded by    │
 * │ garbage, please tell your editor to open this file as UTF-8. │
 * └──────────────────────────────────────────────────────────────┘
 */
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "clib.h"


static void
pci_scan_callback(uint8_t bus, uint8_t dev, uint8_t func,
		  uint16_t ven_id, uint16_t dev_id)
{
    uint8_t i;
    uint16_t j, start, end;
#ifdef IS_32BIT
    uint32_t mmio_base, *mmio;
#endif

#ifdef IS_32BIT
    /* Disable southbridge I/O traps. */
    if ((ven_id == 0x8086) && (((dev_id >= 0x2640) && (dev_id <= 0x2642)) /* ICH6 */ ||
			       ((dev_id >= 0x27b0) && (dev_id <= 0x27bd)) /* ICH7 */ ||
			       ((dev_id >= 0x2810) && (dev_id <= 0x2815)) /* ICH8 */ ||
			       ((dev_id >= 0x2912) && (dev_id <= 0x2919)) /* ICH9 */ ||
			       ((dev_id >= 0x3a14) && (dev_id <= 0x3a1a)) /* ICH10 */)) {
	printf("Found ICH6-10 LPC bridge %04X at bus %02X device %02X function %d\n", dev_id, bus, dev, func);

	/* Get RCBA. */
	printf("> RCBA is ");
	mmio_base = pci_readl(bus, dev, func, 0xf0);
	if ((mmio_base & 0x00000001) && (mmio_base != 0xffffffff)) {
		printf("assigned to %08X\n", mmio_base);
		mmio = (uint32_t *) (mmio_base & ~0x00003fff);

		/* Disable all relevant I/O traps. */
		for (i = 0; i < 4; i++) {
			/* Check if the trap is enabled. */
			j = (0x1e80 | (i << 3)) >> 2;
			if (mmio[j] & 0x00000001) {
				start = mmio[j] & 0xfffc;
				end = (mmio[j] >> 16) & 0xfc; /* temporarily just the mask */
				printf("> Trap %d (%04X+%02X)", i, start, end);
				end += start; /* now the actual end */

				/* Check if the trap covers the KBC ports. */
				if (((start <= 0x60) && (end >= 0x60)) ||
				    ((start <= 0x64) && (end >= 0x64))) {
					/* Clear TRSE bit. */
					mmio[j] &= ~0x00000001;

					/* Check if the bit was actually cleared. */
					if (mmio[j] & 0x00000001)
						printf("enable bit stuck! (%08X%08X)\n", mmio[j | 1], mmio[j]);
					else
						printf("disabled\n");
				} else {
					printf("not relevant\n");
				}
			}
		}
	} else {
		printf("unassigned! (%08X)\n", mmio_base);
	}

	return;
    }
#endif

    /* Skip non-USB devices. */
    if (pci_readw(bus, dev, func, 0x0a) != 0x0c03)
	return;

    /* Read progif code. */
    i = pci_readb(bus, dev, func, 0x09);

    /* Act according to the device class. */
    if (i == 0x00) { /* UHCI */
	printf("Found UHCI USB controller at bus %02X device %02X function %d\n", bus, dev, func);

	/* Clear 60/64h trap/SMI R/W bits. */
	j = pci_readw(bus, dev, func, 0xc0);
	pci_writew(bus, dev, func, 0xc0, j & ~0x000f);

	/* Check if the bits were actually cleared. */
	j = pci_readw(bus, dev, func, 0xc0);
	if (j & 0x000f)
		printf("> I/O trap bits stuck! (%04X)\n", j);
	else
		printf("> I/O traps disabled\n");
    } else if (i == 0x10) { /* OHCI */
	printf("Found OHCI USB controller at bus %02X device %02X function %d\n", bus, dev, func);

#ifdef IS_32BIT
	/* Get MMIO base address. */
	mmio_base = pci_get_mem_bar(bus, dev, func, 0x10, 4096, "> MMIO");
	if (mmio_base) {
		mmio = (uint32_t *) mmio_base;

		/* Clear EmulationEnable bit. */
		mmio[0x100 >> 2] &= ~0x00000001;

		/* Check if the bit was actually cleared. */
		if (mmio[0x100 >> 2] & 0x00000001)
			printf("> Emulation bit stuck! (%08X)\n", mmio[0x100 >> 2]);
		else
			printf("> Emulation disabled\n");
	}
#else
	printf("> OHCI not supported on 16-bit build!\n");
#endif
    } else if ((i != 0x20) && (i != 0x30)) { /* not EHCI or XHCI */
	printf("Found unknown USB controller type %02X at bus %02X device %02X function %d\n", i, bus, dev, func);
    }
}


int
main(int argc, char **argv)
{
    uint8_t dev, func;

    /* Disable stdout buffering. */
    term_unbuffer_stdout();

    /* Initialize PCI, and exit in case of failure. */
    if (!pci_init())
	return 1;

    /* Scan PCI bus 0. */
    pci_scan_bus(0, pci_scan_callback);

    return 0;
}
