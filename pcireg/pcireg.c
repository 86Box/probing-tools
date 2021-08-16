/*
 * 86Box	A hypervisor and IBM PC system emulator that specializes in
 *		running old operating systems and software designed for IBM
 *		PC systems and compatibles from 1981 through fairly recent
 *		system designs based on the PCI bus.
 *
 *		This file is part of the 86Box Probing Tools distribution.
 *
 *		PCI bus and configuration space probing tool.
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
#include <dos.h>
#include <graph.h>
#include <inttypes.h>
#include <malloc.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "wlib.h"


struct videoconfig vc;
union REGPACK rp; /* things break if this is not a global variable... */

#pragma pack(push, 0)
struct {
    uint32_t dev_db_off, cls_db_off, string_db_offset;
} pciids_header;
struct {
    uint16_t vendor;
    uint32_t dev_off;
    uint32_t string_offset;
} pciids_vendor;
struct {
    uint16_t device;
    uint32_t string_offset;
} pciids_device;
struct {
    uint16_t subclass;
    uint32_t string_offset;
} pciids_class;

typedef struct {
    uint8_t bus, dev;
    struct {
	uint8_t link;
	uint16_t bitmap;
    } ints[4];
    uint8_t slot, reserved;
} irq_routing_entry_t;
typedef struct {
    uint16_t len;
    union {
	void far *data_ptr;
	irq_routing_entry_t entry[1];
    };
} irq_routing_table_t;
#pragma pack(pop)


char *
read_string(FILE *f, uint32_t offset)
{
    uint8_t length, *buf;

    fseek(f, pciids_header.string_db_offset + offset, SEEK_SET);
    fread(&length, sizeof(length), 1, f);
    buf = malloc(length + 1);
    fread(buf, length, 1, f);
    buf[length] = '\0';
    return buf;
}


