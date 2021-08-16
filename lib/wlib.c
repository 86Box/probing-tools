/*
 * 86Box	A hypervisor and IBM PC system emulator that specializes in
 *		running old operating systems and software designed for IBM
 *		PC systems and compatibles from 1981 through fairly recent
 *		system designs based on the PCI bus.
 *
 *		This file is part of the 86Box Probing Tools distribution.
 *
 *		Common functions for Watcom C-based tools.
 *
 *
 *
 * Authors:	RichardG, <richardg867@gmail.com>
 *
 *		Copyright 2021 RichardG.
 *
 */
#include <stdint.h>
#include "wlib.h"


uint32_t
pci_cf8(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg)
{
    /* Generate a PCI port CF8h dword. */
    multi_t ret;
    ret.u8[3] = 0x80;
    ret.u8[2] = bus;
    ret.u8[1] = dev << 3;
    ret.u8[1] |= func & 7;
    ret.u8[0] = reg & 0xfc;
    return ret.u32;
}


uint8_t
pci_readb(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg)
{
    uint8_t ret;
    uint16_t data_port = 0xcfc | (reg & 0x03);
    uint32_t cf8 = pci_cf8(bus, dev, func, reg);
    cli();
    outl(0xcf8, cf8);
    ret = inb(data_port);
    sti();
    return ret;
}


uint16_t
pci_readw(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg)
{
    uint16_t ret, data_port = 0xcfc | (reg & 0x02);
    uint32_t cf8 = pci_cf8(bus, dev, func, reg);
    cli();
    outl(0xcf8, cf8);
    ret = inw(data_port);
    sti();
    return ret;
}


uint32_t
pci_readl(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg)
{
    uint32_t ret, cf8 = pci_cf8(bus, dev, func, reg);
    cli();
    outl(0xcf8, cf8);
    ret = inl(0xcfc);
    sti();
    return ret;
}


void
pci_writeb(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg, uint8_t val)
{
    uint16_t data_port = 0xcfc | (reg & 0x03);
    uint32_t cf8 = pci_cf8(bus, dev, func, reg);
    cli();
    outl(0xcf8, cf8);
    outb(data_port, val);
    sti();
}


void
pci_writew(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg, uint16_t val)
{
    uint16_t data_port = 0xcfc | (reg & 0x02);
    uint32_t cf8 = pci_cf8(bus, dev, func, reg);
    cli();
    outl(0xcf8, cf8);
    outw(data_port, val);
    sti();
}


void
pci_writel(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg, uint32_t val)
{
    uint32_t cf8 = pci_cf8(bus, dev, func, reg);
    cli();
    outl(0xcf8, cf8);
    outl(0xcfc, val);
    sti();
}
