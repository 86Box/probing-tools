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


static int
dump_range(uint32_t base, uint32_t size, const char *id)
{
    char fn[13];
    FILE *f;

    /* Output the range being dumped. */
    printf("Dumping %s BIOS range (%08X-%08X)... ", id, base, base + size - 1);

    /* Generate file name. */
    sprintf(fn, "%08X.DMP", base);

    /* Open the dump file. */
    f = fopen(fn, "wb");
    if (!f) {
	printf("FAILURE\n");
	return 1;
    }

    /* Write the dump. */
    if (fwrite((char *) base, 1, size, f) != size) {
	printf("FAILURE\n");
	fclose(f);
	return 2;
    }

    /* Finish the dump. */
    fclose(f);
    printf("OK\n");

    return 0;
}


int
#ifdef STANDARD_MAIN
main(int argc, char *argv[])
#else
main(void)
#endif
{
    int ret;

    /* Disable stdout buffering. */
    term_unbuffer_stdout();

    /* Dump ranges. */
    if ((ret = dump_range(0x000c0000, 0x00040000, "low")))
	return ret;
    if ((ret = dump_range(0x00fe0000, 0x00020000, "mid")))
	return 2 + ret;
    if ((ret = dump_range(0xfff80000, 0x00080000, "high")))
	return 4 + ret;

    return 0;
}
