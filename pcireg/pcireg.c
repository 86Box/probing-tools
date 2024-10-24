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
#    include <uefi.h>
#else
#    include <inttypes.h>
#    include <malloc.h>
#    include <stdio.h>
#    include <stdint.h>
#    include <string.h>
#    include <stdlib.h>
#    ifdef MSDOS
#        include <dos.h>
#        include <i86.h>
#    endif
#endif
#include "lh5_extract.h"
#include "clib_pci.h"
#include "clib_std.h"
#include "clib_sys.h"
#include "clib_term.h"

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
static const char *bridge_flags[] = {
    "Parity",
    "SErr",
    "ISA",
    "VGA",
    NULL,
    "MasterAbort",
    "SecReset",
    "FastB2B",
    "PriMTimeout",
    "SecMTimeout",
    "TimeoutStat",
    "TimeoutSErr",
    NULL,
    NULL,
    NULL,
    NULL
};

static int   term_width;
#if defined(MSDOS)
static union REGS   regs;
static struct SREGS seg_regs;
#    pragma pack(push, 1)
static struct PACKED {
    uint32_t edi, esi, ebp, unused, ebx, edx, ecx, eax;
    uint16_t flags, es, ds, fs, gs, ip, cs, sp, ss;
} dpmi_regs;
#    pragma pack(pop)
#endif

static int pciids_cur_vendor;
static int pciids_cur_device;
#pragma pack(push, 1)
static struct PACKED {
    uint16_t vendor_id;
    uint32_t devices_offset;
    uint32_t string_offset;
} *pciids_vendor = NULL;
static struct PACKED {
    uint16_t device_id;
    uint32_t subdevices_offset;
    uint32_t string_offset;
} *pciids_device = NULL;
static struct PACKED {
    uint16_t subvendor_id;
    uint16_t subdevice_id;
    uint32_t string_offset;
} *pciids_subdevice = NULL;
static struct PACKED {
    uint8_t  class_id;
    uint32_t string_offset;
} *pciids_class = NULL;
static struct PACKED {
    uint8_t  class_id;
    uint8_t  subclass_id;
    uint32_t string_offset;
} *pciids_subclass = NULL;
static struct PACKED {
    uint8_t  class_id;
    uint8_t  subclass_id;
    uint8_t  progif_id;
    uint32_t string_offset;
} *pciids_progif = NULL;
static char *pciids_string = NULL;

#if defined(MSDOS)
typedef struct {
    uint8_t bus, dev;
    struct {
        uint8_t  link;
        uint16_t bitmap;
    } ints[4];
    uint8_t slot, reserved;
} irq_routing_entry_t;
typedef struct {
    uint16_t len;
    union {
        struct {
            uint16_t offset, segment;
        } data_ptr;
        irq_routing_entry_t entry[1];
    };
} irq_routing_table_t;
#endif
#pragma pack(pop)

static int
pciids_open_database(void **ptr, char id)
{
    FILE          *f;
    size_t         pos;
    unsigned int   header_size;
    unsigned int   original_size;
    unsigned int   packed_size;
    uint8_t        header[128];
    char          *filename;
    char           target_filename[13];
    unsigned short crc;
    uint8_t       *buf = NULL;

    /* No action is required if the database is already loaded. */
    if (*ptr)
        return 0;
    fflush(stdout);

    /* Open archive, and stop if the open failed. */
    f = fopen("PCIIDS.LHA", "r" FOPEN_BINARY);
    if (!f)
        return 1;

    /* Generate target filename. */
    strcpy(target_filename, "PCIIDS_@.BIN");
    target_filename[7] = id;

    /* Go through archive. */
    while (!feof(f)) {
        /* Read LHA header. */
        pos = ftell(f);
        if (!fread(header, sizeof(header), 1, f))
            break;

        /* Parse LHA header. */
        header_size = LH5HeaderParse(header, sizeof(header), &original_size, &packed_size, &filename, &crc);
        if (!header_size)
            break; /* invalid header */

        /* Check filename. */
        crc = !strcmp(filename, target_filename);
        free(filename);
        if (crc) {
            /* Allocate buffers for the compressed and decompressed data. */
            *ptr = malloc(original_size);
            if (!*ptr)
                goto fail;
            buf = malloc(packed_size);
            if (!buf)
                goto fail;

            /* Read and decompress data. */
            fseek(f, pos + header_size, SEEK_SET);
            if (!fread(buf, packed_size, 1, f))
                goto fail;
            if (LH5Decode(buf, packed_size, *ptr, original_size) == -1)
                goto fail;

            /* All done, close archive. */
            fclose(f);
            free(buf);
            return 0;
        }

        /* Move on to the next header. */
        fseek(f, pos + header_size + packed_size, SEEK_SET);
    }

fail:
    /* Entry not found or read/decompression failed. */
    printf("PCI ID database %c decompression failed\n", id);
    fclose(f);
    if (*ptr) {
        free(*ptr);
        *ptr = NULL;
    }
    if (buf)
        free(buf);
    return 1;
}