int
dump_regs(uint8_t bus, uint8_t dev, uint8_t func, uint8_t start_reg, char sz)
{
    int i, width, infobox, flags, bar_id;
    char buf[13];
    uint8_t cur_reg, regs[256], dev_type, bar_reg;
    multi_t reg_val;
    FILE *f;

    /* Align the starting register. */
    start_reg &= 0xfc;

    /* Generate dump file name. */
    sprintf(buf, "PCI%02X%02X%d.BIN", bus, dev, func);

    /* Size character '.' indicates a quiet dump for scan_bus. */
    if (sz != '.') {
	/* Print banner message. */
	printf("Dumping registers [%02X:FF] from PCI bus %02X device %02X function %d\n\n", start_reg,
	       bus, dev, func);

	/* Print column headers. */
	printf("   ");
	switch (sz) {
		case 'd':
		case 'l':
			width = 68; /* width for register + infobox display */
			for (i = 0x0; i <= 0xf; i += 4) {
				/* Add spacing at the halfway point. */
				if (i == 0x8)
					putchar(' ');
				printf("        %X", i);
			}
			break;

		case 'w':
			width = 72;
			for (i = 0x0; i <= 0xf; i += 2) {
				if (i == 0x8)
					putchar(' ');
				printf("    %X", i);
			}
			break;

		default:
			width = 80;
			for (i = 0x0; i <= 0xf; i++) {
				if (i == 0x8)
					putchar(' ');
				printf("  %X", i);
			}
			break;
	}

	/* Get terminal size. */
	_getvideoconfig(&vc);

	/* Print top of infobox if we're doing an infobox. */
	infobox = (start_reg < 0x3c) && (vc.numtextcols >= width);
	if (infobox) {
		printf("  ┌");
		for (i = 0; i < 24; i++)
			putchar('─');
		putchar('┐');
		if (vc.numtextcols > width)
			putchar('\n');
	} else {
		putchar('\n');
	}
    } else {
	/* Print dump file name now. */
	printf("Dumping registers to %s", buf);

	infobox = 0;
    }

    cur_reg = 0;
    do {
	/* Print row header. */
	if (sz != '.')
		printf("%02X:", cur_reg);

	/* Iterate through register dwords on this range. */
	do {
		/* Add spacing at the halfway point. */
		if ((sz != '.') && ((cur_reg & 0x0f) == 0x08))
			putchar(' ');

		/* Are we supposed to read this register? */
		if (cur_reg < start_reg) {
			/* No, read it as 0 for the dump. */
			reg_val.u32 = 0;

			/* Print dashes. */
			switch (sz) {
				case '.':
					break;

				case 'd':
				case 'l':
					printf(" --------");
					break;

				case 'w':
					printf(" ---- ----");
					break;

				default:
					printf(" -- -- -- --");
					break;
			}
		} else {
			/* Yes, read dword value. */
#ifdef DEBUG
			reg_val.u32 = pci_cf8(bus, dev, func, cur_reg);
#else
			reg_val.u32 = pci_readl(bus, dev, func, cur_reg);
#endif

			/* Print the value as bytes/words/dword. */
			switch (sz) {
				case '.':
					break;

				case 'd':
				case 'l':
					printf(" %04X%04X", reg_val.u16[1], reg_val.u16[0]);
					break;

				case 'w':
					printf(" %04X %04X", reg_val.u16[0], reg_val.u16[1]);
					break;

				default:
					printf(" %02X %02X %02X %02X", reg_val.u8[0], reg_val.u8[1], reg_val.u8[2], reg_val.u8[3]);
					break;
			}
		}

		/* Save value to dump array. */
		*((uint32_t *) &regs[cur_reg]) = reg_val.u32;

		/* Move on to the next dword. */
		cur_reg += 4;
	} while (cur_reg & 0x0f);

	/* Print infobox line if we're doing an infobox. */
	if (infobox) {
		/* Print left line, unless this is the bottom row. */
		if (cur_reg)
			printf("  │ ");

		/* Generate infobox lines, always checking if we have read the corresponding register(s). */
		switch (cur_reg) {
			case 0x10:
				/* Print class ID. */
				if (start_reg < 0x0c)
					printf("    Class: %02X Base    ", regs[0x0b]);
				else
					goto blank;
				break;

			case 0x20:
				/* Print subclass ID. */
				if (start_reg < 0x0c)
					printf("           %02X Sub     ", regs[0x0a]);
				else
					goto blank;
				break;

			case 0x30:
				/* Print programming interface ID. */
				if (start_reg < 0x0c)
					printf("           %02X ProgIntf", regs[0x09]);
				else
					goto blank;
				break;

			case 0x40:
				flags = (start_reg < 0x02) | ((start_reg < 0x2e) << 1);
				if (flags) {
					printf("Vendor ID: ");

					/* Print vendor ID. */
					if (flags & 1)
						printf("%04X", *((uint16_t *) &regs[0x00]));
					else
						printf("----");

					/* Print subvendor ID. */
					if (flags & 2)
						printf(" (%04X)", *((uint16_t *) &regs[0x2c]));
					else
						printf(" (----)");
				} else {
					goto blank;
				}
				break;

			case 0x50:
				flags = (start_reg < 0x04) | ((start_reg < 0x30) << 1);
				if (flags) {
					printf("Device ID: ");

					/* Print device ID. */
					if (flags & 1)
						printf("%04X", *((uint16_t *) &regs[0x02]));
					else
						printf("----");

					/* Print subdevice ID. */
					if (flags & 2)
						printf(" (%04X)", *((uint16_t *) &regs[0x2e]));
					else
						printf(" (----)");
				} else {
					goto blank;
				}
				break;

			case 0x60:
				/* Print revision. */
				if (start_reg < 0x09)
					printf(" Revision: %02X         ", regs[0x08]);
				else
					goto blank;
				break;

			case 0x70:
				/* No IRQ on bridges. */
				if (regs[0x0e] & 0x7f)
					goto blank;

				flags  =  regs[0x3c] && (regs[0x3c] != 0xff);
				flags |= (regs[0x3d] && (regs[0x3d] != 0xff)) << 1;
				if (flags) {
					printf("      IRQ: ");

					/* Print IRQ number. */
					if (flags & 1)
						printf("%2d", regs[0x3c] & 15);
					else
						printf("--");

					/* Print INTx# line. */
					if (flags & 2)
						printf(" (INT%c)  ", '@' + (regs[0x3d] & 15));
					else
						printf("         ");
				} else {
					goto blank;
				}
				break;

			case 0x80:
				/* Print separator for the BAR section. */
blank:
				printf("                      ");
				break;

			case 0x90:
			case 0xa0:
			case 0xb0:
			case 0xc0:
			case 0xd0:
			case 0xe0:
				/* We must know the device type before printing BARs. */
				if (start_reg > 0x0e)
					goto blank;

				/* Determine device type and current BAR register. */
				dev_type = regs[0x0e] & 0x7f;
				bar_reg = (cur_reg >> 2) - 0x14;

				/* Bridges have special BAR2+ handling. */
				if ((dev_type == 0x01) && (bar_reg > 0x14)) {
					/* Print bridge I/O and memory ranges. */
					switch (bar_reg) {
						case 0x18:
							if (start_reg < 0x1d)
								printf(" Brdg I/O: %X000-%XFFF  ", regs[0x1c] >> 4, regs[0x1d] >> 4);
							else
								goto blank;
							break;

						case 0x1c:
							if (start_reg < 0x21)
								printf(" Brdg Mem: %03X00000-  ", *((uint16_t *) &regs[0x20]) >> 4);
							else
								goto blank;
							break;

						case 0x20:
							if (start_reg < 0x21)
								printf("           %03XFFFFF   ", *((uint16_t *) &regs[0x22]) >> 4);
							else
								goto blank;
							break;

						default:
							goto blank;
					}
					break;
				} else if (dev_type > 0x01) {
					/* We don't parse CardBus bridges or other header types. */
					goto blank;
				}

				/* Print BAR0-5 for regular devices, or BAR0-1 for bridges. */
				reg_val.u32 = *((uint32_t *) &regs[bar_reg]);
				if ((start_reg <= bar_reg) && reg_val.u32 && (reg_val.u32 != 0xffffffff)) {
					bar_id = (bar_reg - 0x10) >> 2;
					if (reg_val.u8[0] & 0x01)
						printf("I/O BAR %d: %04X       ", bar_id, reg_val.u16[0] & 0xfffc);
					else
						printf("Mem BAR %d: %04X%04X   ", bar_id, reg_val.u16[1], reg_val.u16[0] & 0xfff0);
				} else {
					goto blank;
				}
				break;

			case 0xf0:
				/* Print expansion ROM BAR. */
				switch (regs[0x0e] & 0x7f) {
					case 0x00:
						if (start_reg < 0x31)
							reg_val.u32 = *((uint32_t *) &regs[0x30]);
						else
							reg_val.u32 = 0;
						break;

					case 0x01:
						if (start_reg < 0x39)
							reg_val.u32 = *((uint32_t *) &regs[0x38]);
						else
							reg_val.u32 = 0;
						break;

					default:
						reg_val.u32 = 0;
						break;
				}
				if (reg_val.u32 && (reg_val.u32 != 0xffffffff))
					printf("  Exp ROM: %04X%04X   ", reg_val.u16[1], reg_val.u16[0]);
				else
					goto blank;
				break;

			case 0x00:
				/* Print bottom of infobox. */
				printf("  └");
				for (i = 0; i < 24; i++)
					printf("─");
				printf("\xd9");
				if (vc.numtextcols > 80)
					printf("\n");
				break;
		}

		/* Print right line, unless this is the bottom row. */
		if (cur_reg)
			printf(" │");

		/* Move on to the next line if the terminal didn't already do that for us. */
		if (vc.numtextcols > width)
			printf("\n");
	} else if (sz != '.') {
		/* Move on to the next line. */
		printf("\n");
	}
    } while (cur_reg);

    /* Print dump file name. */
    if (sz != '.')
	printf("\nSaving dump to %s\n", buf);

    /* Write dump file. */
    f = fopen(buf, "wb");
    if (!f) {
	if (sz != '.')
		printf("File creation failed\n");
	return 1;
    }
    if (fwrite(regs, sizeof(regs), 1, f) < 1) {
	fclose(f);
	if (sz != '.')
		printf("File write failed\n");
	return 1;
    }
    fclose(f);

    if (sz == '.') {
	/* Clear the dump file name printed earlier. */
	width = strlen(buf) + 21;
	for (i = 0; i < width; i++)
		putchar('\b');
	for (i = 0; i < width; i++)
		putchar(' ');
	for (i = 0; i < width; i++)
		putchar('\b');
    }

    return 0;
}


