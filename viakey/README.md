viakey
======
DOS tool for probing VIA VT82C42N keyboard controllers.

Usage
-----
Run `VIAKEY.COM`. If a VT82C42N keyboard controller is found (either standalone or integrated to a VIA southbridge), its revision code will be displayed. Any errors in the probing process will be displayed on screen.

Building
--------
* **Windows:** Run `build.bat VIAKEY` with [NewBasic](http://www.fysnet.net/newbasic.htm) present in `%PATH%`.
* **Other platforms:** This tool is only guaranteed to assemble and function properly with the (DOS- and Windows-only) NewBasic assembler, but feel free to try other assemblers.
