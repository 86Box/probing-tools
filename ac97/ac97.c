/*
 * 86Box	A hypervisor and IBM PC system emulator that specializes in
 *		running old operating systems and software designed for IBM
 *		PC systems and compatibles from 1981 through fairly recent
 *		system designs based on the PCI bus.
 *
 *		This file is part of the 86Box Probing Tools distribution.
 *
 *		AC'97 audio probing tool.
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
#include <string.h>
#include "clib.h"


uint8_t first = 1, silent = 0, bus = 0, dev, func;
uint16_t i, this_ven_id, this_dev_id, io_base, alt_io_base;


static uint16_t
find_io_range(uint16_t size)
{
    uint16_t base;

    for (base = 0x1000; base >= 0x1000; base += size) {
	/* Test first and last words only, as poking through the entire space
	   can lead to trouble (VIA ACPI has magic reads that hang the CPU). */
	if ((inw(base) == 0xffff) && (inw(base + size - 2) == 0xffff))
		return base;
    }

    return 0;
}


static uint16_t
get_io_bar(uint8_t reg, uint16_t size)
{
    uint16_t ret, temp;

    printf(" I/O BAR is ");

    /* Read BAR register. */
    ret = pci_readw(bus, dev, func, reg);
    if (!(ret & 0x0001) || (ret == 0xffff)) {
	temp = pci_readw(bus, dev, func, reg | 0x2);
	printf("invalid! (%04X%04X)", temp, ret);
	return 0;
    }

    /* Assign BAR if unassigned. */
    ret &= ~(size - 1);
    if (ret) {
	printf("assigned to %04X", ret);
    } else {
	printf("unassigned ");

	/* Find I/O range for the BAR. */
	ret = find_io_range(size);
	if (ret) {
		/* Assign and check value. */
		pci_writew(bus, dev, func, reg, ret | 0x0001);
		temp = pci_readw(bus, dev, func, reg);
		if ((temp & ~(size - 1)) == ret) {
			printf("(assigning to %04X)", ret);
		} else {
			ret = pci_readw(bus, dev, func, reg | 0x2);
			printf("and not responding! (%04X%04X)", ret, temp);
			return 0;
		}
	} else {
		printf("and no suitable range was found!");
		return 0;
	}
    }

    return ret;
}


static void
codec_probe(uint16_t (*codec_read)(uint8_t reg),
	    void (*codec_write)(uint8_t reg, uint16_t val))
{
    uint8_t cur_reg = 0;
    uint16_t regs[64], *regp = regs;
    char buf[13];
    FILE *f;

    /* Reset codec. */
    if (!silent) printf("\nResetting codec...");
    codec_write(0x00, 0xffff);
    if (!silent) printf(" done.\n");

    /* Set all optional registers/bits. */
    if (!silent) printf("Setting optional bits [Master");
    codec_write(0x02, 0xbf3f);
    if (!silent) printf(" AuxOut");
    codec_write(0x04, 0xbf3f);
    if (!silent) printf(" MonoOut");
    codec_write(0x06, 0x803f);
    if (!silent) printf(" PCBeep");
    codec_write(0x0a, 0x9ffe);
    if (!silent) printf(" Phone");
    codec_write(0x0c, 0x801f);
    if (!silent) printf(" Video");
    codec_write(0x14, 0x9f1f);
    if (!silent) printf(" AuxIn");
    codec_write(0x16, 0x9f1f);
    if (!silent) printf(" GP");
    codec_write(0x20, 0xf380);
    if (!silent) printf(" 3D");
    codec_write(0x22, 0xffff);
    if (!silent) printf(" EAID");
    codec_write(0x28, codec_read(0x28) | 0x0030);
    if (!silent) printf(" C/LFE");
    codec_write(0x36, 0xbfbf);
    if (!silent) printf(" Surround");
    codec_write(0x38, 0xbfbf);

    /* Dump registers. */
    if (!silent) {
	printf("] then dumping registers:\n   ");
	for (i = 0x0; i <= 0xf; i += 2) {
		if (i == 0x8)
			putchar(' ');
		printf("    %X", i);
	}
	putchar('\n');
    }

    do {
	/* Print row header. */
	if (!silent) printf("%02X:", cur_reg);

	/* Iterate through register words on this range. */
	do {
		/* Add spacing at the halfway point. */
		if (!silent && ((cur_reg & 0x0f) == 0x08))
			putchar(' ');

		/* Read and print the word value. */
		*regp = codec_read(cur_reg);
		if (!silent) printf(" %04X", *regp++);

		cur_reg += 2;
	} while (cur_reg & 0x0f);

	/* Move on to the next line. */
	if (!silent) putchar('\n');
    } while (cur_reg < 0x80);

    /* Generate and print dump file name. */
    sprintf(buf, "COD%02X%02X%d.BIN", bus, dev, func);
    printf("Saving codec register dump to %s\n", buf);

    /* Write dump file. */
    f = fopen(buf, "wb");
    if (!f) {
	printf("File creation failed\n");
	return;
    }
    if (fwrite(regs, 1, sizeof(regs), f) < sizeof(regs)) {
	fclose(f);
	printf("File write failed\n");
	return;
    }
    fclose(f);
}


