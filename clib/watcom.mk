#
# 86Box		A hypervisor and IBM PC system emulator that specializes in
#		running old operating systems and software designed for IBM
#		PC systems and compatibles from 1981 through fairly recent
#		system designs based on the PCI bus.
#
#		This file is part of the 86Box Probing Tools distribution.
#
#		Base makefile for compiling C-based tools with Watcom.
#
#
#
# Authors:	RichardG, <richardg867@gmail.com>
#
#		Copyright 2021 RichardG.
#

# Establish host-specific stuff.
!ifdef __UNIX__
DEL		= rm -f
COPY		= cp
CP437		= cp437
SLASH		= /
!else
DEL		= del
COPY		= copy /y
CP437		= cp437.exe
SLASH		= \
!if "$(SYSTEM)" == "HOST"
# Build a Windows NT character-mode executable on Windows,
# as that is not the default target (Win16 is).
SYSTEM		= NT
DEST		= $(DEST).exe
!endif
!endif

# Establish target information.
!if "$(SYSTEM)" == "DOS"
CC		= wcc
CP437_CONV	= y
!else
CC		= wcc386
!if "$(SYSTEM)" == "PMODEW"
CP437_CONV	= y
!else
CP437_CONV	= n
!endif
!endif
LINK		= wlink

# Establish target C flags.
CFLAGS		= -i=..$(SLASH)clib $(CFLAGS)
!if "$(SYSTEM)" != "HOST"
CFLAGS		= -bt=$(SYSTEM) $(CFLAGS)
!endif


# Include clib.
.c:		../clib

# Compile source file into object file.
.c.obj
!if "$(CP437_CONV)" == "y"
		..$(SLASH)cp437$(SLASH)$(CP437) $<
		$(CC) $(CFLAGS) $<_cp437 -fo=$*.obj
		@$(DEL) $<_cp437
!else
		$(CC) $(CFLAGS) $< -fo=$*.obj
!endif

# Default target, which must be the first!
all:		..$(SLASH)cp437$(SLASH)$(CP437) $(DEST)

# cp437 host tool compilation target.
# Only valid if cp437 conversion is needed.
..$(SLASH)cp437$(SLASH)$(CP437):
!if "$(CP437_CONV)" == "y"
		cd ..$(SLASH)cp437 & wmake $(CP437)
!endif

# Main target.
$(DEST):	$(OBJS)
!if "$(SYSTEM)" == "PMODEW"
		$(COPY) $(%WATCOM)$(SLASH)binw$(SLASH)pmodew.exe .$(SLASH)
!endif
		%write $@.lnk NAME   $@
!if "$(SYSTEM)" != "HOST"
		%write $@.lnk SYSTEM $(SYSTEM)
!endif
		%write $@.lnk FILE   {$(OBJS)}
		$(LINK) $(LDFLAGS) @$@.lnk
		$(DEL) $@.lnk

clean:		.symbolic
		$(DEL) $(OBJS)
		$(DEL) $(DEST)
		$(DEL) *.err
