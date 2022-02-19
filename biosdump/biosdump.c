/*
 * 86Box	A hypervisor and IBM PC system emulator that specializes in
 *		running old operating systems and software designed for IBM
 *		PC systems and compatibles from 1981 through fairly recent
 *		system designs based on the PCI bus.
 *
 *		This file is part of the 86Box distribution.
 *
 *		BIOS dumping tool.
 *
 * Authors:	Miran Grca, <mgrca8@gmail.com>
 *
 *		Copyright 2021-2022 Miran Grca.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "clib.h"


int
#ifdef STANDARD_MAIN
main(int argc, char *argv[])
#else
main(void)
#endif
{
    FILE *f;
    char *p;
    uint32_t size, ret;

    /* Disable stdout buffering. */
    term_unbuffer_stdout();

    printf("Dumping low BIOS range (000C0000-000FFFFF)... ");
    p = (char *) 0x000c0000;
    size = 0x00040000;
    f = fopen("000c0000.dmp", "wb");
    if (f == NULL) {
        printf("FAILURE\n");
        return 1;
    }
    ret = fwrite(p, 1, size, f);
    if (ret != size) {
        printf("FAILURE\n");
        fclose(f);
        return 2;
    }
    fclose(f);
    printf("OK\n\n");

    printf("Dumping mid BIOS range (00FE0000-00FFFFFF)... ");
    p = (char *) 0x00fe0000;
    size = 0x00020000;
    f = fopen("00fe0000.dmp", "wb");
    if (f == NULL) {
        printf("FAILURE\n");
        return 3;
    }
    ret = fwrite(p, 1, size, f);
    if (ret != size) {
        printf("FAILURE\n");
        fclose(f);
        return 4;
    }
    fclose(f);
    printf("OK\n\n");

    printf("Dumping high BIOS range (FFF80000-FFFFFFFF)... ");
    p = (char *) 0xfff80000;
    size = 0x00080000;
    f = fopen("fff80000.dmp", "wb");
    if (f == NULL) {
        printf("FAILURE\n");
        return 5;
    }
    ret = fwrite(p, 1, size, f);
    if (ret != size) {
        printf("FAILURE\n");
        fclose(f);
        return 6;
    }
    fclose(f);
    printf("OK\n");

    return 0;
}
