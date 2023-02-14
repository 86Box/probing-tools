/*
 * 86Box    A hypervisor and IBM PC system emulator that specializes in
 *          running old operating systems and software designed for IBM
 *          PC systems and compatibles from 1981 through fairly recent
 *          system designs based on the PCI bus.
 *
 *          This file is part of the 86Box Probing Tools distribution.
 *
 *          Common library for C-based tools.
 *
 *
 *
 * Authors: RichardG, <richardg867@gmail.com>
 *
 *          Copyright 2021 RichardG.
 *
 */
#ifdef __POSIX_UEFI__
#    include <uefi.h>
#else
#    include <stdio.h>
#endif
#ifdef __WATCOMC__
#    include <dos.h>
#    include <graph.h>
#endif
#include "clib_term.h"

/* Positioning functions. */
#ifdef __WATCOMC__
static union REGPACK rp; /* things break if this is not a global variable... */

int
term_get_size_x()
{
    struct videoconfig vc;
    _getvideoconfig(&vc);
    return vc.numtextcols;
}

int
term_get_size_y()
{
    struct videoconfig vc;
    _getvideoconfig(&vc);
    return vc.numtextrows;
}

int
term_get_cursor_pos(uint8_t *x, uint8_t *y)
{
    rp.h.ah = 0x03;
    rp.h.bh = 0x00;
    intr(0x10, &rp);
    *x = rp.h.dl;
    *y = rp.h.dh;
    return 1;
}

int
term_set_cursor_pos(uint8_t x, uint8_t y)
{
    rp.h.ah = 0x02;
    rp.h.dl = x;
    rp.h.dh = y;
    intr(0x10, &rp);
    return 1;
}
#else
int
term_get_size_x()
{
    return 80;
}

int
term_get_size_y()
{
    return 25;
}

int
term_get_cursor_pos(uint8_t *x, uint8_t *y)
{
    return 0;
}

int
term_set_cursor_pos(uint8_t x, uint8_t y)
{
    return 0;
}
#endif

/* Output functions. */
#ifdef __WATCOMC__
void
term_unbuffer_stdout()
{
    setbuf(stdout, NULL);
}

void
term_final_linebreak()
{
    /* DOS already outputs a final line break. */
}
#else
void
term_unbuffer_stdout()
{
}

void
term_final_linebreak()
{
    printf("\n");
}
#endif
