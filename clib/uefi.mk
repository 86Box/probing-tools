#
# 86Box		A hypervisor and IBM PC system emulator that specializes in
#		running old operating systems and software designed for IBM
#		PC systems and compatibles from 1981 through fairly recent
#		system designs based on the PCI bus.
#
#		This file is part of the 86Box Probing Tools distribution.
#
#		Base makefile for compiling C-based tools with POSIX-UEFI.
#
#
#
# Authors:	RichardG, <richardg867@gmail.com>
#
#		Copyright 2021 RichardG.
#

VPATH		= . ../clib

export TARGET	= $(DEST)
export SRCS	= $(OBJS:.o=.c)
export CFLAGS	= -mno-sse -D__POSIX_UEFI__ -I../clib
export USE_GCC	= 1

include uefi/Makefile
