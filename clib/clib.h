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
#ifndef CLIB_H
#define CLIB_H
#ifdef __POSIX_UEFI__
#    include <uefi.h>
#else
#    include <inttypes.h>
#endif

/* Common macros. */
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define ABS(x)    ((x) > 0 ? (x) : -(x))

/* Compiler compatibility macros. */
#ifdef __GNUC__
#    define PACKED __attribute__((packed))
#else
#    define PACKED
#endif

/* Platform-specific macros. */
#ifndef __POSIX_UEFI__
#    define FMT_FLOAT_SUPPORTED 1
#endif
#if !defined(__WATCOMC__) || defined(M_I386)
#    define IS_32BIT 1
#endif

#pragma pack(push, 0)
/* Convenience type for breaking a dword value down into words and bytes. */
typedef union {
    uint8_t  u8[4];
    uint16_t u16[2];
    uint32_t u32;
} multi_t;
#pragma pack(pop)

#endif
