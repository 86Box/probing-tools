/*
 * 86Box	A hypervisor and IBM PC system emulator that specializes in
 *		running old operating systems and software designed for IBM
 *		PC systems and compatibles from 1981 through fairly recent
 *		system designs based on the PCI bus.
 *
 *		This file is part of the 86Box Probing Tools distribution.
 *
 *		UTF-8 -> CP437 source file conversion tool.
 *
 *
 *
 * Authors:	RichardG, <richardg867@gmail.com>
 *
 *		Copyright 2021 RichardG.
 */
#include <malloc.h>
#include <stdio.h>
#include <string.h>


/* This is the text renderer's CP437 character conversion table,
   with 00, 09, 0A and 0D modified for text processing purposes. */
static const char *cp437[] = {
    /* 00 */ "",             "\xE2\x98\xBA", "\xE2\x98\xBB", "\xE2\x99\xA5", "\xE2\x99\xA6", "\xE2\x99\xA3", "\xE2\x99\xA0", "\xE2\x80\xA2", "\xE2\x97\x98", "\t",           "\n",           "\xE2\x99\x82", "\xE2\x99\x80", "\r",           "\xE2\x99\xAB", "\xE2\x98\xBC",
    /* 10 */ "\xE2\x96\xBA", "\xE2\x97\x84", "\xE2\x86\x95", "\xE2\x80\xBC", "\xC2\xB6",     "\xC2\xA7",     "\xE2\x96\xAC", "\xE2\x86\xA8", "\xE2\x86\x91", "\xE2\x86\x93", "\xE2\x86\x92", "\xE2\x86\x90", "\xE2\x88\x9F", "\xE2\x86\x94", "\xE2\x96\xB2", "\xE2\x96\xBC",
    /* 20 */ " ",            "!",            "\"",           "#",            "$",            "%",            "&",            "'",            "(",            ")",            "*",            "+",            ",",            "-",            ".",            "/",
    /* 30 */ "0",            "1",            "2",            "3",            "4",            "5",            "6",            "7",            "8",            "9",            ":",            ";",            "<",            "=",            ">",            "?",
    /* 40 */ "@",            "A",            "B",            "C",            "D",            "E",            "F",            "G",            "H",            "I",            "J",            "K",            "L",            "M",            "N",            "O",
    /* 50 */ "P",            "Q",            "R",            "S",            "T",            "U",            "V",            "W",            "X",            "Y",            "Z",            "[",            "\\",           "]",            "^",            "_",
    /* 60 */ "`",            "a",            "b",            "c",            "d",            "e",            "f",            "g",            "h",            "i",            "j",            "k",            "l",            "m",            "n",            "o",
    /* 70 */ "p",            "q",            "r",            "s",            "t",            "u",            "v",            "w",            "x",            "y",            "z",            "{",            "|",            "}",            "~",            "\xE2\x8C\x82",
    /* 80 */ "\xC3\x87",     "\xC3\xBC",     "\xC3\xA9",     "\xC3\xA2",     "\xC3\xA4",     "\xC3\xA0",     "\xC3\xA5",     "\xC3\xA7",     "\xC3\xAA",     "\xC3\xAB",     "\xC3\xA8",     "\xC3\xAF",     "\xC3\xAE",     "\xC3\xAC",     "\xC3\x84",     "\xC3\x85",
    /* 90 */ "\xC3\x89",     "\xC3\xA6",     "\xC3\x86",     "\xC3\xB4",     "\xC3\xB6",     "\xC3\xB2",     "\xC3\xBB",     "\xC3\xB9",     "\xC3\xBF",     "\xC3\x96",     "\xC3\x9C",     "\xC2\xA2",     "\xC2\xA3",     "\xC2\xA5",     "\xE2\x82\xA7", "\xC6\x92",
    /* A0 */ "\xC3\xA1",     "\xC3\xAD",     "\xC3\xB3",     "\xC3\xBA",     "\xC3\xB1",     "\xC3\x91",     "\xC2\xAA",     "\xC2\xBA",     "\xC2\xBF",     "\xE2\x8C\x90", "\xC2\xAC",     "\xC2\xBD",     "\xC2\xBC",     "\xC2\xA1",     "\xC2\xAB",     "\xC2\xBB",
    /* B0 */ "\xE2\x96\x91", "\xE2\x96\x92", "\xE2\x96\x93", "\xE2\x94\x82", "\xE2\x94\xA4", "\xE2\x95\xA1", "\xE2\x95\xA2", "\xE2\x95\x96", "\xE2\x95\x95", "\xE2\x95\xA3", "\xE2\x95\x91", "\xE2\x95\x97", "\xE2\x95\x9D", "\xE2\x95\x9C", "\xE2\x95\x9B", "\xE2\x94\x90",
    /* C0 */ "\xE2\x94\x94", "\xE2\x94\xB4", "\xE2\x94\xAC", "\xE2\x94\x9C", "\xE2\x94\x80", "\xE2\x94\xBC", "\xE2\x95\x9E", "\xE2\x95\x9F", "\xE2\x95\x9A", "\xE2\x95\x94", "\xE2\x95\xA9", "\xE2\x95\xA6", "\xE2\x95\xA0", "\xE2\x95\x90", "\xE2\x95\xAC", "\xE2\x95\xA7",
    /* D0 */ "\xE2\x95\xA8", "\xE2\x95\xA4", "\xE2\x95\xA5", "\xE2\x95\x99", "\xE2\x95\x98", "\xE2\x95\x92", "\xE2\x95\x93", "\xE2\x95\xAB", "\xE2\x95\xAA", "\xE2\x94\x98", "\xE2\x94\x8C", "\xE2\x96\x88", "\xE2\x96\x84", "\xE2\x96\x8C", "\xE2\x96\x90", "\xE2\x96\x80",
    /* E0 */ "\xCE\xB1",     "\xC3\x9F",     "\xCE\x93",     "\xCF\x80",     "\xCE\xA3",     "\xCF\x83",     "\xC2\xB5",     "\xCF\x84",     "\xCE\xA6",     "\xCE\x98",     "\xCE\xA9",     "\xCE\xB4",     "\xE2\x88\x9E", "\xCF\x86",     "\xCE\xB5",     "\xE2\x88\xA9",
    /* F0 */ "\xE2\x89\xA1", "\xC2\xB1",     "\xE2\x89\xA5", "\xE2\x89\xA4", "\xE2\x8C\xA0", "\xE2\x8C\xA1", "\xC3\xB7",     "\xE2\x89\x88", "\xC2\xB0",     "\xE2\x88\x99", "\xC2\xB7",     "\xE2\x88\x9A", "\xE2\x81\xBF", "\xC2\xB2",     "\xE2\x96\xA0", "\xC2\xA0"
};


