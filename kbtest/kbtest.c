/*
 * 86Box	A hypervisor and IBM PC system emulator that specializes in
 *		running old operating systems and software designed for IBM
 *		PC systems and compatibles from 1981 through fairly recent
 *		system designs based on the PCI bus.
 *
 *		This file is part of the 86Box Probing Tools distribution.
 *
 *		PS/2 keyboard probing tool.
 *
 *
 *
 * Authors:	RichardG, <richardg867@gmail.com>
 *
 *		Copyright 2024 RichardG.
 *
 * ┌──────────────────────────────────────────────────────────────┐
 * │ This file is UTF-8 encoded. If this text is surrounded by    │
 * │ garbage, please tell your editor to open this file as UTF-8. │
 * └──────────────────────────────────────────────────────────────┘
 */
#include <inttypes.h>
#include <stdio.h>
#include "clib_sys.h"
#include "clib_term.h"

static uint8_t
send_cmd(uint8_t cmd)
{
    uint16_t i;
    for (i = 1; i; i++) {
        if (!(inb(0x64) & 0x02))
            break;
    }
    if (!i) {
        sti();
        printf("send_cmd(%02X) timed out\n", cmd);
        cli();
        return 0;
    }
    outb(0x64, cmd);
    return 1;
}

static uint8_t
send_data(uint8_t data)
{
    uint16_t i;
    for (i = 1; i; i++) {
        if (!(inb(0x64) & 0x02))
            break;
    }
    if (!i) {
        sti();
        printf("send_data(%02X) timed out\n", data);
        cli();
        return 0;
    }
    outb(0x60, data);
    return 1;
}

static uint16_t
read_data()
{
    uint16_t i;
    for (i = 1; i; i++) {
        if (inb(0x64) & 0x01)
            break;
    }
    if (!i)
        return -1;
    return inb(0x60);
}

int
main(int argc, char **argv)
{
    uint16_t config, data[16];
    uint8_t old_scanset, new_scanset, i, j;

    term_unbuffer_stdout();
    cli();

    /* Flush output buffer. */
    i = inb(0x60);

    /* Disable scanning. */
    send_data(0xf5);
    if ((data[0] = read_data()) != 0xfa) {
        sti();
        printf("Disable scanning failed (%02X)\n", data[0]);
        return 1;
    }

    /* Disable interrupts and translation. */
    send_cmd(0x20);
    config = read_data();
    sti();
    if (config > 0xff) {
        printf("Controller configuration read failed\n");
        return 1;
    }
    printf("Controller configuration: %02X\n", config);
    cli();
    send_cmd(0x60);
    send_data(config & ~0x43);

    /* Read and print keyboard ID. */
    send_data(0xf2);
    if ((data[0] = read_data()) != 0xfa) {
        sti();
        printf("Keyboard ID failed (%02X)\n", data[0]);
    } else {
        while ((data[0] = read_data()) > 0xff);
        data[1] = read_data();
        sti();
        if (data[1] < 0x100)
            printf("Keyboard ID: %02X %02X\n", data[0], data[1]);
        else
            printf("Keyboard ID: %02X\n", data[0]);
    }
    cli();

    /* Read and print scan code set. */
    data[1] = -2;
    data[2] = -2;
    send_data(0xf0);
    if ((data[0] = read_data()) != 0xfa) {
get_set_fail:
        sti();
        printf("Get scan code set failed (%02X %02X %02X)\n", data[0], data[1], data[2]);
        return 1;
    } else {
        send_data(0x00);
        if ((data[1] = read_data()) != 0xfa)
            goto get_set_fail;
        else if ((data[2] = read_data()) > 0xff)
            goto get_set_fail;
    }
    old_scanset = data[2];
    new_scanset = (argc > 1) ? atoi(argv[1]) : 3;
    sti();
    printf("Scan code set: %02X (switching to %02X)\n", old_scanset, new_scanset);
    cli();

    /* Enable specified scan code set. */
    data[1] = -2;
    send_data(0xf0);
    if ((data[0] = read_data()) != 0xfa) {
set_3_fail:
        sti();
        printf("Enable set %d failed (%02X %02X)\n", new_scanset, data[0], data[1]);
        return 1;
    } else {
        send_data(0x03);
        if ((data[1] = read_data()) != 0xfa)
            goto set_3_fail;
    }

    /* Reenable scanning. */
    send_data(0xf4);
    if ((data[0] = read_data()) != 0xfa) {
        sti();
        printf("Enable scanning failed (%02X)\n", data[0]);
        return 1;
    }

    /* Read and print incoming bytes. */
    do {
        while ((data[0] = read_data()) > 0xff);
        for (i = 1; i < 16; i++) {
            if ((data[i] = read_data()) > 0xff)
                break;
        }
        sti();
        for (j = 0; j < i; j++)
            printf("%02X ", data[j]);
        cli();
    } while (data[0] != 0x1c);
    sti();
    printf("\n");
    cli();

    /* Disable scanning. */
    send_data(0xf5);
    if ((data[0] = read_data()) != 0xfa) {
        sti();
        printf("Disable scanning failed (%02X)\n", data[0]);
        return 1;
    }

    /* Restore old scan code set. */
    data[1] = -2;
    send_data(0xf0);
    if ((data[0] = read_data()) != 0xfa) {
restore_set_fail:
        sti();
        printf("Restore old scan code set failed (%02X %02X)\n", data[0], data[1]);
        return 1;
    } else {
        send_data(old_scanset);
        if ((data[1] = read_data()) != 0xfa)
            goto restore_set_fail;
    }

    /* Restore old configuration. */
    send_cmd(0x60);
    send_data(config);

    /* Reenable scanning. */
    send_data(0xf4);
    if ((data[0] = read_data()) != 0xfa) {
        sti();
        printf("Enable scanning failed (%02X)\n", data[0]);
        return 1;
    }

    /* We're done here. */
    sti();

    return 0;
}
