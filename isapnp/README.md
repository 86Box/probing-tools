isapnp
======
DOS tool for dumping the Resource Data ROM of ISA Plug and Play devices.

Usage
-----
Run `ISAPNP.EXE` on a system with ISA Plug and Play devices. All ISA PnP devices will be enumerated, and their Resource Data ROM contents will be saved to `iiiiiiiu.BIN` where `iiiiiii` is the device's PnP ID (`RTL8019` for example) and `u` is an unique identifier for devices with the same PnP ID (`A` for the first one, `B` for the second one and so on).

Building
--------
* **Windows:** Run `wmake` from an OpenWatcom "Build Environment" command prompt.
* **Linux:** Run `wmake` with OpenWatcom tools present in `$PATH`.
