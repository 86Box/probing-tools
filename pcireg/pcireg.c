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
#ifdef __POSIX_UEFI__
# include <uefi.h>
#else
# include <inttypes.h>
# include <malloc.h>
# include <stdio.h>
# include <stdint.h>
# include <string.h>
# include <stdlib.h>
# ifdef __WATCOMC__
#  include <dos.h>
# endif
#endif
#include "clib.h"


static const char *command_flags[] = {
    "I/O",
    "Mem",
    "Master",
    "Special",
    "Invalidate",
    "Palette",
    "Parity",
    "Wait",
    "SErr",
    "FastBack",
    "DisableINTx",
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};
static const char *status_flags[] = {
    NULL,
    NULL,
    NULL,
    "IRQ",
    "CapList",
    "66MHz",
    "UDF",
    "FastBack",
    "ParityErr1",
    NULL,
    NULL,
    "TargetAbort",
    "TargetAbortAck",
    "MasterAbort",
    "SErr",
    "ParityErr2"
};
static const char *devsel[] = {
    "Fast",
    "Medium",
    "Slow",
    "Invalid"
};


static int term_width;
static FILE *pciids_f = NULL;
#if defined(__WATCOMC__) && !defined(M_I386)
static union REGPACK rp;
#endif

#pragma pack(push, 0)
static struct PACKED {
    uint32_t	device_db_offset,
		subdevice_db_offset,
		class_db_offset,
		subclass_db_offset,
		progif_db_offset,
		string_db_offset;
} pciids_header;
static struct PACKED {
    uint16_t	vendor_id;
    uint32_t	devices_offset,
		string_offset;
} pciids_vendor;
static struct PACKED {
    uint16_t	device_id;
    uint32_t	subdevices_offset,
		string_offset;
} pciids_device;
static struct PACKED {
    uint16_t	subvendor_id,
		subdevice_id;
    uint32_t	string_offset;
} pciids_subdevice;
static struct PACKED {
    uint8_t	class_id;
    uint32_t	string_offset;
} pciids_class;
static struct PACKED {
    uint8_t	class_id,
		subclass_id;
    uint32_t	string_offset;
} pciids_subclass;
static struct PACKED {
    uint8_t	class_id,
		subclass_id,
		progif_id;
    uint32_t	string_offset;
} pciids_progif;


# if defined(__WATCOMC__) && !defined(M_I386)
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
# endif
#pragma pack(pop)


static int
pciids_open_database()
{
    /* No action is required if the file is already open. */
    if (pciids_f)
	return 0;

    /* Open file, and stop if the open failed or the header couldn't be read. */
    pciids_f = fopen("PCIIDS.BIN", "rb");
    if (pciids_f && (fread(&pciids_header, sizeof(pciids_header), 1, pciids_f) < 1)) {
	fclose(pciids_f);
	pciids_f = NULL;
    }

    /* Return 0 if the file was opened successfully, 1 otherwise. */
    return !pciids_f;
}


static char *
pciids_read_string(uint32_t offset)
{
    uint8_t length, *buf;
    uint32_t sum;
    int i;

    /* Return nothing if the string offset is invalid. */
    if (offset == 0xffffffff)
	return NULL;

    /* Open database if required. */
    if (pciids_open_database())
	return NULL;

    /* Seek to string offset. */
    fseek_to(pciids_f, pciids_header.string_db_offset + offset);

    /* Read string length, and return nothing if it's an empty string. */
    fread(&length, sizeof(length), 1, pciids_f);
    if (length == 0)
	return NULL;

    /* Read string into buffer. */
    buf = malloc(length + 1);
    if (fread(buf, length, 1, pciids_f) < 1) {
	/* Clean up and return nothing if the read failed. */
	free(buf);
	return NULL;
    }
    buf[length] = '\0';

    return buf;
}


static int
pciids_find_vendor(uint16_t vendor_id)
{
    /* Open database if required. */
    if (pciids_open_database())
	return 0;

    /* Seek to vendor database. */
    fseek_to(pciids_f, sizeof(pciids_header));

    /* Read vendor entries until the ID is matched or overtaken. */
    do {
	if (fread(&pciids_vendor, sizeof(pciids_vendor), 1, pciids_f) < 1)
		break;
    } while (pciids_vendor.vendor_id < vendor_id);

    /* Return 1 if the ID was matched, 0 otherwise. */
    return pciids_vendor.vendor_id == vendor_id;
}