int
scan_bus(uint8_t bus, int nesting, char dump, FILE *f, char *buf)
{
    int i, j, count, last_count, children;
    char *temp;
    uint8_t dev, func, header_type, new_bus, row, column;
    multi_t dev_id, dev_rev_class;

    /* Iterate through devices. */
    count = 0;
    for (dev = 0; dev <= 31; dev++) {
	/* Iterate through functions. */
	for (func = 0; func <= 7; func++) {
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

		/* Report a valid ID. */
		if (dev_id.u32 && (dev_id.u32 != 0xffffffff)) {
			/* Clear vendor/device name buffer while adding nested bus spacing if required. */
			if (nesting) {
				j = (nesting << 1) - 1;
				memset(buf, ' ', j);
				buf[j - 1] = '└';
				buf[j] = '─';
				buf[j + 1] = '\0';
			} else {
				buf[0] = '\0';
			}

			/* Print device address and IDs. */
			printf(" %02X  %02X  %d  [%04X:%04X] ", bus, dev, func,
			       dev_id.u16[0], dev_id.u16[1]);

			/* Read revision and class ID. */
#ifdef DEBUG
			dev_rev_class.u16[0] = rand();
			dev_rev_class.u16[1] = rand();
#else
			dev_rev_class.u32 = pci_readl(bus, dev, func, 0x08);
#endif

			/* Look up vendor name in the database file. */
			if (f) {
				fseek(f, sizeof(pciids_header), SEEK_SET);
				do {
					if (fread(&pciids_vendor, sizeof(pciids_vendor), 1, f) < 1)
						break;
				} while (pciids_vendor.vendor < dev_id.u16[0]);
				if (pciids_vendor.vendor == dev_id.u16[0]) {
					/* Add vendor name to buffer. */
					temp = read_string(f, pciids_vendor.string_offset);
					strcat(buf, temp);
					strcat(buf, " ");
					free(temp);

					/* Look up device name. */
					fseek(f, pciids_header.dev_db_off + pciids_vendor.dev_off, SEEK_SET);
					do {
						if (fread(&pciids_device, sizeof(pciids_device), 1, f) < 1)
							break;
					} while (pciids_device.device < dev_id.u16[1]);
					if (pciids_device.device == dev_id.u16[1]) {
						/* Add device name to buffer. */
						temp = read_string(f, pciids_device.string_offset);
						strcat(buf, temp);
						free(temp);
					} else {
						/* Device name not found. */
						goto unknown;
					}
				} else {
					/* Vendor name not found. */
					strcat(buf, "[Unknown] ");
					goto unknown;
				}
			} else {
unknown:
				/* Look up class ID. */

				fseek(f, pciids_header.cls_db_off, SEEK_SET);
				do {
					if (fread(&pciids_class, sizeof(pciids_class), 1, f) < 1)
						break;
				} while (pciids_class.subclass < dev_rev_class.u16[1]);
				if (pciids_class.subclass == dev_rev_class.u16[1]) {
					/* Add class name to buffer. */
					strcat(buf, "[");
					temp = read_string(f, pciids_class.string_offset);
					strcat(buf, temp);
					strcat(buf, "]");
					free(temp);
				} else {
					/* Class name not found. */
					sprintf(&buf[strlen(buf)], "[Class %02X:%02X:%02X]", dev_rev_class.u8[3], dev_rev_class.u8[2], dev_rev_class.u8[1]);
				}
			}

			/* Limit buffer to screen width, then print it with the revision ID. */
			i = vc.numtextcols - strlen(buf) - 24;
			if (i >= 9) {
				sprintf(&buf[strlen(buf)], " (rev %02X)", dev_rev_class.u8[0]);
			} else {
				if (i >= 5)
					strcat(buf, " ");
				else if (i < 4)
					strcpy(&buf[vc.numtextcols - 32], "... ");
				sprintf(&buf[strlen(buf)], "(%02X)", dev_rev_class.u8[0]);
			}
			printf(buf);

			/* Move on to the next line if the terminal didn't already do that for us. */
			if (vc.numtextcols > (strlen(buf) + 24))
				putchar('\n');

			if (nesting && count) {
				/* Get the current cursor position. */
				rp.h.ah = 0x03;
				rp.h.bh = 0x00;
				intr(0x10, &rp);
				row = rp.h.dh;
				column = rp.h.dl;

				/* Rectify previous nesting indicator. */
				rp.h.ah = 0x02;
				rp.h.dl = 22 + (nesting << 1);
				rp.h.dh -= 2;
				if (children > rp.h.dh)
					children = rp.h.dh;
				while (children) {
					intr(0x10, &rp);
					putchar('│');
					children--;
					rp.h.dh--;
				}
				intr(0x10, &rp);
				putchar('├');

				/* Restore cursor position. */
				rp.h.ah = 0x02;
				rp.h.dh = row;
				rp.h.dl = column;
				intr(0x10, &rp);
			}

			/* Dump registers if requested. */
			if (dump)
				dump_regs(bus, dev, func, 0, '.');

			/* Increment device count. */
			count++;
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
			/* Read bus numbers. */
#ifdef DEBUG
			new_bus = bus + 1;
#else
			new_bus = pci_readb(bus, dev, func, 0x19);
#endif

			/* Scan the secondary bus. */
			children = scan_bus(new_bus, nesting + 1, dump, f, buf);
			count += children;
		} else {
			children = 0;
		}

		/* If we're at the first function, stop if this is not a multi-function device. */
		if ((func == 0) && !(header_type & 0x80))
			break;
	}
    }

    return count;
}


