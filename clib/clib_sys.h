/*
 * 86Box    A hypervisor and IBM PC system emulator that specializes in
 *          running old operating systems and software designed for IBM
 *          PC systems and compatibles from 1981 through fairly recent
 *          system designs based on the PCI bus.
 *
 *          This file is part of the 86Box Probing Tools distribution.
 *
 *          Definitions for the common library for C-based tools.
 *
 *
 *
 * Authors: RichardG, <richardg867@gmail.com>
 *
 *          Copyright 2021 RichardG.
 *
 */
#ifndef CLIB_SYS_H
#define CLIB_SYS_H
#include "clib.h"

/* Interrupt functions. */
#ifdef __WATCOMC__
void cli();
#    pragma aux cli = "cli";
void sti();
#    pragma aux sti = "sti";
#else
extern void     cli();
extern void     sti();
#endif

/* Port I/O functions. */
#ifdef __WATCOMC__
uint8_t inb(uint16_t port);
#    pragma aux inb = "in al, dx" parm[dx] value[al];
void outb(uint16_t port, uint8_t data);
#    pragma aux outb = "out dx, al" parm[dx][al];
uint16_t inw(uint16_t port);
#    pragma aux inw = "in ax, dx" parm[dx] value[ax];
void outw(uint16_t port, uint16_t data);
#    pragma aux outw = "out dx, ax" parm[dx][ax];
#    ifdef M_I386
uint32_t inl(uint16_t port);
#        pragma aux inl = "in eax, dx" parm[dx] value[eax];
void outl(uint16_t port, uint32_t data);
#        pragma aux outl = "out dx, eax" parm[dx][eax];
#    else
/* Some manual prefixing trickery to perform 32-bit I/O and access
   the extended part of EAX in real mode. Exchanging is necessary
   due to Watcom ignoring the order registers are specified in... */
uint32_t inl(uint16_t port);
#        pragma aux inl = "db 0x66"                                     \
                          "in ax, dx"                 /* in eax, dx */  \
                          "mov cx, ax"                                  \
                          "db 0x66, 0xc1, 0xe8, 0x10" /* shr eax, 16 */ \
                          "xchg ax, cx" parm[dx] value[ax cx];
void     outl(uint16_t port, uint32_t data);
#        pragma aux                                       outl = "xchg ax, cx"                                 \
                                                                 "db 0x66, 0xc1, 0xe0, 0x10" /* shl eax, 16 */ \
                           "mov ax, cx"                                                                        \
                           "db 0x66"                                                                           \
                           "out dx, ax" /* out dx, eax */                                                      \
            parm[dx][ax cx] modify[ax cx];
#    endif
#else
#    if defined(__GNUC__) && !defined(__POSIX_UEFI__)
#        define inb  sys_inb
#        define outb sys_outb
#        define inw  sys_inw
#        define outw sys_outw
#        define inl  sys_inl
#        define outl sys_outl
#        include <sys/io.h>
#        undef inb
#        undef outb
#        undef inw
#        undef outw
#        undef inl
#        undef outl
#    endif
extern uint8_t  inb(uint16_t port);
extern void     outb(uint16_t port, uint8_t data);
extern uint16_t inw(uint16_t port);
extern void     outw(uint16_t port, uint16_t data);
extern uint32_t inl(uint16_t port);
extern void     outl(uint16_t port, uint32_t data);
#endif
extern uint16_t io_find_range(uint16_t size);

#endif