static char *
pciids_get_vendor(uint16_t vendor_id)
{
    /* Find vendor ID in the database, and return its name if found. */
    if (pciids_find_vendor(vendor_id))
	return pciids_read_string(pciids_vendor.string_offset);

    /* Return nothing if the vendor ID was not found. */
    return NULL;
}


static char *
pciids_get_device(uint16_t device_id)
{
    /* Must be preceded by a call to {find|get}_vendor to establish the vendor ID! */

    /* Open database if required. */
    if (pciids_open_database())
	return NULL;

    /* Seek to device database entries for the established vendor. */
    fseek_to(pciids_f, pciids_header.device_db_offset + pciids_vendor.devices_offset);

    /* Read device entries until the ID is matched or overtaken. */
    do {
	if (fread(&pciids_device, sizeof(pciids_device), 1, pciids_f) < 1)
		break;
    } while (pciids_device.device_id < device_id);

    /* Return the device name if found. */
    if (pciids_device.device_id == device_id)
	return pciids_read_string(pciids_device.string_offset);

    /* Return nothing if the device ID was not found. */
    return NULL;
}


static char *
pciids_get_subdevice(uint16_t subvendor_id, uint16_t subdevice_id)
{
    /* Must be preceded by calls to {find|get}_vendor and get_device to establish the vendor/device ID! */

    /* Open database if required. */
    if (pciids_open_database())
	return NULL;

    /* Seek to subdevice database entries for the established subvendor. */
    fseek_to(pciids_f, pciids_header.subdevice_db_offset + pciids_device.subdevices_offset);

    /* Read subdevice entries until the ID is matched or overtaken. */
    do {
	if (fread(&pciids_subdevice, sizeof(pciids_subdevice), 1, pciids_f) < 1)
		break;
    } while ((pciids_subdevice.subvendor_id < subvendor_id) || (pciids_subdevice.subdevice_id < subdevice_id));

    /* Return the subdevice name if found. */
    if ((pciids_subdevice.subvendor_id == subvendor_id) && (pciids_subdevice.subdevice_id == subdevice_id))
	return pciids_read_string(pciids_subdevice.string_offset);

    /* Return nothing if the subdevice ID was not found. */
    return NULL;
}


static char *
pciids_get_class(uint8_t class_id)
{
    /* Open database if required. */
    if (pciids_open_database())
	return NULL;

    /* Seek to class database. */
    fseek_to(pciids_f, pciids_header.class_db_offset);

    /* Read class entries until the ID is matched or overtaken. */
    do {
	if (fread(&pciids_class, sizeof(pciids_class), 1, pciids_f) < 1)
		break;
    } while (pciids_class.class_id < class_id);

    /* Return the class name if found. */
    if (pciids_class.class_id == class_id)
	return pciids_read_string(pciids_class.string_offset);

    /* Return nothing if the class ID was not found. */
    return NULL;
}


static char *
pciids_get_subclass(uint8_t class_id, uint8_t subclass_id)
{
    /* Open database if required. */
    if (pciids_open_database())
	return NULL;

    /* Seek to subclass database. */
    fseek_to(pciids_f, pciids_header.subclass_db_offset);

    /* Read subclass entries until the ID is matched or overtaken. */
    do {
	if (fread(&pciids_subclass, sizeof(pciids_subclass), 1, pciids_f) < 1)
		break;
    } while ((pciids_subclass.class_id < class_id) || (pciids_subclass.subclass_id < subclass_id));

    /* Return the subclass name if found. */
    if ((pciids_subclass.class_id == class_id) && (pciids_subclass.subclass_id == subclass_id))
	return pciids_read_string(pciids_subclass.string_offset);

    /* Return nothing if the subclass ID was not found. */
    return NULL;
}