int
scan_buses(char dump)
{
    int i;
    char *buf;
    FILE *f;

    /* Initialize buffers. */
    buf = malloc(1024);
    memset(buf, 0, 1024);

    /* Initialize PCI ID database file. */
    f = fopen("PCIIDS.BIN", "rb");
    if (f && (fread(&pciids_header, sizeof(pciids_header), 1, f) < 1)) {
	fclose(f);
	f = NULL;
    }

    /* Get terminal size. */
    _getvideoconfig(&vc);

    /* Print header. */
    printf("Bus Dev Fun [Vend:Dev ] Device\n");
    for (i = 0; i < vc.numtextcols; i++)
	putchar('─');

    /* Scan the root bus. */
    scan_bus(0, 0, dump, f, buf);

    /* Clean up. */
    free(buf);
    if (f)
	fclose(f);

    return 0;
}


int
read_reg(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg)
{
    multi_t reg_val;

    /* Print banner message. */
    printf("Reading from PCI bus %02X device %02X function %d registers [%02X:%02X]\n",
	   bus, dev, func, reg | 3, reg & 0xfc);

    /* Read dword value from register. */
#ifdef DEBUG
    reg_val.u32 = pci_cf8(bus, dev, func, reg);
#else
    reg_val.u32 = pci_readl(bus, dev, func, reg);
#endif

    /* Print value as a dword and bytes. */
    printf("Value: %04X%04X / %04X %04X / %02X %02X %02X %02X\n",
	   reg_val.u16[1], reg_val.u16[0],
	   reg_val.u16[0], reg_val.u16[1],
	   reg_val.u8[0], reg_val.u8[1], reg_val.u8[2], reg_val.u8[3]);

    return 0;
}


