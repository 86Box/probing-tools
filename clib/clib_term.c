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
#    ifdef __GNUC__
#        include <sys/ioctl.h>
#        include <termios.h>
#        include <unistd.h>
#    endif
#endif
#ifdef MSDOS
#    include <dos.h>
#    include <graph.h>
#endif
#include "clib_term.h"

/* Positioning functions. */
#ifdef MSDOS
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
#elif defined(TIOCGWINSZ)
int
term_get_size_x()
{
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    return ws.ws_col;
}

int
term_get_size_y()
{
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    return ws.ws_row;
}

int
term_get_cursor_pos(uint8_t *x, uint8_t *y)
{
    int rx, ry, n;
    struct termios attrs;

    /* Capturing then uncapturing stdin like this is weird, but it'll have to do. */
    if (tcgetattr(STDOUT_FILENO, &attrs))
        return 0;
    int old_lflag = attrs.c_lflag;
    attrs.c_lflag &= ~(ECHO | ICANON);
    if (tcsetattr(STDOUT_FILENO, TCSANOW, &attrs))
        return 0;

    /* Send CPR request. */
    fputs("\033[6n", stderr);
    fflush(stderr);

    /* Read CPR response. */
    n = scanf("\033[%d;%dR", &ry, &rx);
    if (n >= 2) {
        *x = rx - 1;
        *y = ry - 1;
    }

    /* Uncapture stdin. */
    attrs.c_lflag = old_lflag;
    tcsetattr(STDOUT_FILENO, TCSANOW, &attrs);

    return n >= 2;
}

int
term_set_cursor_pos(uint8_t x, uint8_t y)
{
    fprintf(stdout, "\033[%d;%dH", y + 1, x + 1);
    fflush(stdout);
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
#ifdef MSDOS
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
