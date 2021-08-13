#!/bin/sh
#
# 86Box		A hypervisor and IBM PC system emulator that specializes in
#		running old operating systems and software designed for IBM
#		PC systems and compatibles from 1981 through fairly recent
#		system designs based on the PCI bus.
#
#		This file is part of the 86Box Probing Tools distribution.
#
#		Universal *nix build script for Watcom C-based DOS tools.
#
#
#
# Authors:	RichardG, <richardg867@gmail.com>
#
#		Copyright 2021 RichardG.
#

# Check for Watcom environment.
if ! wcl -v >/dev/null 2>&1
then
    echo '***' Watcom compiler not found. Make sure OpenWatcom is installed and in \$PATH.
    exit 1
fi

# Check for cp437 tool and build it if necessary.
if [ ! -x ../cp437/cp437 ]
then
    echo '***' Building cp437 conversion tool for your host system...
    pushd ../cp437
    if ! ./build.sh
    then
	exit $?
    fi
    popd
fi

# Find source file.
srcfile="$(ls *.c 2>/dev/null | head -1)"
if [ "x$srcfile" == "x" ]
then
    echo '***' Source file not found.
    exit 2
fi

# Convert source file to CP437.
echo '***' Converting $srcfile to CP437...
if ! ../cp437/cp437 "$srcfile"
then
    echo '***' Conversion failed.
    exit 3
fi

# Generate output file name.
destfile=$(basename "$srcfile" .c | tr [:lower:] [:upper:]).EXE

# Call compiler and check for success.
if wcl -bcl=dos -fo="$destfile" "$srcfile".cp437
then
    echo '***' Build successful.
else
    echo '***' Build failed.
    exit 4
fi