static char *
pciids_read_string(uint32_t offset)
{
    /* Return nothing if the string offset is invalid. */
    if (offset == 0xffffffff)
        return NULL;

    /* Open database if required. */
    if (pciids_open_database((void **) &pciids_string, 'T'))
        return NULL;

    return &pciids_string[offset];
}

static int
find_vendor(uint16_t vendor_id)
{
    /* Open database if required. */
    if (pciids_open_database((void **) &pciids_vendor, 'V'))
        return 0;

    /* Go through vendor entries until the ID is matched or overtaken. */
    for (pciids_cur_vendor = 0; pciids_vendor[pciids_cur_vendor].vendor_id < vendor_id; pciids_cur_vendor++)
        ;

    /* Return 1 if the ID was matched, 0 otherwise. */
    return pciids_vendor[pciids_cur_vendor].vendor_id == vendor_id;
}

static char *
pciids_get_vendor(uint16_t vendor_id)
{
    /* Find vendor ID in the database, and return its name if found. */
    if (find_vendor(vendor_id))
        return pciids_read_string(pciids_vendor[pciids_cur_vendor].string_offset);

    /* Return nothing if the vendor ID was not found. */
    return NULL;
}

static char *
pciids_get_device(uint16_t device_id)
{
    /* Must be preceded by a call to {find|get}_vendor to establish the vendor ID! */

    /* Open database if required. */
    if (pciids_open_database((void **) &pciids_device, 'D'))
        return NULL;

    /* Go through device entries until the ID is matched or overtaken. */
    for (pciids_cur_device = pciids_vendor[pciids_cur_vendor].devices_offset; pciids_device[pciids_cur_device].device_id < device_id; pciids_cur_device++)
        ;

    /* Return the device name if found. */
    if (pciids_device[pciids_cur_device].device_id == device_id)
        return pciids_read_string(pciids_device[pciids_cur_device].string_offset);

    /* Return nothing if the device ID was not found. */
    return NULL;
}

static char *
pciids_get_subdevice(uint16_t subvendor_id, uint16_t subdevice_id)
{
    /* Must be preceded by calls to {find|get}_vendor and get_device to establish the vendor/device ID! */
    int i;

    /* Open database if required. */
    if (pciids_open_database((void **) &pciids_subdevice, 'S'))
        return NULL;

    /* Go through subdevice entries until the ID is matched or overtaken. */
    for (i = pciids_device[pciids_cur_device].subdevices_offset; (pciids_subdevice[i].subvendor_id < subvendor_id) || (pciids_subdevice[i].subdevice_id < subdevice_id); i++)
        ;

    /* Return the subdevice name if found. */
    if ((pciids_subdevice[i].subvendor_id == subvendor_id) && (pciids_subdevice[i].subdevice_id == subdevice_id))
        return pciids_read_string(pciids_subdevice[i].string_offset);

    /* Return nothing if the subdevice ID was not found. */
    return NULL;
}

