@echo off
::
:: 86Box	A hypervisor and IBM PC system emulator that specializes in
::		running old operating systems and software designed for IBM
::		PC systems and compatibles from 1981 through fairly recent
::		system designs based on the PCI bus.
::
::		This file is part of the 86Box Probing Tools distribution.
::
::		Universal Windows build script for Watcom C-based DOS tools.
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

:: Check for cp437 tool and build it if necessary.
if not exist ..\cp437\cp437.exe (
    echo *** Building cp437 conversion tool for your host system...
    pushd ..\cp437
    call build.bat
    popd
    if errorlevel 1 exit /b %errorlevel%
)

:: Convert source file to CP437.
echo *** Converting source to CP437...
..\cp437\cp437.exe %* > cp437.tmp
if errorlevel 1 (
    echo *** Conversion failed.
    exit /b 2
)

:: Generate output file name.
set srcfile=%1
set destfile=%srcfile:~0,-2%.exe
setlocal enabledelayedexpansion
for %%i in (A B C D E F G H I J K L M N O P Q R S T U V W X Y Z) do set "destfile=!destfile:%%i=%%i!"

:: Call compiler.
echo *** Building...
set srcfiles=
for /f "delims=" %%i in (cp437.tmp) do set srcfiles=!srcfiles! %%i
wcl -bcl=dos -i=..\lib -fe="!destfile!" %srcfiles%

:: Check for success.
if errorlevel 1 (
    echo *** Build failed.
    exit /b 3
) else (
    echo *** Build successful.
)