int
main(int argc, char **argv)
{
    int i, j, success;
    char *buf;
    FILE *fin, *fout;

    /* Disable stdout/stderr buffering. */
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    /* Print usage if no input files were specified. */
    if (argc < 2) {
	printf("Usage:\n");
	printf("\n");
	printf("cp437 infile [infile...]\n");
	printf("- Converts UTF-8 input file(s) to CP437 output file(s) with .cp437 appended.\n");
	printf("  The new file names are also printed to stdout.\n");
	return 1;
    }

    /* Process input files. */
    success = 0;
    for (i = 1; i < argc; i++) {
	/* Open input file. */
	fin = fopen(argv[i], "rb");
	if (!fin) {
		fprintf(stderr, "Could not open input file \"%s\"\n", argv[i]);
		continue;
	}

	/* Generate output file name. */
	buf = malloc(strlen(argv[i]) + 7);
	sprintf(buf, "%s_cp437", argv[i]);

	/* Open output file. */
	fout = fopen(buf, "wb");
	if (!fout) {
		fclose(fin);
		fprintf(stderr, "Could not open output file \"%s\"\n", buf);
		continue;
	}

	printf("%s\n", buf);

	/* Perform the conversion. */
	while (!feof(fin)) {
		/* Read UTF-8 codepoint. */
		memset(buf, 0, 5);
		fread(buf, 1, 1, fin);
		if ((buf[0] & 0xe0) == 0xc0)
			fread(&buf[1], 1, 1, fin);
		else if ((buf[0] & 0xf0) == 0xe0)
			fread(&buf[1], 2, 1, fin);
		else if ((buf[0] & 0xf8) == 0xf0)
			fread(&buf[1], 3, 1, fin);

		for (j = 1; j < (sizeof(cp437) / sizeof(cp437[0])); j++) {
			if (!strcmp(buf, cp437[j])) {
				fputc(j, fout);
				break;
			}
		}
	}

	/* Clean up. */
	free(buf);
	fclose(fin);
	fclose(fout);

	/* Increase success counter. */
	success++;
    }

    if (--i == success)
	return 0;
    else if (success > 0)
	return 1;
    else
	return 2;
}