static void
audiopci_codec_wait(multi_t *regval)
{
    /* Wait for WIP to be cleared. */
    for (i = 1; i; i++) {
	regval->u32 = inl(io_base | 0x14);
	if (!(regval->u16[1] & 0x4000))
		break;
    }
    if (!i)
	printf("[!]");
}


static uint16_t
audiopci_codec_read(uint8_t reg)
{
    multi_t regval;

    /* Wait for WIP to be cleared. */
    audiopci_codec_wait(&regval);

    /* Set address and read flag. */
    regval.u8[2] = reg | 0x80;

    /* Perform read. */
    outl(io_base | 0x14, regval.u32);

    /* Wait for WIP to be cleared. */
    audiopci_codec_wait(&regval);

    /* Return value read by audiopci_codec_wait. */
    return regval.u16[0];
}


static void
audiopci_codec_write(uint8_t reg, uint16_t val)
{
    multi_t regval;

    /* Wait for WIP to be cleared. */
    audiopci_codec_wait(&regval);

    /* Set address and data. */
    regval.u8[2] = reg;
    regval.u16[0] = val;

    /* Perform write. */
    outl(io_base | 0x14, regval.u32);

    /* Wait for WIP to be cleared. */
    audiopci_codec_wait(&regval);
}


static void
audiopci_probe()
{
    uint8_t rev;

    /* Get revision. */
    rev = pci_readb(bus, dev, func, 0x08);

    /* Print controller information. */
    printf("Found AudioPCI %04X revision %02X at bus %02X device %02X function %d\n", this_dev_id, rev, bus, dev, func);
    printf("Subsystem ID is %04X:%04X\n", pci_readw(bus, dev, func, 0x2c), pci_readw(bus, dev, func, 0x2e));

    /* Get I/O BAR. */
    printf("Main");
    io_base = get_io_bar(0x10, 64);
    printf("\n");
    if (!io_base)
	return;

    /* Perform codec probe. */
    codec_probe(audiopci_codec_read, audiopci_codec_write);
}


static void
via_codec_wait(multi_t *regval, uint16_t additional)
{
    uint16_t mask = 0x0100;

    /* Wait for Controller Busy to be cleared and additional bits to be set. */
    mask |= additional;
    for (i = 1; i; i++) {
	regval->u32 = inl(io_base | 0x80);
	if ((regval->u16[1] & mask) == additional)
		break;
    }
    if (!i)
	printf("[!]");
}


static uint16_t
via_codec_read(uint8_t reg)
{
    multi_t regval;

    /* Wait for Controller Busy to be cleared. */
    via_codec_wait(&regval, 0);

    /* Clear codec ID and Primary Valid. */
    regval.u8[3] = 0x02;

    /* Set address and read flag. */
    regval.u8[2] = reg | 0x80;

    /* Perform read. */
    outl(io_base | 0x80, regval.u32);

    /* Wait for Controller Busy to be cleared and Primary Valid to be set. */
    via_codec_wait(&regval, 0x0200);

    /* Return value read by via_codec_wait. */
    return regval.u16[0];
}