static char *
pciids_get_progif(uint8_t class_id, uint8_t subclass_id, uint8_t progif_id)
{
    /* Open database if required. */
    if (pciids_open_database())
	return NULL;

    /* Seek to programming interface database. */
    fseek_to(pciids_f, pciids_header.progif_db_offset);

    /* Read programming interface entries until the ID is matched or overtaken. */
    do {
	if (fread(&pciids_progif, sizeof(pciids_progif), 1, pciids_f) < 1)
		break;
    } while ((pciids_progif.class_id < class_id) || (pciids_progif.subclass_id < subclass_id) || (pciids_progif.progif_id < progif_id));

    /* Return the programming interface name if found. */
    if ((pciids_progif.class_id == class_id) && (pciids_progif.subclass_id == subclass_id) && (pciids_progif.progif_id == progif_id))
	return pciids_read_string(pciids_progif.string_offset);

    /* Return nothing if the programming interface ID was not found. */
    return NULL;
}


static int
dump_regs(uint8_t bus, uint8_t dev, uint8_t func, uint8_t start_reg, char sz)
{
    int i, width, flags, bar_id;
    char buf[13];
    uint8_t cur_reg, regs[256], dev_type, bar_reg;
    multi_t reg_val;
    FILE *f;

    /* Align the starting register. */
    start_reg &= 0xfc;

    /* Generate dump file name. */
    sprintf(buf, "PCI%02X%02X%d.BIN", bus, dev, func);

    /* Get terminal size. */
    term_width = term_get_size_x();

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
			width = 40;
			for (i = 0x0; i <= 0xf; i += 4) {
				/* Add spacing at the halfway point. */
				if (i == 0x8)
					putchar(' ');
				printf("        %X", i);
			}
			break;

		case 'w':
			width = 44;
			for (i = 0x0; i <= 0xf; i += 2) {
				if (i == 0x8)
					putchar(' ');
				printf("    %X", i);
			}
			break;

		default:
			width = 52;
			for (i = 0x0; i <= 0xf; i++) {
				if (i == 0x8)
					putchar(' ');
				printf("  %X", i);
			}
			break;
	}

	/* Move on to the next line if the terminal didn't already do that for us. */
	if (width < term_width)
		putchar('\n');
    } else {
	/* Print dump file name now. */
	printf("Dumping registers to %s", buf);
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

	if (sz != '.') {
		/* Move on to the next line if the terminal didn't already do that for us. */
		if (width < term_width)
			putchar('\n');
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


static int
scan_bus(uint8_t bus, int nesting, char dump, char *buf)
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
				j = (nesting << 1) - 2;
				for (i = 0; i < j; i++)
					buf[i] = ' ';
				buf[i] = '\0';
				sprintf(&buf[strlen(buf)], "└─");
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

			/* Look up vendor name in the PCI ID database. */
			temp = pciids_get_vendor(dev_id.u16[0]);
			if (temp) {
				/* Add vendor name to buffer. */
				strcat(buf, temp);
				strcat(buf, " ");
				free(temp);

				/* Look up device name. */
				temp = pciids_get_device(dev_id.u16[1]);
				if (temp) {
					/* Add device name to buffer. */
					strcat(buf, temp);
					free(temp);
				} else {
					/* Device name not found. */
					goto unknown_device;
				}
			} else {
				/* Vendor name not found. */
				strcat(buf, "[Unknown] ");

unknown_device:			/* Look up class ID. */
				temp = pciids_get_subclass(dev_rev_class.u8[3], dev_rev_class.u8[2]);
				if (temp) {
					/* Add class name to buffer. */
					sprintf(&buf[strlen(buf)], "[%s]", temp);
					free(temp);
				} else {
					/* Class name not found. */
					sprintf(&buf[strlen(buf)], "[Class %02X:%02X:%02X]", dev_rev_class.u8[3], dev_rev_class.u8[2], dev_rev_class.u8[1]);
				}
			}

			/* Limit buffer to screen width, then print it with the revision ID. */
			i = term_width - strlen(buf) - 24;
			if (i >= 9) {
				sprintf(&buf[strlen(buf)], " (rev %02X)", dev_rev_class.u8[0]);
			} else {
				if (i >= 5)
					strcat(buf, " ");
				else if (i < 4)
					strcpy(&buf[term_width - 32], "... ");
				sprintf(&buf[strlen(buf)], "(%02X)", dev_rev_class.u8[0]);
			}
			printf(buf);

			/* Move on to the next line if the terminal didn't already do that for us. */
			if (term_width > (strlen(buf) + 24))
				putchar('\n');

			/* Rectify previous nesting indicator when nesting buses. */
			if (nesting && count && term_get_cursor_pos(&column, &row)) {
				i = 22 + (nesting << 1);
				j = row - 2;
				if (children > j)
					children = j;
				while (children) {
					term_set_cursor_pos(i, j);
					printf("│");
					children--;
					j--;
				}
				if (j != 0xff) {
					term_set_cursor_pos(i, j);
					printf("├");
				}

				/* Restore cursor position. */
				term_set_cursor_pos(column, row);
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
			children = scan_bus(new_bus, nesting + 1, dump, buf);
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


static int
scan_buses(char dump)
{
    int i;
    char *buf;

    /* Initialize buffers. */
    buf = malloc(1024);
    memset(buf, 0, 1024);

    /* Get terminal size. */
    term_width = term_get_size_x();

    /* Print header. */
    printf("Bus Dev Fun [VeID:DeID] Device\n");
    for (i = 0; i < term_width; i++)
	printf("─");

    /* Scan the root bus. */
    scan_bus(0, 0, dump, buf);

    /* Clean up. */
    free(buf);

    return 0;
}


static void
info_flags_helper(uint16_t bitfield, const char **table)
{
    /* Small helper function for printing PCI command/status flags. */
    int i, j;

    /* Check each bit. */
    j = 0;
    for (i = 0; i < 16; i++) {
	/* Ignore if there is no table entry. */
	if (!table[i])
		continue;

	/* Print flag name in [brackets] if set. */
	if (bitfield & (1 << i))
		printf(" [%s]", table[i]);
	else
		printf("  %s ", table[i]);

	/* Add line break and indentation after the 7th item. */
	if (++j == 7) {
		printf("\n        ");
		j = 0;
	}
    }
}


static int
dump_info(uint8_t bus, uint8_t dev, uint8_t func)
{
    char *temp;
    int i;
    uint8_t header_type, subsys_reg, num_bars;
    multi_t reg_val;

    /* Print banner message. */
    printf("Displaying information for PCI bus %02X device %02X function %d\n",
	   bus, dev, func);

    /* Read vendor/device ID, and stop if it's invalid. */
#ifdef DEBUG
    reg_val.u32 = 0x05711106;
#else
    reg_val.u32 = pci_readl(bus, dev, func, 0x00);
    if (!reg_val.u32 || (reg_val.u32 == 0xffffffff)) {
	printf("\nNo device appears to exist here. (vendor:device %04X:%04X)",
	reg_val.u16[0], reg_val.u16[1]);
	return 1;
    }
#endif

    /* Print vendor ID. */
    printf("\nVendor: [%04X] ", reg_val.u16[0]);

    /* Print vendor name if found. */
    temp = pciids_get_vendor(reg_val.u16[0]);
    if (temp) {
	printf(temp);
	free(temp);
    } else {
	printf("[Unknown]");
    }

    /* Print device ID. */
    printf("\nDevice: [%04X] ", reg_val.u16[1]);

    /* Print device name if found. */
    temp = pciids_get_device(reg_val.u16[1]);
    if (temp) {
	printf(temp);
	free(temp);
    } else {
	printf("[Unknown]");
    }

    /* Read header type. We'll be using it a lot. */
    header_type = pci_readb(bus, dev, func, 0x0e);

    /* Determine the locations of common registers for this header type. */
    switch (header_type & 0x7f) {
	case 0x00: /* standard */
		subsys_reg = 0x2c;
		num_bars = 6;
		break;

	case 0x02: /* CardBus bridge */
		subsys_reg = 0x40;
		num_bars = 0;
		break;

	case 0x03: /* PCI bridge and others */
		subsys_reg = 0xff;
		num_bars = 2;
		break;

	default: /* others */
		subsys_reg = 0xff;
		num_bars = 0;
		break;
    }
    if (subsys_reg != 0xff) {
	/* Read subsystem ID and print it if valid. */
	reg_val.u32 = pci_readl(bus, dev, func, subsys_reg);
	if (reg_val.u32 && (reg_val.u32 != 0xffffffff)) {
		/* Print subvendor ID. */
		printf("\nSubvendor: [%04X] ", reg_val.u16[0]);

		/* Print subvendor name if found. */
		temp = pciids_get_vendor(reg_val.u16[0]);
		if (temp) {
			printf(temp);
			free(temp);
		} else {
			printf("[Unknown]");
		}

		/* Print subdevice ID. */
		printf("\nSubdevice: [%04X] ", reg_val.u16[1]);

		/* Print subdevice ID if found. */
		temp = pciids_get_subdevice(reg_val.u16[0], reg_val.u16[1]);
		if (temp) {
			printf(temp);
			free(temp);
		} else {
			printf("[Unknown]");
		}
	}
    }

    /* Read command and status. */
    reg_val.u32 = pci_readl(bus, dev, func, 0x04);

    /* Print command and status flags. */
    printf("\n\nCommand:");
    info_flags_helper(reg_val.u16[0], command_flags);
    printf("\n Status:");
    info_flags_helper(reg_val.u16[1], status_flags);
    printf(" DEVSEL[%s]", devsel[(reg_val.u16[1] >> 9) & 3]);

    /* Read revision and class ID. */
    reg_val.u32 = pci_readl(bus, dev, func, 0x08);

    /* Print revision. */
    printf("\n\nRevision: %02X", reg_val.u8[0]);

    /* Print class ID. */
    printf("\nClass: [%02X] ", reg_val.u8[3]);

    /* Print class name if found. */
    temp = pciids_get_class(reg_val.u8[3]);
    if (temp) {
	printf(temp);
	free(temp);
    } else {
	printf("[Unknown]");
    }

    /* Print subclass ID. */
    printf("\n       [%02X] ", reg_val.u8[2]);

    /* Print subclass name if found. */
    temp = pciids_get_subclass(reg_val.u8[3], reg_val.u8[2]);
    if (temp) {
	printf(temp);
	free(temp);
    } else {
	printf("[Unknown]");
    }

    /* Print programming interface ID. */
    printf("\n       [%02X] ", reg_val.u8[1]);

    /* Print programming interface name if found. */
    temp = pciids_get_progif(reg_val.u8[3], reg_val.u8[2], reg_val.u8[1]);
    if (temp) {
	printf(temp);
	free(temp);
    } else {
	printf("[Unknown]");
    }

    /* Read latency, grant and interrupt line. */
    reg_val.u32 = pci_readl(bus, dev, func, 0x3c);

    /* Print interrupt if present. */
    if (reg_val.u16[0] && (reg_val.u16[0] != 0xffff))
	printf("\nInterrupt: INT%c (IRQ %d)", '@' + (reg_val.u8[1] & 15), reg_val.u8[0] & 15);

    /* Print latency and grant if available. */
    if ((header_type & 0x7f) == 0x00) {
	printf("\nMin Grant: %.0f us at 33 MHz", reg_val.u8[2] * 0.25);
	printf("\nMax Latency: %.0f us at 33 MHz", reg_val.u8[3] * 0.25);
    }

    /* Read and print BARs. */
    for (i = 0; i < num_bars; i++) {
	if (i == 0)
		putchar('\n');

	/* Read BAR. */
	reg_val.u32 = pci_readl(bus, dev, func, 0x10 + (i << 2));

	/* Move on to the next BAR if this one doesn't appear to be valid. */
	if (!reg_val.u32 || (reg_val.u32 == 0xffffffff))
		continue;

	/* Print BAR index. */
	printf("\nBAR %d: ", i);

	/* Print BAR type, address and properties. */
	if (reg_val.u8[0] & 1) {
		printf("I/O at %04X", reg_val.u16[0] & 0xfffc);
	} else {
		printf("Memory at %04X%04X (", reg_val.u16[1], reg_val.u16[0] & 0xfff0);
		switch (reg_val.u8[0] & 0x06) {
			case 0x00:
				printf("32-bit");
				break;

			case 0x02:
				printf("below 1 MB");
				break;

			case 0x04:
				printf("64-bit");
				break;

			case 0x06:
				printf("invalid location");
				break;
		}
		printf(", ");
		if (!(reg_val.u8[0] & 0x08))
			printf("not ");
		printf("prefetchable)");
	}
    }

    printf("\n");

    return 0;
}


#if defined(__WATCOMC__) && !defined(M_I386)
static int
comp_irq_routing_entry(const void *elem1, const void *elem2)
{
    irq_routing_entry_t *a = (irq_routing_entry_t *) elem1;
    irq_routing_entry_t *b = (irq_routing_entry_t *) elem2;
    return comp_ui8(&a->dev, &b->dev);
}


static int
dump_steering_table(char mode)
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
    term_width = term_get_size_x();

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
	for (i = 75; i < term_width; i++)
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
			printf(" %d", irq_bitmap[entry->ints[i].link]);
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
#endif


static int
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


static int
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
		parse_hex_u8(val, &reg_val.u8[0]);

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
		parse_hex_u16(val, &reg_val.u16[0]);

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
		parse_hex_u32(val, &reg_val.u32);

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
    char *ch;
    uint8_t hexargv[8], bus, dev, func, reg;
    uint32_t cf8;

    /* Disable stdout buffering. */
    term_unbuffer_stdout();

    /* Print usage if there are too few parameters or if the first one looks invalid. */
    if ((argc <= 1) || (strlen(argv[1]) < 2) || ((argv[1][0] != '-') && (argv[1][0] != '/'))) {
usage:
	printf("Usage:\n");
	printf("\n");
	printf("%s -s [-d]\n", argv[0]);
	printf("∟ Display all devices on the PCI bus. Specify -d to dump registers as well.\n");
#if defined(__WATCOMC__) && !defined(M_I386)
	printf("\n");
	printf("%s -t [-8]\n", argv[0]);
	printf("∟ Display BIOS IRQ steering table. Specify -8 to display as 86Box code.\n");
#endif
	printf("\n");
	printf("%s -i [bus] device [function]\n", argv[0]);
	printf("∟ Show information about the specified device.\n");
	printf("\n");
	printf("%s -r [bus] device [function] register\n", argv[0]);
	printf("∟ Read the specified register.\n");
	printf("\n");
	printf("%s -w [bus] device [function] register value\n", argv[0]);
	printf("∟ Write byte, word or dword to the specified register.\n");
	printf("\n");
	printf("%s {-d|-dw|-dl} [bus] device [function [register]]\n", argv[0]);
	printf("∟ Dump registers as bytes (-d), words (-dw) or dwords (-dl). Optionally\n");
	printf("  specify the register to start from (requires bus to be specified as well).\n");
	printf("\n");
	printf("All numeric parameters should be specified in hexadecimal (without 0x prefix).\n");
	printf("{bus device function register} can be substituted for a single port CF8h dword.\n");
	printf("Register dumps are saved to PCIbbddf.BIN where bb=bus, dd=device, f=function.");
	term_final_linebreak();
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
    }
#if defined(__WATCOMC__) && !defined(M_I386)
    else if (argv[1][1] == 't') {
	/* Steering table display only requires a single optional parameter. */
	if ((argc >= 3) && (strlen(argv[2]) > 1))
		return dump_steering_table(argv[2][1]);
	else
		return dump_steering_table('\0');
    }
#endif
    else if ((argc >= 3) && (strlen(argv[1]) > 1)) {
	/* Read second parameter as a dword. */
	if (parse_hex_u32(argv[2], &cf8)) {
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
			if (!parse_hex_u8(argv[i], &hexargv[hexargc++]))
				break;
		}
	} else {
		/* Print usage if the second parameter is an invalid hex value. */
		goto usage;
	}

	if ((argv[1][1] == 'd') || (argv[1][1] == 'i')) {
		/* Process parameters for a register or information dump. */
		switch (hexargc) {
			case 4:
				/* Specifying a register is not valid on an information dump. */
				if (argv[1][1] == 'i')
					goto usage;
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
		switch (argv[1][1]) {
			case 'd':
				/* Start register dump. */
				return dump_regs(bus, dev, func, reg, argv[1][2]);

			case 'i':
				/* Start information dump. */
				return dump_info(bus, dev, func);
		}
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
				printf("unknown hexargc %d\n", hexargc);
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
