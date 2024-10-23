pcireg
======
DOS, UEFI and Linux tool for reading, writing and dumping PCI configuration space registers; scanning the PCI bus; and more.

Usage
-----
```
PCIREG -s [-d]
∟ Display all devices on the PCI bus. Specify -d to dump registers as well.

PCIREG -t [-8]
∟ Display BIOS IRQ steering table. Specify -8 to display as 86Box code.
  (Not available in UEFI version)

PCIREG -i [bus] device [function]
∟ Show information about the specified device.

PCIREG -r [bus] device [function] register
∟ Read the specified register.

PCIREG -w [bus] device [function] register value
∟ Write byte, word or dword to the specified register.

PCIREG {-d|-dw|-dl} [bus] device [function [register]]
∟ Dump registers as bytes (-d), words (-dw) or dwords (-dl). Optionally
  specify the register to start from (requires bus to be specified as well).

All numeric parameters should be specified in hexadecimal (without 0x prefix).
{bus device function register} can be substituted for a single port CF8h dword.
Register dumps are saved to PCIbbddf.BIN where bb=bus, dd=device, f=function.
```

Building
--------
### DOS target

* **Windows:** Run `wmake` from an OpenWatcom "Build Environment" command prompt.
* **Linux:** Run `wmake` with OpenWatcom tools present in `$PATH`.

### UEFI target

* **Linux:** Run `make -f Makefile.uefi ARCH=x86_64` with a GCC toolchain installed.
  * Note that 32-bit UEFI targets are not supported yet.

### Linux target

* **Linux:** Run `make -f Makefile.gcc` with a GCC toolchain and development files for `libpci` installed.

### PCI ID database

* Run `python3 pciids.py` to update the `PCIIDS.BIN` file.
  * The latest version of `pci.ids` is automatically downloaded and used to update the database.
