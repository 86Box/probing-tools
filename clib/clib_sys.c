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
#include "clib_sys.h"
#ifdef __POSIX_UEFI__
#    include <uefi.h>
#elif defined(_WIN32)
#    include <windows.h>
#elif defined(__GNUC__) && !defined(__POSIX_UEFI__)
#    include <unistd.h>
#endif

/* Interrupt functions. */
#ifdef __WATCOMC__
/* Defined in header. */
#elif defined(__GNUC__)
void
cli()
{
    __asm__("cli");
}

void
sti()
{
    __asm__("sti");
}
#else
void
cli()
{
}

void
sti()
{
}
#endif

/* Time functions. */
#ifndef __WATCOMC__
void
delay(unsigned int ms)
{
#    ifdef _WIN32
    Sleep(ms);
#    else
    usleep(ms * 1000);
#    endif
}
#endif

/* Port I/O functions. */
#ifdef __WATCOMC__
/* Defined in header. */
#elif defined(__GNUC__)
uint8_t
inb(uint16_t port)
{
    uint8_t ret;
    __asm__ __volatile__("inb %1, %0"
                         : "=a"(ret)
                         : "Nd"(port));
    return ret;
}

void
outb(uint16_t port, uint8_t val)
{
    __asm__ __volatile__("outb %0, %1"
                         :
                         : "a"(val), "Nd"(port));
}

uint16_t
inw(uint16_t port)
{
    uint16_t ret;
    __asm__ __volatile__("inw %1, %0"
                         : "=a"(ret)
                         : "Nd"(port));
    return ret;
}

void
outw(uint16_t port, uint16_t val)
{
    __asm__ __volatile__("outw %0, %1"
                         :
                         : "a"(val), "Nd"(port));
}

uint32_t
inl(uint16_t port)
{
    uint32_t ret;
    __asm__ __volatile__("inl %1, %0"
                         : "=a"(ret)
                         : "Nd"(port));
    return ret;
}

void
outl(uint16_t port, uint32_t val)
{
    __asm__ __volatile__("outl %0, %1"
                         :
                         : "a"(val), "Nd"(port));
}
#else
uint8_t
inb(uint16_t port)
{
    return 0xff;
}

void
outb(uint16_t port, uint8_t val)
{
}

uint16_t
inw(uint16_t port)
{
    return 0xffff;
}

void
outw(uint16_t port, uint16_t val)
{
}

uint32_t
inl(uint16_t port)
{
    return 0xffffffff;
}

void
outl(uint16_t port, uint32_t val)
{
}
#endif

uint16_t
io_find_range(uint16_t size)
{
    uint16_t base;

    for (base = 0x1000; base >= 0x1000; base += size) {
        /* Test first and last words only, as poking through the entire space
           can lead to trouble (VIA ACPI has magic reads that hang the CPU). */
        if ((inw(base) == 0xffff) && (inw(base + size - 2) == 0xffff))
            return base;
    }

    return 0;
}
