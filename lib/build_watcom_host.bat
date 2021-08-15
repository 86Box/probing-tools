@echo off
::
:: 86Box	A hypervisor and IBM PC system emulator that specializes in
::		running old operating systems and software designed for IBM
::		PC systems and compatibles from 1981 through fairly recent
::		system designs based on the PCI bus.
::
::		This file is part of the 86Box Probing Tools distribution.
::
::		Universal Windows build script for Watcom C-based host tools.
::
::
::
:: Authors:	RichardG, <richardg867@gmail.com>
::
::		Copyright 2021 RichardG.
::

:: Check for Watcom environment.
if "x%WATCOM%" == "x" (
    echo *** WATCOM environment variable not set. Make sure you're on an OpenWatcom
    echo     "Build Environment" command prompt.
    exit /b 1
)

:: Generate output file name.
set srcfile=%1
set destfile=%srcfile:~0,-2%.exe

:: Call compiler.
echo *** Building...
wcl386 -bcl=nt -fo="%destfile%" %*

:: Check for success.
if errorlevel 1 (
    echo *** Build failed.
    exit /b 4
) else (
    echo *** Build successful.
)
