# Tiny PowerPC64 ELF test fixture

This creates a tiny **unencrypted ELF64 big-endian PowerPC64 executable** with one read/execute load segment:

~~~sh
python tools/make_tiny_ps3_elf.py
~~~

It contains four PowerPC instructions: set GPR3 to 7, set GPR4 to 5, add them into GPR5 (12), then branch to itself. The loop makes it suitable for a bounded-runner test, which should stop at its configured instruction limit.

Run the fixture-generator checks with:

~~~sh
python -m unittest tests/test_tiny_ps3_elf.py -v
~~~

This file is a test fixture for the experimental native loader/runner. It is **not a PS3 game**, and the current Android UI does not yet execute a selected ELF end-to-end. PS3 SELF decryption, relocations/imports, OS services, SPU emulation, and RSX graphics are not implemented.