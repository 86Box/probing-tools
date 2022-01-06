usblgoff
========
DOS tool for disabling USB legacy emulation to allow for running keyboard controller probing tools.

Usage
-----
Run `USBLGOFF.EXE` on a system with USB. The I/O traps set by onboard USB controllers and a handful of chipsets will be disabled, and the virtual keyboard controller created by the BIOS for USB input emulation should no longer interfere with KBC probing tools such as [amikey](../amikey) until the system is rebooted.

Building
--------
* **Windows:** Run `wmake` from an OpenWatcom "Build Environment" command prompt.
* **Linux:** Run `wmake` with OpenWatcom tools present in `$PATH`.
