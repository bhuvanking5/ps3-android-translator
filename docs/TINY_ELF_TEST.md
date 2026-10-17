# Tiny PowerPC64 ELF test fixture

This creates a tiny **unencrypted ELF64 big-endian PowerPC64 executable** with one read/execute load segment:

~~~sh
python tools/make_tiny_ps3_elf.py
~~~

It contains four PowerPC instructions: set GPR3 to 7, set GPR4 to 5, add them into GPR5 (12), then execute `blr`. In this prototype only, the runner treats this exact instruction as a top-level return to a synthetic host sentinel and reports a clean guest exit. This is not full link-register or subroutine support.

Run the fixture-generator checks with:

~~~sh
python -m unittest discover -s tests -p 'test_tiny_ps3_elf.py' -v
~~~

This file is a test fixture for the experimental native loader/runner. It is **not a PS3 game**. The Android built-in test can exercise this clean-return path, while selected-file support remains a limited raw-ELF interpreter path. PS3 SELF decryption, relocations/imports, OS services, SPU emulation, and RSX graphics are not implemented.

## More complete native integration regression

The native C regression test also loads a synthetic ELF program that performs a writable-data store and load, compares the loaded value, branches over a failure marker, and returns via the prototype top-level `blr` sentinel. This covers more of the loader/interpreter path than the four-instruction arithmetic fixture. It is still synthetic and is not a PS3 homebrew game.
