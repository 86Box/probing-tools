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
#ifndef CLIB_TERM_H
#define CLIB_TERM_H
#include "clib.h"

/* Positioning functions. */
extern int term_get_size_x();
extern int term_get_size_y();
extern int term_get_cursor_pos(uint8_t *x, uint8_t *y);
extern int term_set_cursor_pos(uint8_t x, uint8_t y);

/* Output functions. */
extern void term_unbuffer_stdout();
extern void term_final_linebreak();

#endif
