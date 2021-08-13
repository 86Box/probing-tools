#!/bin/sh
#
# 86Box		A hypervisor and IBM PC system emulator that specializes in
#		running old operating systems and software designed for IBM
#		PC systems and compatibles from 1981 through fairly recent
#		system designs based on the PCI bus.
#
#		This file is part of the 86Box Probing Tools distribution.
#
#		Universal *nix build script for Watcom C-based host tools.
#
#
#
# Authors:	RichardG, <richardg867@gmail.com>
#
#		Copyright 2021 RichardG.
#

if ! wcl386 -v >/dev/null 2>&1
then
    echo '***' Watcom compiler not found. Make sure OpenWatcom is installed and in \$PATH.
    exit 1
fi

# Find source file.
srcfile="$(ls *.c 2>/dev/null | head -1)"
if [ "x$srcfile" == "x" ]
then
    echo '***' Source file not found.
    exit 2
fi

# Generate output file name.
destfile=$(basename "$srcfile" .c)

# Call compiler and check for success.
if wcl386 -bcl=linux -fo="$destfile" "$srcfile"
then
    echo '***' Build successful.
    chmod +x "$destfile"
else
    echo '***' Build failed.
    exit 4
fi