static char *
pciids_get_class(uint8_t class_id)
{
    int i;

    /* Open database if required. */
    if (pciids_open_database((void **) &pciids_class, 'C'))
        return NULL;

    /* Go through class entries until the ID is matched or overtaken. */
    for (i = 0; pciids_class[i].class_id < class_id; i++)
        ;

    /* Return the class name if found. */
    if (pciids_class[i].class_id == class_id)
        return pciids_read_string(pciids_class[i].string_offset);

    /* Return nothing if the class ID was not found. */
    return NULL;
}

static char *
pciids_get_subclass(uint8_t class_id, uint8_t subclass_id)
{
    int i;

    /* Open database if required. */
    if (pciids_open_database((void **) &pciids_subclass, 'U'))
        return NULL;

    /* Go through subclass entries until the ID is matched or overtaken. */
    for (i = 0; (pciids_subclass[i].class_id < class_id) || (pciids_subclass[i].subclass_id < subclass_id); i++)
        ;

    /* Return the subclass name if found. */
    if ((pciids_subclass[i].class_id == class_id) && (pciids_subclass[i].subclass_id == subclass_id))
        return pciids_read_string(pciids_subclass[i].string_offset);

    /* Return nothing if the subclass ID was not found. */
    return NULL;
}

static char *
pciids_get_progif(uint8_t class_id, uint8_t subclass_id, uint8_t progif_id)
{
    int i;

    /* Open database if required. */
    if (pciids_open_database((void **) &pciids_progif, 'P'))
        return NULL;

    /* Go through programming interface entries until the ID is matched or overtaken. */
    for (i = 0; (pciids_progif[i].class_id < class_id) || (pciids_progif[i].subclass_id < subclass_id) || (pciids_progif[i].progif_id < progif_id); i++)
        ;

    /* Return the programming interface name if found. */
    if ((pciids_progif[i].class_id == class_id) && (pciids_progif[i].subclass_id == subclass_id) && (pciids_progif[i].progif_id == progif_id))
        return pciids_read_string(pciids_progif[i].string_offset);

    /* Return nothing if the programming interface ID was not found. */
    return NULL;
}

