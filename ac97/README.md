ac97
====
DOS tool for probing AC'97 audio controllers and codecs.

Usage
-----
Run `AC97.EXE` on a system with an Ensoniq AudioPCI (Creative Sound Blaster PCI), VIA or Intel AC'97 audio controller. On the first stage, audio controller information will be displayed on screen. On the second stage, AC'97 codec registers corresponding to optional features will be probed, and a register dump will be displayed and saved to `CODbbddf.BIN` where `bb`=bus, `dd`=device, `f`=function where the controller is located.

The optional `-s` parameter makes the second stage silent, only saving the codec register dump to file, which is useful if multiple audio controllers are being probed at once.

Building
--------
* **Windows:** Run `wmake` from an OpenWatcom "Build Environment" command prompt.
* **Linux:** Run `wmake` with OpenWatcom tools present in `$PATH`.
