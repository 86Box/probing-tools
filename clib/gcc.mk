#
# 86Box		A hypervisor and IBM PC system emulator that specializes in
#		running old operating systems and software designed for IBM
#		PC systems and compatibles from 1981 through fairly recent
#		system designs based on the PCI bus.
#
#		This file is part of the 86Box Probing Tools distribution.
#
#		Base makefile for compiling C-based tools with gcc.
#
#
#
# Authors:	RichardG, <richardg867@gmail.com>
#
#		Copyright 2023 RichardG.
#

VPATH		= . ../clib
CC			?= "gcc"

default: $(DEST)

%.o: %.c $(HEADERS)
	$(CC) -I../clib $(CFLAGS) -c $< -o $@

$(DEST): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@
	chmod +x $@ || true

clean:
	-rm -f $(OBJS) $(DEST)