static void
via_codec_write(uint8_t reg, uint16_t val)
{
    multi_t regval;

    /* Wait for Controller Busy to be cleared. */
    via_codec_wait(&regval, 0);

    /* Clear codec ID and Primary Valid. */
    regval.u8[3] = 0x02;

    /* Set address and data. */
    regval.u8[2] = reg;
    regval.u16[0] = val;

    /* Perform write. */
    outl(io_base | 0x80, regval.u32);

    /* Wait for Controller Busy to be cleared. */
    via_codec_wait(&regval, 0);
}


static void
via_probe()
{
    uint8_t rev, base_rev, is8233;

    /* Print controller information. */
    rev = pci_readb(bus, dev, func, 0x08);
    base_rev = pci_readb(bus, dev, 0, 0x08);
    printf("Found VIA %04X revision %02X (base %02X) at bus %02X device %02X function %d\n", this_dev_id, rev, base_rev, bus, dev, func);

    /* Get SGD I/O BAR. */
    printf("SGD");
    io_base = get_io_bar(0x10, 256);
    printf("\n");
    if (!io_base)
	return;

    /* Set up AC-Link interface. */
    printf("Waking codec up... ");
    pci_writeb(bus, dev, func, 0x41, is8233 ? 0xc0 : 0xc4);
    for (i = 1; i; i++) {
	if (pci_readb(bus, dev, func, 0x40) & 0x01)
		break;
    }
    if (!i) {
	printf("timed out!\n");
	return;
    }
    printf("done.\n");

    /* Test Codec Shadow I/O BAR on 686. */
    if (this_dev_id == 0x3058) {
	printf("Codec Shadow");
	alt_io_base = get_io_bar(0x1c, 256);

	/* Perform tests if the BAR is set. */
	if (alt_io_base) {
		/* Reset codec. */
		via_codec_write(0x00, 0xffff);

		/* Test shadow behavior. */
		printf(", testing:\n");

		i = via_codec_read(0x02);
		printf("Read[%04X %04X]", i, inw(alt_io_base | 0x02));

		via_codec_write(0x02, 0xffff);
		printf("  Write[%04X %04X]", 0xffff, inw(alt_io_base | 0x02));

		via_codec_write(0x02, 0xffff);
		i = via_codec_read(0x02);
		printf("  WriteRead[%04X %04X]", i, inw(alt_io_base | 0x02));

		outw(alt_io_base | 0x02, 0x0000);
		i = inw(alt_io_base | 0x02);
		printf("  DirectWrite[%04X %04X]", i, via_codec_read(0x02));
	}

	printf("\n");
    }

    /* Perform codec probe. */
    codec_probe(via_codec_read, via_codec_write);
}


static void
intel_codec_wait()
{
    /* Wait for CAS to be cleared. */
    for (i = 1; i; i++) {
	if (!(inb(alt_io_base | 0x34) & 0x01))
		break;
    }
    if (!i)
	printf("[!]");
}


static uint16_t
intel_codec_read(uint8_t reg)
{
    /* Wait for CAS to be cleared. */
    intel_codec_wait();

    /* Perform read. */
    return inw(io_base | reg);
}


static void
intel_codec_write(uint8_t reg, uint16_t val)
{
    /* Wait for CAS to be cleared. */
    intel_codec_wait();

    /* Perform write. */
    outw(io_base | reg, val);
}


