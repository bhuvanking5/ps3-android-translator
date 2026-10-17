"""Apply a deliberately tiny, bounds-checked subset of PPC64 ELF RELA relocations.

This is a host-side research helper, not wired into the Android loader. Callers
must supply an already-resolved symbol address; undefined imports must be
rejected by the caller until a real PS3 runtime resolver exists.
"""
from __future__ import annotations

R_PPC64_ADDR32 = 1
R_PPC64_ADDR64 = 38
R_PPC64_REL64 = 44


class RelocationError(ValueError):
    """Unsupported relocation or invalid relocation target/value."""


def apply_rela(
    memory: bytearray,
    offset: int,
    relocation_type: int,
    symbol_address: int,
    addend: int,
    place_address: int,
) -> None:
    """Apply one supported RELA relocation to a bytearray in big-endian order.

    Supported forms:
      * R_PPC64_ADDR32: S + A, unsigned 32-bit
      * R_PPC64_ADDR64: S + A, unsigned 64-bit
      * R_PPC64_REL64:  S + A - P, signed 64-bit

    The bytearray is changed only after all bounds and value checks succeed.
    """
    if not isinstance(memory, bytearray):
        raise RelocationError("relocation memory must be a bytearray")
    if not isinstance(offset, int) or offset < 0:
        raise RelocationError("relocation offset must be non-negative")
    if not all(isinstance(value, int) for value in (symbol_address, addend, place_address)):
        raise RelocationError("relocation values must be integers")

    if relocation_type == R_PPC64_ADDR32:
        width, value, minimum, maximum = 4, symbol_address + addend, 0, (1 << 32) - 1
        signed = False
    elif relocation_type == R_PPC64_ADDR64:
        width, value, minimum, maximum = 8, symbol_address + addend, 0, (1 << 64) - 1
        signed = False
    elif relocation_type == R_PPC64_REL64:
        width, value, minimum, maximum = 8, symbol_address + addend - place_address, -(1 << 63), (1 << 63) - 1
        signed = True
    else:
        raise RelocationError(f"unsupported PPC64 relocation type: {relocation_type}")

    if offset > len(memory) or width > len(memory) - offset:
        raise RelocationError("relocation target is outside mapped memory")
    if value < minimum or value > maximum:
        raise RelocationError("relocation result is out of range")

    memory[offset:offset + width] = value.to_bytes(width, "big", signed=signed)