int
comp_ui8(const void *elem1, const void *elem2)
{
    uint8_t a = *((uint8_t *) elem1);
    uint8_t b = *((uint8_t *) elem2);
    return ((a < b) ? -1 : ((a > b) ? 1 : 0));
}


int
comp_irq_routing_entry(const void *elem1, const void *elem2)
{
    irq_routing_entry_t *a = (irq_routing_entry_t *) elem1;
    irq_routing_entry_t *b = (irq_routing_entry_t *) elem2;
    return comp_ui8(&a->dev, &b->dev);
}


int
show_steering_table(char mode)
{
    int i, j, entries;
    uint8_t irq_bitmap[256], temp[4];
    uint16_t buf_size = 1024, dev_class;
    irq_routing_table_t *table;
    irq_routing_entry_t *entry;

    /* Allocate buffer for PCI BIOS. */
retry_buf:
    table = malloc(buf_size);
    if (!table) {
	printf("Failed to allocate %d bytes.\n", buf_size);
	return 1;
    }

    /* Specify where the IRQ routing information will be placed. */
    table->len = buf_size;
    table->data_ptr = (void far *) table;

    /* Call PCI BIOS to fetch IRQ routing information. */
    rp.w.ax = 0xb10e;
    rp.w.bx = 0x0000;
    rp.w.es = FP_SEG(table->data_ptr);
    rp.w.di = FP_OFF(table->data_ptr);
    table->data_ptr = &table->entry[0];
    rp.w.ds = 0xf000;
    intr(0x1a, &rp);

    /* Check for any returned error. */
    if ((rp.h.ah == 0x59) && (table->len > buf_size)) {
	/* Re-allocate buffer with the requested size. */
	buf_size = table->len;
	printf("PCI BIOS claims %d bytes for table entries, ", buf_size);
	if (buf_size >= 65530) {
		printf("which looks invalid.\n");
		return 1;
	}
	printf("retrying...\n");
	buf_size += 2;
	free(table);
	goto retry_buf;
    } else if (rp.h.ah) {
	/* Something else went wrong. */
	printf("PCI BIOS call failed. (AH=%02X)\n", rp.h.ah);
	return 1;
    }

    /* Get terminal size. */
    _getvideoconfig(&vc);

    /* Start output according to the selected mode. */
    if (mode == '8') {
	/* Stop if no entries were found. */
	entries = table->len / sizeof(table->entry[0]);
	if (!entries) {
		printf("/* No entries found! */\n");
		return 1;
	}

	/* Sort table entries by device number for a tidier generated code. */
	entry = &table->entry[0];
	qsort(entry, entries, sizeof(table->entry[0]), comp_irq_routing_entry);

	/* Check for and add missing northbridge steering entries. */
	while (entries > 0) {
		/* Make sure we are on the first entry for the root bus. */
		if (entry->bus == 0)
			break;
		entries--;
		entry++;
	}
	if (entries > 0) {
		/* Assume device 00 is the northbridge if it has a host bridge class. */
		if (entry->dev > 0x00) {
			dev_class = pci_readw(0x00, 0x00, 0, 0x0a);
			if (dev_class == 0x0600)
				printf("pci_register_slot(0x00, PCI_CARD_NORTHBRIDGE, 1, 2, 3, 4);\n");
		}
		/* Assume device 01 is the AGP bridge if it has a PCI bridge class. */
		if (entry->dev > 0x01) {
			dev_class = pci_readw(0x00, 0x01, 0, 0x0a);
			if (dev_class == 0x0604)
				printf("pci_register_slot(0x01, PCI_CARD_AGPBRIDGE,   1, 2, 3, 4);\n");
		}
	}

	/* Identify INTx# link value mapping by gathering all known values. */
	memset(irq_bitmap, 0, sizeof(irq_bitmap));
	entry = &table->entry[0];
	entries = table->len / sizeof(table->entry[0]);
	while (entries-- > 0) {
		/* Ignore non-root buses. */
		if (!entry->bus) {
			/* Populate each IRQ link value. */
			for (j = 0; j < 4; j++)
				irq_bitmap[entry->ints[j].link] = entry->ints[j].link;
		}

		/* Move on to the next entry. */
		entry++;
	}

	/* Sort link value entries. */
	qsort(irq_bitmap, sizeof(irq_bitmap), sizeof(irq_bitmap[0]), comp_ui8);

	/* Jump to the first non-zero link value entry. */
	for (i = 0; (i < sizeof(irq_bitmap)) && (!irq_bitmap[i]); i++);

	/* Warn if the mapping might not be completely valid. */
	j = sizeof(irq_bitmap) - i;
	if (j % 4)
		printf("/* WARNING: %d IRQ link mappings found */\n", j);

	/* Copy link value entries to temporary array. */
	if (j > sizeof(temp))
		j = sizeof(temp);
	memcpy(temp, &irq_bitmap[i], j);

	/* Fill in mapping entries. */
	memset(irq_bitmap, 0, sizeof(irq_bitmap));
	for (i = 0; i < j; i++) {
		if (temp[i])
			irq_bitmap[temp[i]] = i + 1;
	}
    } else {
	/* Print header. */
	printf("┌ Location ─┐┌ INT Lines ┐┌ Steerable IRQs ───────── (INTA=1 B=2 C=4 D=8) ┐\n");
	printf("│Slt Bus Dev││A1 B2 C3 D4││ 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15│\n");
	printf("┴───────────┴┴───────────┴┴───────────────────────────────────────────────┴");
	for (i = 75; i < vc.numtextcols; i++)
		putchar('─');
    }

    /* Print entries until the end of the table. */
    entries = table->len / sizeof(table->entry[0]);
    entry = &table->entry[0];
    while (entries-- > 0) {
	/* Correct device number. */
	entry->dev >>= 3;

	/* Print entry according to the selected mode. */
	if (mode == '8') {
		/* Ignore non-root buses. */
		if (entry->bus)
			goto next_entry;

		/* Print device ID. */
		printf("pci_register_slot(0x%02X, PCI_CARD_", entry->dev);

		/* Determine and print slot type. */
		if (entry->dev == 0) {
			printf("NORTHBRIDGE,");
		} else {
			/* Read device class. */
			dev_class = pci_readw(0x00, 0x00, 0, 0x0a);

			/* Determine slot type by location and class. */
			if ((entry->dev == 1) && (dev_class == 0x0604)) {
				printf("AGPBRIDGE,  ");
			} else if (dev_class == 0x0601) {
				printf("SOUTHBRIDGE,");
			} else if (entry->slot == 0) {
				/* Very likely to be an onboard device. */
				if ((dev_class & 0xff00) == 0x0300)
					printf("VIDEO,      ");
				else if ((dev_class == 0x0401) || (dev_class == 0x0403))
					printf("SOUND,      ");
				else if (dev_class == 0x0100)
					printf("SCSI,       ");
				else if (dev_class == 0x0101)
					printf("IDE,        ");
				else
					goto normal;
			} else {
normal:
				printf("NORMAL,     ");
			}
		}

		/* Print INTx# IRQ routing entries. */
		for (i = 0; i < 4; i++) {
			if (i)
				putchar(',');
			putchar(' ');
			printf("%d", irq_bitmap[entry->ints[i].link]);
		}

		/* Finish line with a comment containing the slot number if present. */
		printf(");");
		if (entry->slot)
			printf(" /* Slot %02X */", entry->slot);
		else
			printf(" /* Onboard */");
	} else {
		/* Print slot, bus and device. */
		printf("  %02X  %02X  %02X ", entry->slot, entry->bus, entry->dev);

		/* Clear IRQ bitmap. */
		memset(irq_bitmap, 0, 16);

		/* Print INTx# line values, while populating the IRQ bitmap. */
		for (i = 0; i < (sizeof(entry->ints) / sizeof(entry->ints[0])); i++) {
			printf(" %02X", entry->ints[i].link);

			/* Set this INTx# line's bit on the bitmap entries for IRQs where it can be placed. */
			for (j = 0; j < 16; j++)
				irq_bitmap[j] |= ((entry->ints[i].bitmap >> j) & 1) << i;
		}

		/* Print IRQ bitmap. */
		putchar(' ');
		for (i = 0; i < 16; i++)
			printf("  %X", irq_bitmap[i]);
	}

	/* Move on to the next entry. */
	putchar('\n');
next_entry:
	entry++;
    }

    return 0;
}


