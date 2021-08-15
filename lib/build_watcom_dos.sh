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

# Convert source file to CP437.
echo '***' Converting $1 to CP437...
if ! ../cp437/cp437 "$1"
then
    echo '***' Conversion failed.
    exit 2
fi

# Generate output file name.
destfile=$(basename "$1" .c | tr [:lower:] [:upper:]).EXE

# Call compiler and check for success.
if wcl -bcl=dos -fo="$destfile" "$1".cp437
then
    echo '***' Build successful.
else
    echo '***' Build failed.
    exit 3
fi
