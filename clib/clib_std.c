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
#    include <string.h>
#endif
#include "clib_std.h"

/* String functions. */
int
parse_hex_u8(char *val, uint8_t *dest)
{
    uint32_t dest32;
    int      ret = parse_hex_u32(val, &dest32);
    *dest        = dest32;
    return ret;
}

int
parse_hex_u16(char *val, uint16_t *dest)
{
    uint32_t dest32;
    int      ret = parse_hex_u32(val, &dest32);
    *dest        = dest32;
    return ret;
}

int
parse_hex_u32(char *val, uint32_t *dest)
{
    int     i, len = strlen(val);
    uint8_t digit;

    *dest = 0;
    for (i = 0; i < len; i++) {
        if ((val[i] >= 0x30) && (val[i] <= 0x39))
            digit = val[i] - 0x30;
        else if ((val[i] >= 0x41) && (val[i] <= 0x46))
            digit = val[i] - 0x37;
        else if ((val[i] >= 0x61) && (val[i] <= 0x66))
            digit = val[i] - 0x57;
        else
            return 0;
        *dest = (*dest << 4) | digit;
    }

    return 1;
}

/* Comparator functions. */
int
comp_ui8(const void *elem1, const void *elem2)
{
    uint8_t a = *((uint8_t *) elem1);
    uint8_t b = *((uint8_t *) elem2);
    return ((a < b) ? -1 : ((a > b) ? 1 : 0));
}
