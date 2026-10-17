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


def apply_rela_records(
    memory: bytearray,
    memory_base: int,
    relocations: list[dict],
    symbols: dict[int, dict],
    *,
    elf_type: str,
    load_bias: int = 0,
    resolve_import=None,
) -> int:
    """Apply a complete supported relocation list atomically.

    Relocation offset and defined st_value fields are ELF virtual addresses.
    For ET_DYN, load_bias is added to target, place, and defined symbol
    addresses. ET_EXEC requires zero load bias. Undefined nonzero symbols must
    be supplied by resolve_import(name, symbol_record); no fake address is
    supplied. Writes commit only if every relocation succeeds.
    """
    if not isinstance(memory, bytearray):
        raise RelocationError("relocation memory must be a bytearray")
    if not isinstance(memory_base, int) or memory_base < 0:
        raise RelocationError("memory base must be non-negative")
    if not isinstance(load_bias, int) or load_bias < 0:
        raise RelocationError("load bias must be non-negative")
    if elf_type not in ("ET_EXEC", "ET_DYN"):
        raise RelocationError("only ET_EXEC and ET_DYN are supported")
    if elf_type == "ET_EXEC" and load_bias != 0:
        raise RelocationError("ET_EXEC must use zero load bias")
    if resolve_import is not None and not callable(resolve_import):
        raise RelocationError("import resolver must be callable")

    staged = bytearray(memory)
    applied = 0
    for relocation in relocations:
        try:
            target_vaddr = relocation["offset"]
            relocation_type = relocation["type"]
            addend = relocation["addend"]
            symbol_index = relocation["symbol_index"]
        except (KeyError, TypeError) as exc:
            raise RelocationError("malformed relocation record") from exc
        if not all(isinstance(value, int) for value in
                   (target_vaddr, relocation_type, addend, symbol_index)):
            raise RelocationError("relocation fields must be integers")
        if target_vaddr < 0 or symbol_index < 0:
            raise RelocationError("negative relocation address or symbol index")
        place = target_vaddr + (load_bias if elf_type == "ET_DYN" else 0)
        if place > (1 << 64) - 1:
            raise RelocationError("relocation place address overflows 64 bits")
        offset = place - memory_base
        if offset < 0:
            raise RelocationError("relocation target precedes mapped memory")

        if symbol_index == 0:
            symbol_address = 0
        else:
            symbol = symbols.get(symbol_index)
            if not isinstance(symbol, dict):
                raise RelocationError(f"missing dynamic symbol {symbol_index}")
            if symbol.get("undefined_import", symbol.get("section_index") == 0):
                name = symbol.get("name", "")
                if resolve_import is None:
                    raise RelocationError(f"unresolved import: {name or symbol_index}")
                symbol_address = resolve_import(name, symbol)
                if not isinstance(symbol_address, int) or symbol_address < 0:
                    raise RelocationError(f"resolver returned invalid address for {name or symbol_index}")
            else:
                symbol_address = symbol.get("value")
                if not isinstance(symbol_address, int) or symbol_address < 0:
                    raise RelocationError(f"invalid value for dynamic symbol {symbol_index}")
                if elf_type == "ET_DYN":
                    symbol_address += load_bias
                if symbol_address > (1 << 64) - 1:
                    raise RelocationError("symbol address overflows 64 bits")

        apply_rela(staged, offset, relocation_type, symbol_address, addend, place)
        applied += 1

    memory[:] = staged
    return applied
