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
#ifndef CLIB_STD_H
#define CLIB_STD_H
#include "clib.h"

/* String functions. */
extern int parse_hex_u8(char *val, uint8_t *dest);
extern int parse_hex_u16(char *val, uint16_t *dest);
extern int parse_hex_u32(char *val, uint32_t *dest);

/* Comparator functions. */
extern int comp_ui8(const void *elem1, const void *elem2);

#endif
