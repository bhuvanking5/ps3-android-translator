
"""Translate small PowerPC blocks with correctly mapped local branches."""

from prototype.arm64_encoder import encode_program
from prototype.pipeline import translate_word
from prototype.ppc_decoder import decode_word


def translate_block(instructions, start_address=0):
    """Translate a block with local relative unconditional branches.

    Supported branches must be relative, non-linking, and target an
    instruction inside this block. Unsupported cases raise ValueError.
    """
    if not isinstance(instructions, (list, tuple)):
        raise ValueError("Instructions must be a list or tuple")

    if (
        not isinstance(start_address, int)
        or isinstance(start_address, bool)
        or start_address < 0
        or start_address % 4 != 0
    ):
        raise ValueError(
            "Start address must be non-negative and 4-byte aligned"
        )

    decoded = []
    for index, word in enumerate(instructions):
        address = start_address + index * 4
        try:
            decoded.append(decode_word(word))
        except ValueError as exc:
            raise ValueError(
                f"Decode failed at 0x{address:08X}: {exc}"
            ) from exc

    # Pass 1: calculate each guest instruction's output byte offset.
    guest_to_host = {}
    translated = []
    host_offset = 0

    for index, (word, ins) in enumerate(zip(instructions, decoded)):
        guest_address = start_address + index * 4
        guest_to_host[guest_address] = host_offset

        if ins["mnemonic"] == "b":
            if ins["absolute"]:
                raise ValueError(
                    f"Absolute branch unsupported at 0x{guest_address:08X}"
                )
            if ins["link"]:
                raise ValueError(
                    f"Link branch unsupported at 0x{guest_address:08X}"
                )

            # A branch will become exactly one ARM64 instruction.
            translated.append(None)
            host_offset += 4
        else:
            try:
                assembly = translate_word(word)
                machine_code = encode_program(assembly)
            except ValueError as exc:
                raise ValueError(
                    f"Translation failed at 0x{guest_address:08X}: {exc}"
                ) from exc

            translated.append(assembly)
            host_offset += len(machine_code)

    # Pass 2: resolve guest branch targets through the address map.
    output_assembly = []

    for index, (word, ins) in enumerate(zip(instructions, decoded)):
        guest_address = start_address + index * 4

        if ins["mnemonic"] != "b":
            output_assembly.extend(translated[index])
            continue

        guest_target = guest_address + ins["displacement"]

        if guest_target not in guest_to_host:
            raise ValueError(
                f"Branch target 0x{guest_target:08X} is outside "
                "the translated block or not an instruction boundary"
            )

        branch_host_offset = guest_to_host[guest_address]
        target_host_offset = guest_to_host[guest_target]
        host_displacement = target_host_offset - branch_host_offset

        output_assembly.append(f"b #{host_displacement}")

    return encode_program(output_assembly)
                                                    
