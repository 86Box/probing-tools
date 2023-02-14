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
#include "clib_sys.h"
#include "clib_pci.h"
#include "clib_term.h"

uint8_t  first = 1, silent = 0, bus, dev, func;
uint16_t i, io_base, alt_io_base;

static void
codec_probe(uint16_t (*codec_read)(uint8_t reg),
            void (*codec_write)(uint8_t reg, uint16_t val))
{
    uint8_t  cur_reg = 0;
    uint16_t regs[64], *regp = regs;
    char     buf[13];
    FILE    *f;

    /* Reset codec. */
    if (!silent)
        printf("\nResetting codec...");
    codec_write(0x00, 0xffff);
    if (!silent)
        printf(" done.\n");

    /* Set all optional registers/bits. */
    if (!silent)
        printf("Setting optional bits [Master");
    codec_write(0x02, 0xbf3f);
    if (!silent)
        printf(" AuxOut");
    codec_write(0x04, 0xbf3f);
    if (!silent)
        printf(" MonoOut");
    codec_write(0x06, 0x803f);
    if (!silent)
        printf(" PCBeep");
    codec_write(0x0a, 0x9ffe);
    if (!silent)
        printf(" Phone");
    codec_write(0x0c, 0x801f);
    if (!silent)
        printf(" Video");
    codec_write(0x14, 0x9f1f);
    if (!silent)
        printf(" AuxIn");
    codec_write(0x16, 0x9f1f);
    if (!silent)
        printf(" GP");
    codec_write(0x20, 0xf380);
    if (!silent)
        printf(" 3D");
    codec_write(0x22, 0xffff);
    if (!silent)
        printf(" EAID");
    codec_write(0x28, codec_read(0x28) | 0x0030);
    if (!silent)
        printf(" C/LFE");
    codec_write(0x36, 0xbfbf);
    if (!silent)
        printf(" Surround");
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
        if (!silent)
            printf("%02X:", cur_reg);

        /* Iterate through register words on this range. */
        do {
            /* Add spacing at the halfway point. */
            if (!silent && ((cur_reg & 0x0f) == 0x08))
                putchar(' ');

            /* Read and print the word value. */
            *regp = codec_read(cur_reg);
            if (!silent)
                printf(" %04X", *regp);
            regp++;

            cur_reg += 2;
        } while (cur_reg & 0x0f);

        /* Move on to the next line. */
        if (!silent)
            putchar('\n');
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
    regval.u8[2]  = reg;
    regval.u16[0] = val;

    /* Perform write. */
    outl(io_base | 0x14, regval.u32);

    /* Wait for WIP to be cleared. */
    audiopci_codec_wait(&regval);
}

static void
audiopci_probe(uint16_t dev_id)
{
    uint8_t rev;

    /* Get revision. */
    rev = pci_readb(bus, dev, func, 0x08);

    /* Print controller information. */
    printf("Found AudioPCI %04X revision %02X at bus %02X device %02X function %d\n", dev_id, rev, bus, dev, func);
    printf("Subsystem ID [%04X:%04X]\n", pci_readw(bus, dev, func, 0x2c), pci_readw(bus, dev, func, 0x2e));

    /* Get I/O BAR. */
    io_base = pci_get_io_bar(bus, dev, func, 0x10, 64, "Main");
    if (!io_base)
        return;

    printf("GPIO readout [%02X]\n", inb(io_base | 0x02));

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
    regval.u8[2]  = reg;
    regval.u16[0] = val;

    /* Perform write. */
    outl(io_base | 0x80, regval.u32);

    /* Wait for Controller Busy to be cleared. */
    via_codec_wait(&regval, 0);
}

