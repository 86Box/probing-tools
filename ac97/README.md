ac97
====
DOS and Linux tool for probing AC'97 audio controllers and codecs.

Usage
-----
Run `ac97` on a system with one or more of the following audio controllers. **On Linux, make sure all `snd_*` modules are unloaded** to avoid external interference.

* Ensoniq AudioPCI or Sound Blaster PCI (except ES1370/5507)
* Sound Blaster Live! or Audigy
* Intel, Nvidia or VIA AC'97

On the first stage, audio controller information will be displayed on screen. On the second stage, AC'97 codec registers corresponding to optional features will be probed, and a register dump will be displayed and saved to `CODbbddf.BIN` where `bb`=bus, `dd`=device, `f`=function where the controller is located.

The optional `-s` parameter makes the second stage silent, only saving the codec register dump to file, which is useful if multiple audio controllers are being probed at once.

Building
--------
### DOS target

* **Windows:** Run `wmake` from an OpenWatcom "Build Environment" command prompt.
* **Linux:** Run `wmake` with OpenWatcom tools present in `$PATH`.

### Linux target

* **Linux:** Run `make -f Makefile.gcc` with a GCC toolchain and development files for `libpci` installed.