static int
dump_regs(uint8_t bus, uint8_t dev, uint8_t func, uint8_t start_reg, char sz)
{
    int     i, width, flags, bar_id;
    char    buf[13];
    uint8_t cur_reg, regs[256], dev_type, bar_reg;
    multi_t reg_val;
    FILE   *f;

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
    int     i, j, count, last_count, children;
    char   *temp;
    uint8_t dev, func, header_type, new_bus, row, column;
    multi_t dev_id, dev_rev_class;

    /* Iterate through devices. */
    count = 0;
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

                    /* Look up device name. */
                    temp = pciids_get_device(dev_id.u16[1]);
                    if (temp) {
                        /* Add device name to buffer. */
                        strcat(buf, temp);
                    } else {
                        /* Device name not found. */
                        goto unknown_device;
                    }
                } else {
                    /* Vendor name not found. */
                    strcat(buf, "[Unknown] ");

unknown_device: /* Look up class ID. */
                    temp = pciids_get_subclass(dev_rev_class.u8[3], dev_rev_class.u8[2]);
                    if (temp) {
                        /* Add class name to buffer. */
                        sprintf(&buf[strlen(buf)], "[%s]", temp);
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
                printf("%s", buf);

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
    int   i;
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
    char   *temp;
    int     i, j;
    uint8_t header_type, subsys_reg, num_bars, exprom_reg;
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
        printf("\nNo device appears to exist here. (vendor:device %04X:%04X)\n",
               reg_val.u16[0], reg_val.u16[1]);
        return 1;
    }
#endif

    /* Print vendor ID. */
    printf("\nVendor: [%04X] ", reg_val.u16[0]);

    /* Print vendor name if found. */
    temp = pciids_get_vendor(reg_val.u16[0]);
    printf("%s", temp ? temp : "[Unknown]");

    /* Print device ID. */
    printf("\nDevice: [%04X] ", reg_val.u16[1]);

    /* Print device name if found. */
    temp = pciids_get_device(reg_val.u16[1]);
    if (temp)
        printf("%s", temp ? temp : "[Unknown]");

    /* Read header type. We'll be using it a lot. */
    header_type = pci_readb(bus, dev, func, 0x0e);

    /* Determine the locations of common registers for this header type. */
    switch (header_type & 0x7f) {
        case 0x00: /* standard */
            subsys_reg = 0x2c;
            num_bars   = 6;
            exprom_reg = 0x30;
            break;

        case 0x01: /* PCI bridge */
            subsys_reg = 0xff;
            num_bars   = 2;
            exprom_reg = 0x38;
            break;

        case 0x02: /* CardBus bridge */
            subsys_reg = 0x40;
            num_bars   = 0;
            exprom_reg = 0xff;
            break;

        default: /* others */
            subsys_reg = 0xff;
            num_bars   = 0;
            exprom_reg = 0xff;
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
            printf("%s", temp ? temp : "[Unknown]");

            /* Print subdevice ID. */
            printf("\nSubdevice: [%04X] ", reg_val.u16[1]);

            /* Print subdevice ID if found. */
            temp = pciids_get_subdevice(reg_val.u16[0], reg_val.u16[1]);
            printf("%s", temp ? temp : "[Unknown]");
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

    /* Print bridge flags if this is a bridge. */
    if ((header_type & 0x7f) == 0x01) {
        printf("\n Bridge:");
        info_flags_helper(pci_readw(bus, dev, func, 0x3e), bridge_flags);
    }

    /* Read revision and class ID. */
    reg_val.u32 = pci_readl(bus, dev, func, 0x08);

    /* Print revision. */
    printf("\n\nRevision: %02X", reg_val.u8[0]);

    /* Print class ID. */
    printf("\nClass: [%02X] ", reg_val.u8[3]);

    /* Print class name if found. */
    temp = pciids_get_class(reg_val.u8[3]);
    printf("%s", temp ? temp : "[Unknown]");

    /* Print subclass ID. */
    printf("\n       [%02X] ", reg_val.u8[2]);

    /* Print subclass name if found. */
    temp = pciids_get_subclass(reg_val.u8[3], reg_val.u8[2]);
    printf("%s", temp ? temp : "[Unknown]");

    /* Print programming interface ID. */
    printf("\n       [%02X] ", reg_val.u8[1]);

    /* Print programming interface name if found. */
    temp = pciids_get_progif(reg_val.u8[3], reg_val.u8[2], reg_val.u8[1]);
    printf("%s", temp ? temp : "[Unknown]");

    /* Read latency, grant and interrupt line. */
    reg_val.u32 = pci_readl(bus, dev, func, 0x3c);

    /* Print interrupt if present. */
    if (reg_val.u16[0] && (reg_val.u8[0] != 0xff))
        printf("\nInterrupt: INT%c (IRQ %d)", '@' + (reg_val.u8[1] & 15), reg_val.u8[0] & 15);

    /* Print latency and grant if available. */
    if ((header_type & 0x7f) == 0x00) {
#ifdef FMT_FLOAT_SUPPORTED
        printf("\nMin Grant: %.0f us at 33 MHz", reg_val.u8[2] * 0.25);
        printf("\nMax Latency: %.0f us at 33 MHz", reg_val.u8[3] * 0.25);
#else
        printf("\nMin Grant: %d.%d us at 33 MHz", reg_val.u8[2] >> 2, 25 * (reg_val.u8[2] & 3));
        printf("\nMax Latency: %d.%d us at 33 MHz", reg_val.u8[3] >> 2, 25 * (reg_val.u8[2] & 3));
#endif
    }

    /* Read and print BARs. */
    j = 0;
    for (i = 0; i < num_bars; i++) {
        /* Read BAR. */
        reg_val.u32 = pci_readl(bus, dev, func, 0x10 + (i << 2));

        /* Move on to the next BAR if this one doesn't appear to be valid. */
        if (!reg_val.u32 || (reg_val.u32 == 0xffffffff))
            continue;

        /* Print BAR index. */
        if (!j) {
            putchar('\n');
            j = 1;
        }
        printf("\nBAR %d: ", i);

        /* Print BAR type, address and properties. */
        if (reg_val.u8[0] & 1) {
            printf("I/O at %04X", reg_val.u16[0] & 0xfffc);
        } else {
            printf("Memory at ");
            switch (reg_val.u32 & 0x00000006) {
                case 0x00:
                    printf("%08X (32-bit", reg_val.u32 & 0xfffffff0);
                    break;

                case 0x02:
                    printf("%05X (below 1 MB", reg_val.u32 & 0xfffffff0);
                    break;

                case 0x04:
                    /* Next BAR has the upper 32 bits. */
                    printf("%08X'%08X (64-bit", pci_readl(bus, dev, func, 0x14 + (i++ << 2)), reg_val.u32 & 0xfffffff0);
                    break;

                case 0x06:
                    printf("%08X (invalid location", reg_val.u32 & 0xfffffff0);
                    break;
            }
            printf(", ");
            if (!(reg_val.u8[0] & 0x08))
                printf("not ");
            printf("prefetchable)");
        }
    }

    if ((header_type & 0x7f) == 0x01) {
        /* Read and print PCI bridge specific registers. */
        putchar('\n');

        /* Read and print bus numbers. */
        reg_val.u32 = pci_readl(bus, dev, func, 0x18);
        printf("\nPCI bus: Primary[%02X] Secondary[%02X] Subordinate[%02X]", reg_val.u8[0], reg_val.u8[1], reg_val.u8[2]);

        /* Read and print I/O range. */
        reg_val.u16[0] = pci_readw(bus, dev, func, 0x1c);
        printf("\n    I/O: ");
        if (reg_val.u8[0] & 1)
            printf("%04X%04X-%04X%04X (32-bit)", pci_readw(bus, dev, func, 0x30), (reg_val.u8[0] & 0xf0) << 8, pci_readw(bus, dev, func, 0x32), reg_val.u16[0] | 0x0fff);
        else
            printf("%04X-%04X (16-bit)", (reg_val.u8[0] & 0xf0) << 8, reg_val.u16[0] | 0x0fff);

        /* Read and print MMIO memory range. */
        reg_val.u32 = pci_readl(bus, dev, func, 0x20);
        printf("\n Memory: %08X-%08X (32-bit, not prefetchable)\n         ", (reg_val.u32 & 0x0000fff0) << 16, reg_val.u32 | 0x000fffff);

        /* Read and print prefetchable memory range. */
        reg_val.u32 = pci_readl(bus, dev, func, 0x24);
        if (reg_val.u16[0] & 1)
            printf("%08X'%08X-%08X'%08X (64-bit", pci_readl(bus, dev, func, 0x28), (reg_val.u32 & 0x0000fff0) << 16, pci_readl(bus, dev, func, 0x2c), reg_val.u32 | 0x000fffff);
        else
            printf("%08X-%08X (32-bit", (reg_val.u32 & 0x0000fff0) << 16, reg_val.u32 | 0x000fffff);
        printf(", prefetchable)");
    }

    if (exprom_reg != 0xff) {
        /* Read and print expansion ROM. */
        reg_val.u32 = pci_readl(bus, dev, func, exprom_reg);
        if (reg_val.u32 && (reg_val.u32 != 0xffffffff))
            printf("\nExpansion ROM: %08X (%sabled)", reg_val.u32 & 0xfffffffe, (reg_val.u8[0] & 1) ? "en" : "dis");
    }

    printf("\n");

    return 0;
}

#if defined(MSDOS)
static int
comp_irq_routing_entry(const void *elem1, const void *elem2)
{
    irq_routing_entry_t *a = (irq_routing_entry_t *) elem1;
    irq_routing_entry_t *b = (irq_routing_entry_t *) elem2;
    return comp_ui8(&a->dev, &b->dev);
}

static int
comp_irq_routing_link(const void *elem1, const void *elem2)
{
    uint8_t a = *((uint8_t *) elem1);
    uint8_t b = *((uint8_t *) elem2);

    /* IRQ link values are highly chipset-dependent. For VIA and ALi chipsets,
       we can get away with rotating the values left by 1 to get an ascending
       order. Naturally, there are exceptions, but this should cover most cases. */

    /* Special case for ALi M1489 AMIBIOS 6, actual order unconfirmed. */
    if (a == 0x89)
        a = 0xc1; /* immediately after 0x41 */
    if (b == 0x89)
        b = 0xc1;

    /* Perform rotation. */
    a = (a << 1) | (a >> 7);
    b = (b << 1) | (b >> 7);

    return comp_ui8(&a, &b);
}

static int
free_realmode(uint16_t segment)
{
    memset(&regs, 0, sizeof(regs));
    regs.w.ax = 0x0101;
    regs.w.dx = segment;
    int386(0x31, &regs, &regs);
    return !regs.w.cflag;
}

#define STEERING_TABLE_PIR   1
#define STEERING_TABLE_86BOX 2

static int
dump_steering_table(uint8_t mode)
{
    int                      i, j, entries;
    uint8_t                  irq_bitmap[256], temp[4];
    uint16_t                 buf_size, table_segment, dev_class;
    irq_routing_table_t far *far_table;
    irq_routing_table_t     *table;
    irq_routing_entry_t     *entry;

    if (mode & STEERING_TABLE_PIR) {
        /* Search for PIR table. */
        uint32_t *p = (uint32_t *) 0xf0000;
        for (; (int) p <= 0xfffff; p += 4) { /* every 16 bytes */
            if (*p == ('$' | ('P' << 8) | ('I' << 16) | ('R' << 24)))
                break;
        }
        if ((int) p > 0xfffff) {
            printf("$PIR table not found in BIOS segment.\n");
retry_pir:
            printf("Try again without -m\n");
            return 1;
        }

        /* Move data to a near buffer while converting to the PCI BIOS format. */
        i = (((uint16_t *) p)[3] - 32) >> 4; /* byte 6 */
        buf_size = sizeof(irq_routing_table_t) + (sizeof(irq_routing_entry_t) * (i - 1)); /* subtract the single entry in the base struct */
        table = malloc(buf_size);
        if (!table) {
            printf("Failed to allocate %d local bytes.\n", buf_size);
            goto retry_pir;
        }
        table->len = i * sizeof(irq_routing_entry_t);
        memcpy(&table->entry[0], &p[8], table->len); /* byte 32 */
    } else {
        /* Allocate real mode memory buffer for PCI BIOS. */
        buf_size = 1024;
retry_buf:
        memset(&regs, 0, sizeof(regs));
        regs.w.ax = 0x0100;
        regs.w.bx = (buf_size + 15) >> 4;
        int386(0x31, &regs, &regs);
        if (regs.w.cflag) {
            printf("Failed to allocate %d bytes. (AX=%04X)\n", (buf_size + 15) & ~0xf, regs.w.ax);
retry_pcibios:
            printf("Try again with -m\n");
            return 1;
        }
        table_segment = regs.w.ax;
        far_table     = (irq_routing_table_t far *) MK_FP(regs.w.dx, 0);

        /* Specify where the IRQ routing information will be placed. */
        far_table->len              = buf_size;
        far_table->data_ptr.segment = table_segment;
        far_table->data_ptr.offset  = 2; /* hardcoded! */

        /* Call PCI BIOS to fetch IRQ routing information. */
        memset(&dpmi_regs, 0, sizeof(dpmi_regs));
        dpmi_regs.eax = 0xb10e;
        dpmi_regs.ebx = 0x0000;
        dpmi_regs.es  = table_segment;
        dpmi_regs.edi = 0x0000;
        dpmi_regs.ds  = 0xf000;
        segread(&seg_regs);
        regs.w.ax    = 0x0300;
        regs.w.bx    = 0x001a;
        regs.w.cx    = 0x0000;
        seg_regs.es  = FP_SEG(&dpmi_regs);
        regs.x.edi   = FP_OFF(&dpmi_regs);
        regs.w.cflag = 0;
        int386x(0x31, &regs, &regs, &seg_regs);
        if (regs.w.cflag) {
            printf("DPMI call failed. (AX=%04X)\n", regs.w.ax);
            goto retry_pcibios;
        }

        /* Check for any returned error. */
        i = (dpmi_regs.eax >> 8) & 0xff;
        if ((i == 0x59) && (far_table->len > buf_size)) {
            /* Re-allocate buffer with the requested size. */
            buf_size = far_table->len;
            printf("PCI BIOS claims %d bytes for table entries, ", buf_size);
            free_realmode(table_segment);
            if (buf_size >= 65530) {
                printf("which looks invalid.\n");
                goto retry_pcibios;
            }
            printf("retrying...\n");
            buf_size += 2;
            goto retry_buf;
        } else if (i) {
            /* Something else went wrong. */
            printf("PCI BIOS call failed. (AH=%02X)\n", i);
            free_realmode(table_segment);
            goto retry_pcibios;
        }

        /* Move data to a near buffer. */
        table = malloc(buf_size);
        if (!table) {
            printf("Failed to allocate %d local bytes.\n", buf_size);
            free_realmode(table_segment);
            goto retry_pcibios;
        }
        _fmemcpy(table, far_table, buf_size);
        free_realmode(table_segment);
    }

    /* Get terminal size. */
    term_width = term_get_size_x();

    /* Start output according to the selected mode. */
    if (mode & STEERING_TABLE_86BOX) {
        /* Stop if no entries were found. */
        entries = table->len / sizeof(table->entry[0]);
        if (!entries) {
            printf("/* No entries found! */\n");
            free(table);
            return 1;
        }

        /* Sort table entries by device number for a tidier generated code. */
        entry = &table->entry[0];
        qsort(entry, entries, sizeof(table->entry[0]), comp_irq_routing_entry);

        /* Check for and add missing northbridge steering entries. */
        i = 0;
        for (; entries > 0; entries--) {
            /* Make sure we are on the root bus. */
            if (entry->bus == 0) {
                if ((entry->dev >> 3) == 0x00)
                    i |= 1;
                else if ((entry->dev >> 3) == 0x01)
                    i |= 2;
            }
            entry++;
        }
        /* Assume device 00 is the northbridge if it has a host bridge class. */
        if (!(i & 1)) {
            dev_class = pci_readw(0x00, 0x00, 0, 0x0a);
            if (dev_class == 0x0600)
                printf("pci_register_slot(0x00, PCI_CARD_NORTHBRIDGE, 1, 2, 3, 4);\n");
        }
        /* Assume device 01 is the AGP bridge if it has a PCI bridge class. */
        if (!(i & 2)) {
            dev_class = pci_readw(0x00, 0x01, 0, 0x0a);
            if (dev_class == 0x0604)
                printf("pci_register_slot(0x01, PCI_CARD_AGPBRIDGE,   1, 2, 3, 4);\n");
        }

        /* Identify INTx# link value mapping by gathering all known values. */
        memset(irq_bitmap, 0, sizeof(irq_bitmap));
        entry   = &table->entry[0];
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
        qsort(irq_bitmap, sizeof(irq_bitmap), sizeof(irq_bitmap[0]), comp_irq_routing_link);

        /* Jump to the first non-zero link value entry. */
        for (i = 0; (i < sizeof(irq_bitmap)) && (!irq_bitmap[i]); i++)
            ;

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
    entry   = &table->entry[0];
    while (entries-- > 0) {
        /* Correct device number. */
        entry->dev >>= 3;

        /* Print entry according to the selected mode. */
        if (mode & STEERING_TABLE_86BOX) {
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
                dev_class = pci_readw(0x00, entry->dev, 0, 0x0a);

                /* Determine slot type by location and class. */
                if ((entry->dev == 1) && (dev_class == 0x0604)) {
                    printf("AGPBRIDGE,  ");
                } else if (dev_class == 0x0601) { /* ISA bridge */
                    printf("SOUTHBRIDGE,");
                } else if (entry->slot == 0) {
                    /* Very likely to be an onboard device. */
                    if ((dev_class & 0xff00) == 0x0300)
                        printf("VIDEO,      ");
                    else if (dev_class == 0x0101)
                        printf("IDE,        ");
                    else if (dev_class == 0x0100)
                        printf("SCSI,       ");
                    else if ((dev_class == 0x0401) || (dev_class == 0x0403))
                        printf("SOUND,      ");
                    else if (dev_class == 0x0703)
                        printf("MODEM,      ");
                    else if ((dev_class & 0xff00) == 0x0200)
                        printf("NETWORK,    ");
                    else if (dev_class == 0x0700)
                        printf("UART,       ");
                    else if (dev_class == 0x0c03)
                        printf("USB,        ");
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

    free(table);
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
    multi_t  reg_val;

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
            reg_val.u32   = pci_cf8(bus, dev, func, reg);
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
            reg_val.u32    = pci_cf8(bus, dev, func, reg);
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
    int      hexargc, i;
    char    *ch;
    uint8_t  hexargv[8], bus, dev, func, reg;
    uint32_t cf8;

    /* Disable stdout buffering. */
    term_unbuffer_stdout();

    /* Print usage if there are too few parameters or if the first one looks invalid. */
    if ((argc <= 1) || (strlen(argv[1]) < 2) || ((argv[1][0] != '-') && (argv[1][0] != '/'))) {
        ch = strrchr(argv[0], '\\');
        if (!(ch++)) {
            ch = strrchr(argv[0], '/');
            if (!(ch++))
                ch = argv[0];
        }
usage:
        printf("%s -s [-d]\n", ch);
        printf("∟ Display all devices on the PCI bus. Specify -d to dump registers as well.\n");
#if defined(MSDOS)
        printf("\n");
        printf("%s -t [-m] [-8]\n", ch);
        printf("∟ Display BIOS IRQ steering table. Specify -m to look for a Microsoft $PIR\n");
        printf("  table instead of calling PCI BIOS. Specify -8 to display as 86Box code.\n");
#endif
        printf("\n");
        printf("%s -i [bus] device [function]\n", ch);
        printf("∟ Show information about the specified device.\n");
        printf("\n");
        printf("%s -r [bus] device [function] register\n", ch);
        printf("∟ Read the specified register.\n");
        printf("\n");
        printf("%s -w [bus] device [function] register value\n", ch);
        printf("∟ Write byte, word or dword to the specified register.\n");
        printf("\n");
        printf("%s {-d|-dw|-dl} [bus] device [function [register]]\n", ch);
        printf("∟ Dump registers as bytes (-d), words (-dw) or dwords (-dl). Optionally\n");
        printf("  specify the register to start from (requires bus to be specified as well).\n");
        printf("\n");
        printf("All numeric parameters should be specified in hexadecimal (without 0x prefix).\n");
        printf("{bus device function register} can be substituted for a single port CF8h dword.\n");
        printf("Register dumps are saved to PCIbbddf.BIN where bb=bus, dd=device, f=function.");
        term_final_linebreak();
        return 1;
    }

#ifdef DEBUG
    pci_mechanism = 1;
#else
    /* Initialize PCI, and exit in case of failure. */
    if (!pci_init())
        return 1;
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
        /* Bus scan only asks for a single optional parameter. */
        if ((argc >= 3) && (strlen(argv[2]) > 1))
            return scan_buses(argv[2][1]);
        else
            return scan_buses('\0');
    }
#if defined(MSDOS)
    else if (argv[1][1] == 't') {
        /* Steering table display asks for optional parameters. */
        reg = 0;
        for (i = 2; i < argc; i++) {
            if (argv[i][0] == '\0')
                continue;
            else if (argv[i][1] == '8')
                reg |= STEERING_TABLE_86BOX;
            else if (argv[i][1] == 'm')
                reg |= STEERING_TABLE_PIR;
        }
        return dump_steering_table(reg);
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
                    dev  = hexargv[1];
                    bus  = hexargv[0];
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
                    reg  = hexargv[3];
                    func = hexargv[2];
                    dev  = hexargv[1];
                    bus  = hexargv[0];
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