static void
via_probe(uint16_t dev_id)
{
    uint8_t rev, base_rev;

    /* Print controller information. */
    rev      = pci_readb(bus, dev, func, 0x08);
    base_rev = pci_readb(bus, dev, 0, 0x08);
    printf("Found VIA %04X revision %02X (base %02X) at bus %02X device %02X function %d\n", dev_id, rev, base_rev, bus, dev, func);

    /* Get SGD I/O BAR. */
    io_base = pci_get_io_bar(bus, dev, func, 0x10, 256, "SGD");
    if (!io_base)
        return;

    /* Set up AC-Link interface. */
    printf("Waking codec up... ");
    pci_writeb(bus, dev, func, 0x41, (dev_id == 0x3058) ? 0xc4 : 0xc0);
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
    if (dev_id == 0x3058) {
        alt_io_base = pci_get_io_bar(bus, dev, func, 0x1c, 256, "Codec Shadow");

        /* Perform tests if the BAR is set. */
        if (alt_io_base) {
            /* Reset codec. */
            via_codec_write(0x00, 0xffff);

            /* Test shadow behavior. */
            printf("Testing Codec Shadow:");

            i = via_codec_read(0x02);
            printf(" Read[%04X %04X]", i, inw(alt_io_base | 0x02));

            via_codec_write(0x02, 0xffff);
            printf("  Write[%04X %04X]", 0xffff, inw(alt_io_base | 0x02));

            via_codec_write(0x02, 0xffff);
            i = via_codec_read(0x02);
            printf("  WriteRead[%04X %04X]", i, inw(alt_io_base | 0x02));

            outw(alt_io_base | 0x02, 0x0000);
            i = inw(alt_io_base | 0x02);
            printf("\n                      DirectWrite[%04X %04X]", i, via_codec_read(0x02));
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
intel_probe(uint16_t dev_id)
{
    uint8_t rev;

    /* Print controller information. */
    rev = pci_readb(bus, dev, func, 0x08);
    printf("Found Intel %04X revision %02X at bus %02X device %02X function %d\n", dev_id, rev, bus, dev, func);

    /* Get Mixer I/O BAR. */
    io_base = pci_get_io_bar(bus, dev, func, 0x10, 256, "Mixer");
    if (!io_base)
        return;

    /* Get Bus Master I/O BAR. */
    alt_io_base = pci_get_io_bar(bus, dev, func, 0x14, 256, "Bus Master");
    if (!alt_io_base)
        return;

    /* Set up AC-Link interface. */
    printf("Waking codec up... ");
    outl(alt_io_base | 0x2c, inl(alt_io_base | 0x2c) & ~0x00000002);
    for (i = 1; i; i++) /* unknown delay required */
        inb(0xeb);
    outl(alt_io_base | 0x2c, inl(alt_io_base | 0x2c) | 0x00000002);
    for (i = 1; i; i++) {
        if (inl(alt_io_base | 0x30) & 0x00000100)
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
pci_scan_callback(uint8_t this_bus, uint8_t this_dev, uint8_t this_func,
                  uint16_t ven_id, uint16_t dev_id)
{
    bus  = this_bus;
    dev  = this_dev;
    func = this_func;

    if ((ven_id == 0x1274) && (dev_id != 0x5000)) {
        print_spacer();
        audiopci_probe(dev_id);
    } else if ((ven_id == 0x1106) && ((dev_id == 0x3058) || (dev_id == 0x3059))) {
        print_spacer();
        via_probe(dev_id);
    } else if ((ven_id == 0x8086) && ((dev_id == 0x2415) || (dev_id == 0x2425) || (dev_id == 0x2445) || (dev_id == 0x2485) || (dev_id == 0x24c5) || (dev_id == 0x24d5) || (dev_id == 0x25a6) || (dev_id == 0x266e) || (dev_id == 0x27de) || (dev_id == 0x7195))) {
        print_spacer();
        intel_probe(dev_id);
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

    /* Set silent mode. */
    if ((argc >= 1) && !strcmp(argv[1], "-s"))
        silent = 1;

    /* Scan PCI bus 0. */
    pci_scan_bus(0, pci_scan_callback);

    return 0;
}
