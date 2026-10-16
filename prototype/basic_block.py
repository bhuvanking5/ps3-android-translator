"""Build a validated basic-block plan for a bounded PowerPC instruction sequence.

This module does not execute code. It splits instruction runs around supported
relative unconditional branches and records explicit control-flow edges.
Unsupported control flow is rejected rather than silently treated as data.
"""

from prototype.ppc_decoder import decode_word
from prototype.pipeline import translate_word

MAX_BLOCK_INSTRUCTIONS = 256


def build_basic_blocks(instructions, start_address=0):
    """Return a basic-block plan for a small instruction sequence.

    Supported control flow is relative, unlinked, unconditional b whose target
    is another instruction in this same sequence. Conditional branches,
    absolute branches, linked branches, and out-of-range targets are rejected.
    """
    if not isinstance(instructions, (list, tuple)):
        raise ValueError("Instructions must be a list or tuple")
    if not instructions:
        raise ValueError("Instructions must not be empty")
    if len(instructions) > MAX_BLOCK_INSTRUCTIONS:
        raise ValueError("Basic-block input exceeds 256 instructions")
    if (
        not isinstance(start_address, int)
        or isinstance(start_address, bool)
        or start_address < 0
        or start_address % 4 != 0
    ):
        raise ValueError("Start address must be non-negative and 4-byte aligned")

    addresses = [start_address + index * 4 for index in range(len(instructions))]
    address_to_index = {address: index for index, address in enumerate(addresses)}
    decoded = []
    branch_targets = {}

    for index, word in enumerate(instructions):
        address = addresses[index]
        try:
            ins = decode_word(word)
        except ValueError as exc:
            raise ValueError(f"Decode failed at 0x{address:08X}: {exc}") from exc

        mnemonic = ins["mnemonic"]
        if mnemonic == "bc":
            raise ValueError(f"Unsupported conditional branch at 0x{address:08X}")
        if mnemonic == "b":
            if ins["absolute"]:
                raise ValueError(f"Absolute branch at 0x{address:08X} is not supported")
            if ins["link"]:
                raise ValueError(f"Linked branch at 0x{address:08X} is not supported")
            displacement = ins["displacement"]
            if displacement % 4:
                raise ValueError(f"Unaligned branch displacement at 0x{address:08X}")
            target = address + displacement
            if target not in address_to_index:
                raise ValueError(
                    f"Branch at 0x{address:08X} targets 0x{target:08X}, "
                    "outside this instruction sequence"
                )
            branch_targets[index] = target

        decoded.append(ins)

    leaders = {0}
    for index, target in branch_targets.items():
        leaders.add(address_to_index[target])
        if index + 1 < len(instructions):
            leaders.add(index + 1)

    leader_indices = sorted(leaders)
    blocks = []
    index_to_block_start = {}
    for leader_pos, first_index in enumerate(leader_indices):
        end_index = (
            leader_indices[leader_pos + 1]
            if leader_pos + 1 < len(leader_indices)
            else len(instructions)
        )
        block_addresses = addresses[first_index:end_index]
        block_words = instructions[first_index:end_index]
        block_arm64 = []
        for offset, word in enumerate(block_words):
            ins_address = block_addresses[offset]
            if decoded[first_index + offset]["mnemonic"] == "b":
                continue
            try:
                block_arm64.extend(translate_word(word))
            except ValueError as exc:
                raise ValueError(
                    f"Translation failed at 0x{ins_address:08X}: {exc}"
                ) from exc
        block = {
            "start_address": block_addresses[0],
            "instruction_addresses": block_addresses,
            "arm64": block_arm64,
            "terminator": "branch" if end_index - 1 in branch_targets else "fallthrough",
            "successors": [],
        }
        blocks.append(block)
        for ins_index in range(first_index, end_index):
            index_to_block_start[ins_index] = block_addresses[0]

    for block_index, block in enumerate(blocks):
        first_index = address_to_index[block["start_address"]]
        last_index = first_index + len(block["instruction_addresses"]) - 1
        if last_index in branch_targets:
            target_index = address_to_index[branch_targets[last_index]]
            block["successors"] = [index_to_block_start[target_index]]
        elif block_index + 1 < len(blocks):
            block["successors"] = [blocks[block_index + 1]["start_address"]]

    return blocks