static void
intel_probe()
{
    uint8_t rev;

    /* Print controller information. */
    rev = pci_readb(bus, dev, func, 0x08);
    printf("Found Intel %04X revision %02X at bus %02X device %02X function %d\n", this_dev_id, rev, bus, dev, func);

    /* Get Mixer I/O BAR. */
    printf("Mixer");
    io_base = get_io_bar(0x10, 256);
    printf("\n");
    if (!io_base)
	return;

    /* Get Bus Master I/O BAR. */
    printf("Bus Master");
    alt_io_base = get_io_bar(0x14, 256);
    printf("\n");
    if (!alt_io_base)
	return;

    /* Set up AC-Link interface. */
    printf("Waking codec up... ");
    outb(alt_io_base | 0x2c, inb(alt_io_base | 0x2c) | 0x02);
    for (i = 0; i < 4096; i++) /* unknown delay required */
	outb(0xeb, 0x00);
    outb(alt_io_base | 0x2c, inb(alt_io_base | 0x2c) & ~0x02);
    for (i = 1; i; i++) {
	if (inb(alt_io_base | 0x31) & 0x01)
		break;
    }
    if (!i) {
	printf("timed out!\n");
	return;
    }
    printf("done.\n");

    /* Perform codec probe. */
    codec_probe(intel_codec_read, intel_codec_write);
}


static void
print_spacer()
{
    if (first)
	first = 0;
    else if (silent)
	printf("\n");
    else
	printf("\n\n");
}


static void
scan_bus()
{
    uint8_t header_type, saved_bus, saved_dev, saved_func;
    multi_t dev_id;

    /* Iterate through devices. */
    for (dev = 0; dev < pci_device_count; dev++) {
	/* Iterate through functions. */
	for (func = 0; func < 8; func++) {
		/* Read vendor/device ID. */
#ifdef DEBUG
		if ((bus < DEBUG) && (dev <= bus) && (func == 0)) {
			dev_id.u16[0] = rand();
			dev_id.u16[1] = rand();
		} else {
			dev_id.u32 = 0xffffffff;
		}
#else
		dev_id.u32 = pci_readl(bus, dev, func, 0x00);
#endif

		/* Probe a valid ID. */
		if (dev_id.u32 && (dev_id.u32 != 0xffffffff)) {
			this_ven_id = dev_id.u16[0];
			this_dev_id = dev_id.u16[1];
			if ((this_ven_id == 0x1274) && (this_dev_id != 0x5000)) {
				print_spacer();
				audiopci_probe();
			} else if ((this_ven_id == 0x1106) && ((this_dev_id == 0x3058) || (this_dev_id == 0x3059))) {
				print_spacer();
				via_probe();
			} else if ((this_ven_id == 0x8086) && ((this_dev_id == 0x2415) || (this_dev_id == 0x2425) || (this_dev_id == 0x2445) || (this_dev_id == 0x2485) || (this_dev_id == 0x24c5) || (this_dev_id == 0x24d5) || (this_dev_id == 0x25a6) || (this_dev_id == 0x266e) || (this_dev_id == 0x27de) || (this_dev_id == 0x7195))) {
				print_spacer();
				intel_probe();
			}
		} else {
			/* Stop or move on to the next function if there's nothing here. */
			if (func)
				continue;
			else
				break;
		}

		/* Read header type. */
#ifdef DEBUG
		header_type = (bus < (DEBUG - 1)) ? 0x01 : 0x00;
#else
		header_type = pci_readb(bus, dev, func, 0x0e);
#endif

		/* If this is a bridge, mark that we should probe its bus. */
		if (header_type & 0x7f) {
			/* Save current state. */
			saved_bus = bus;
			saved_dev = dev;
			saved_func = func;

			/* Read bus numbers. */
#ifdef DEBUG
			bus = bus + 1;
#else
			bus = pci_readb(bus, dev, func, 0x19);
#endif

			/* Scan the secondary bus. */
			scan_bus();

			/* Restore saved state. */
			bus = saved_bus;
			dev = saved_dev;
			func = saved_func;
		}

		/* If we're at the first function, stop if this is not a multi-function device. */
		if ((func == 0) && !(header_type & 0x80))
			break;
	}
    }
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

    /* Set silent mode. */
    if ((argc >= 1) && !strcmp(argv[1], "-s"))
	silent = 1;

    /* Scan PCI bus 0. */
    scan_bus();

    return 0;
}
