pcireg
======
DOS tool for reading, writing and dumping PCI configuration space registers; scanning the PCI bus; and more.

Usage
-----
```
PCIREG -s [-d]
∟ Display all devices on the PCI bus. Specify -d to dump registers as well.

PCIREG -i [-8]
∟ Display BIOS IRQ steering table. Specify -8 to display as 86Box code.

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
* **Windows:** Run `build.bat` from an OpenWatcom "Build Environment" command prompt.
* **Linux:** Run `./build.sh` with OpenWatcom tools present in `$PATH`.