int
write_reg(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg, char *val)
{
    uint16_t data_port;
    multi_t reg_val;

    /* Write a byte, word or dword depending on the input value's length. */
    data_port = 0xcfc;
    switch (strlen(val)) {
	case 1:
	case 2:
		/* Byte input value. */
		sscanf(val, "%" SCNx8, &reg_val.u8[0]);

		/* Print banner message. */
		printf("Writing %02X to PCI bus %02X device %02X function %d register [%02X]\n",
		       reg_val.u8[0],
		       bus, dev, func, reg);

		/* Write byte value to register. */
		pci_writeb(bus, dev, func, reg, reg_val.u8[0]);
		printf("Written!\n");

		/* Read the register's byte value back. */
#ifdef DEBUG
		reg_val.u32 = pci_cf8(bus, dev, func, reg);
		reg_val.u8[0] = reg_val.u8[reg & 3];
#else
		reg_val.u8[0] = pci_readb(bus, dev, func, reg);
#endif
		printf("Readback: %02X\n", reg_val.u8[0]);

		break;

	case 3:
	case 4:
		/* Word input value. */
		sscanf(val, "%" SCNx16, &reg_val.u16[0]);

		/* Print banner message. */
		printf("Writing %04X to PCI bus %02X device %02X function %d registers [%02X:%02X]\n",
		       reg_val.u16[0],
		       bus, dev, func, reg | 1, reg & 0xfe);

		/* Write word value to register. */
		pci_writew(bus, dev, func, reg, reg_val.u16[0]);
		printf("Written!\n");

		/* Read the register's word value back. */
#ifdef DEBUG
		reg_val.u32 = pci_cf8(bus, dev, func, reg);
		reg_val.u16[0] = reg_val.u16[(reg >> 1) & 1];
#else
		reg_val.u16[0] = pci_readw(bus, dev, func, reg);
#endif
		printf("Readback: %04X / %02X %02X\n",
		       reg_val.u16[0],
		       reg_val.u8[0], reg_val.u8[1]);

		break;

	default:
		/* Dword input value. */
		sscanf(val, "%" SCNx32, &reg_val.u32);

		/* Print banner message. */
		printf("Writing %04X%04X to PCI bus %02X device %02X function %d registers [%02X:%02X]\n",
		       reg_val.u16[1], reg_val.u16[0],
		       bus, dev, func, reg | 3, reg & 0xfc);

		/* Write dword value to register. */
		pci_writel(bus, dev, func, reg, reg_val.u32);
		printf("Written!\n");

		/* Read the register's dword value back. */
#ifdef DEBUG
		reg_val.u32 = pci_cf8(bus, dev, func, reg);
#else
		reg_val.u32 = pci_readl(bus, dev, func, reg);
#endif
		printf("Readback: %04X%04X / %04X %04X / %02X %02X %02X %02X\n",
		       reg_val.u16[1], reg_val.u16[0],
		       reg_val.u16[0], reg_val.u16[1],
		       reg_val.u8[0], reg_val.u8[1], reg_val.u8[2], reg_val.u8[3]);

		break;
    }

    return 0;
}


