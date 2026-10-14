
"""PowerPC branch target analysis for a future block translator."""

from prototype.ppc_decoder import decode_word


def analyze_branch(word, instruction_address):
    """Calculate the target of a supported PowerPC branch."""

    if (
        not isinstance(instruction_address, int)
        or isinstance(instruction_address, bool)
        or instruction_address < 0
        or instruction_address % 4 != 0
    ):
        raise ValueError(
            "Instruction address must be non-negative "
            "and 4-byte aligned"
        )

    instruction = decode_word(word)

    if instruction["mnemonic"] != "b":
        raise ValueError("Instruction is not an unconditional branch")

    displacement = instruction["displacement"]
    absolute = instruction["absolute"]

    target = (
        displacement
        if absolute
        else instruction_address + displacement
    )

    return {
        "mnemonic": "b",
        "instruction_address": instruction_address,
        "displacement": displacement,
        "target_address": target,
        "absolute": absolute,
        "link": instruction["link"],
    }
  
