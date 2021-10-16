amikey
======
DOS tool for probing American Megatrends AMIKEY and compatible keyboard controllers.

Usage
-----
Run `AMIKEY.COM`. If an AMIKEY or compatible keyboard controller is found, its revision code will be displayed, as well as the copyright string stored in its firmware. The responses to various keyboard controller commands will also be saved to the `AMIKEY.BIN` file. Any errors in the probing process will be displayed on screen.

If running `AMIKEY.COM` hangs or reboots your system, run `JETKEY.COM` instead, which saves the `JETKEY.BIN` file after every test. This hang/reboot is caused by undefined behavior in the *Write Output Port* command on some AMIKEY clones such as the JETkey.

Inform us on Discord or by creating an issue in this repository (make sure to attach `AMIKEY.BIN` or `JETKEY.BIN`) if your system:

* reports any of the following revision codes: `7` `8` `A` `B` `D` `E` `L` `M` `N` `P` `R` `Z`; or
* has an AMI BIOS *and* the revision code does not match [the character at the end of the identification string displayed at POST](postkbc.png); or
* has an early AMI BIOS with `(C) Access Methods Inc.` copyright; or
* doesn't have a JETkey keyboard controller chip *and* running `AMIKEY.COM` froze or rebooted it.

Building
--------
* **Windows:** Run `build.bat AMIKEY` or `build.bat JETKEY` with [NewBasic](http://www.fysnet.net/newbasic.htm) present in `%PATH%`.
* **Other platforms:** This tool is only guaranteed to assemble and function properly with the (DOS- and Windows-only) NewBasic assembler, but feel free to try other assemblers.
