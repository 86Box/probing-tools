@echo off
: 
: 86Box		A hypervisor and IBM PC system emulator that specializes in
: 		running old operating systems and software designed for IBM
: 		PC systems and compatibles from 1981 through fairly recent
: 		system designs based on the PCI bus.
: 
: 		This file is part of the 86Box Probing Tools distribution.
: 
: 		Build script for compiling assembly tools with NewBasic.
: 
: 
: 
: Authors:	RichardG, <richardg867@gmail.com>
: 
: 		Copyright 2021 RichardG.
: 

: Check for NBASM presence.
set NBASMBIN=nbasm32
%NBASMBIN% /v > NUL 2> NUL
if not errorlevel 100 goto build
set NBASMBIN=nbasm64
%NBASMBIN% /v > NUL 2> NUL
if not errorlevel 100 goto build
set NBASMBIN=nbasm16
%NBASMBIN% /v > NUL 2> NUL
if not errorlevel 100 goto build

: No NBASM found.
echo NewBasic not found. Make sure NBASM16/32/64 is present in PATH.
exit /b 1

:build
%NBASMBIN% %1.ASM %1.COM
