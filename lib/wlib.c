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