int
main(int argc, char **argv)
{
    int hexargc, i;
    char *ch, nonhex;
    uint8_t hexargv[8], bus, dev, func, reg;
    uint32_t cf8;

    /* Disable stdout buffering. */
    setbuf(stdout, NULL);

    /* Print usage if there are too few parameters or if the first one looks invalid. */
    if ((argc <= 1) || (strlen(argv[1]) < 2) || ((argv[1][0] != '-') && (argv[1][0] != '/'))) {
usage:
	printf("Usage:\n");
	printf("\n");
	printf("PCIREG -s [-d]\n");
	printf("∟ Display all devices on the PCI bus. Specify -d to dump registers as well.\n");
	printf("\n");
	printf("PCIREG -i [-8]\n");
	printf("∟ Display BIOS IRQ steering table. Specify -8 to display as 86Box code.\n");
	printf("\n");
	printf("PCIREG -r [bus] device [function] register\n");
	printf("∟ Read the specified register.\n");
	printf("\n");
	printf("PCIREG -w [bus] device [function] register value\n");
	printf("∟ Write byte, word or dword to the specified register.\n");
	printf("\n");
	printf("PCIREG {-d|-dw|-dl} [bus] device [function [register]]\n");
	printf("∟ Dump registers as bytes (-d), words (-dw) or dwords (-dl). Optionally\n");
	printf("  specify the register to start from (requires bus to be specified as well).\n");
	printf("\n");
	printf("All numeric parameters should be specified in hexadecimal (without 0x prefix).\n");
	printf("{bus device function register} can be substituted for a single port CF8h dword.\n");
	printf("Register dumps are saved to PCIbbddf.BIN where bb=bus, dd=device, f=function.\n");
	return 1;
    }

#ifndef DEBUG
    /* Test for PCI presence. */
    outl(0xcf8, 0x80000000);
    cf8 = inl(0xcf8);
    if (cf8 == 0xffffffff) {
	printf("Port CF8h is not responding. Does this system even have PCI?\n");
	return 1;
    }
#endif

    /* Convert the first parameter to lowercase. */
    ch = argv[1];
    while (*ch) {
	if ((*ch >= 'A') && (*ch <= 'Z'))
		*ch += 32;
	ch++;
    }

    /* Interpret parameters. */
    if (argv[1][1] == 's') {
	/* Bus scan only require a single optional parameter. */
	if ((argc >= 3) && (strlen(argv[2]) > 1))
		return scan_buses(argv[2][1]);
	else
		return scan_buses('\0');
    } else if (argv[1][1] == 'i') {
	/* Steering table display only requires a single optional parameter. */
	if ((argc >= 3) && (strlen(argv[2]) > 1))
		return show_steering_table(argv[2][1]);
	else
		return show_steering_table('\0');
    } else if ((argc >= 3) && (strlen(argv[1]) > 1)) {
	/* Read second parameter as a dword. */
	nonhex = 0;
	if (sscanf(argv[2], "%" SCNx32 "%c", &cf8, &nonhex) && !nonhex) {
		/* Initialize default bus/device/function/register values. */
		bus = dev = func = reg = 0;

		/* Determine if the second parameter's value is a port CF8h dword. */
		hexargc = 0;
		if (cf8 & 0xffffff00) {
			/* Break the CF8h dword's MSBs down into individual parameters. */
			hexargv[hexargc++] = (cf8 >> 16) & 0xff;
			hexargv[hexargc++] = (cf8 >> 11) & 31;
			hexargv[hexargc++] = (cf8 >> 8) & 7;
		}
		hexargv[hexargc++] = cf8;

		/* Read parameters until the end is reached or an invalid hex value is found. */
		for (i = 3; (i < argc) && (i < (sizeof(hexargv) - 1)); i++) {
			nonhex = 0;
			if (!sscanf(argv[i], "%" SCNx8 "%s", &hexargv[hexargc++], &nonhex) || nonhex)
				break;
		}
	} else {
		/* Print usage if the second parameter is an invalid hex value. */
		goto usage;
	}

	if (argv[1][1] == 'd') {
		/* Process parameters for a register dump. */
		switch (hexargc) {
			case 4:
				reg = hexargv[3];
				/* fall-through */

			case 3:
				func = hexargv[2];
				dev = hexargv[1];
				bus = hexargv[0];
				break;

			case 2:
				func = hexargv[1];
				/* fall-through */

			case 1:
				dev = hexargv[0];
				break;

			case -1:
				break;

			default:
				goto usage;
		}

		/* Start the operation. */
		return dump_regs(bus, dev, func, reg, argv[1][2]);
	} else {
		/* Subtract value parameter from a write operation. */
		if (argv[1][1] == 'w')
			hexargc -= 1;

		/* Process parameters for read/write operations. */
		switch (hexargc) {
			case 4:
				reg = hexargv[3];
				func = hexargv[2];
				dev = hexargv[1];
				bus = hexargv[0];
				break;

			case 3:
				reg = hexargv[2];
				/* fall-through */

			case 2:
				func = hexargv[1];
				/* fall-through */

			case 1:
				dev = hexargv[0];
				break;

			case -1:
				break;

			default:
				goto usage;
		}

		/* Start the operation. */
		switch (argv[1][1]) {
			case 'r':
				/* Start read. */
				return read_reg(bus, dev, func, reg);

			case 'w':
				/* Start write. */
				return write_reg(bus, dev, func, reg, argv[2 + hexargc]);

			default:
				/* Print usage if an unknown parameter was specified. */
				goto usage;
		}
	}
    } else {
	/* Print usage if a single parameter was specified. */
	goto usage;
    }

    return 1;
}
