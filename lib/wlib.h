/*
 * 86Box	A hypervisor and IBM PC system emulator that specializes in
 *		running old operating systems and software designed for IBM
 *		PC systems and compatibles from 1981 through fairly recent
 *		system designs based on the PCI bus.
 *
 *		This file is part of the 86Box Probing Tools distribution.
 *
 *		Definitions for common functions for Watcom C-based tools.
 *
 *
 *
 * Authors:	RichardG, <richardg867@gmail.com>
 *
 *		Copyright 2021 RichardG.
 *
 */
#ifndef WLIB_H
# define WLIB_H
#include <stdint.h>

#pragma pack(push, 0)
/* Convenience type for breaking a dword value down into words and bytes. */
typedef union {
    uint8_t u8[4];
    uint16_t u16[2];
    uint32_t u32;
} multi_t;
#pragma pack(pop)


/* Port I/O functions. */
uint8_t inb(uint16_t port);
#pragma aux inb = "in al, dx" parm [dx] value [al];
void outb(uint16_t port, uint8_t data);
#pragma aux outb = "out dx, al" parm [dx] [al];
uint16_t inw(uint16_t port);
#pragma aux inw = "in ax, dx" parm [dx] value [ax];
void outw(uint16_t port, uint16_t data);
#pragma aux outw = "out dx, ax" parm [dx] [ax];
#ifdef M_I386
uint32_t inl(uint16_t port);
# pragma aux inl = "in eax, dx" parm [dx] value [eax];
void outl(uint16_t port, uint32_t data);
# pragma aux outl = "out dx, eax" parm [dx] [eax];
#else
/* Some manual prefixing trickery to perform 32-bit I/O and access
   the extended part of EAX in real mode. Exchanging is necessary
   due to Watcom ignoring the order registers are specified in... */
uint32_t inl(uint16_t port);
# pragma aux inl =	"db 0x66" "in ax, dx" /* in eax, dx */ \
			"mov cx, ax" \
			"db 0x66, 0xc1, 0xe8, 0x10" /* shr eax, 16 */ \
			"xchg ax, cx" \
			parm [dx] \
			value [ax cx];
void outl(uint16_t port, uint32_t data);
# pragma aux outl =	"xchg ax, cx" \
			"db 0x66, 0xc1, 0xe0, 0x10" /* shl eax, 16 */ \
			"mov ax, cx" \
			"db 0x66" "out dx, ax" /* out dx, eax */ \
			parm [dx] [ax cx] \
			modify [ax cx];
#endif


/* PCI functions. */
extern uint32_t	pci_cf8(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg);

#endif
