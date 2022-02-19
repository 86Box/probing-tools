biosdump
========
DOS tool for dumping the BIOS ROM on 386 or newer systems.

Usage
-----
Run `BIOSDUMP.EXE`. The following BIOS ROM regions will be dumped to corresponding files:

* `000C0000`-`000FFFFF` (low) to `000C0000.DMP`
* `00FE0000`-`00FFFFFF` (mid) to `00FE0000.DMP`
* `FFF80000`-`FFFFFFFF` (high) to `FFF80000.DMP`

Building
--------
* **Windows:** Run `wmake` from an OpenWatcom "Build Environment" command prompt.
* **Linux:** Run `wmake` with OpenWatcom tools present in `$PATH`.
