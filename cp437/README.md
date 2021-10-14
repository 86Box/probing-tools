cp437
=====
Not a probing tool. Runs on the build host to convert UTF-8 source files to Code Page 437 for building DOS tools.

Usage
-----
```
cp437 infile [infile...]
- Converts UTF-8 input file(s) to CP437 output file(s) with .cp437 appended.
  The new file names are also printed to stdout.
```

Building
--------
This tool is automatically built as needed by the build scripts for other tools. Alternatively:

* **Windows:** Run `wmake` from an OpenWatcom "Build Environment" command prompt.
* **Linux:** Run `wmake` with OpenWatcom tools present in `$PATH`.
