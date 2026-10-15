
"""PowerPC GPR to ARM64 register mapping.

This is a mapping policy for the prototype, not a complete CPU state
implementation. PowerPC r0-r31 map to ARM64 x0-x30 plus a reserved
spill slot for r31; x31 is never treated as a normal GPR.
"""

PPC_GPR_COUNT = 32
ARM64_GPR_COUNT = 31

# r0-r30 use x0-x30. r31 must be stored elsewhere until the runtime
# implements a real spill/load strategy for it.
PPC_TO_ARM64 = {
    ppc_reg: ppc_reg
    for ppc_reg in range(ARM64_GPR_COUNT)
}

PPC_SPILL_REGISTERS = {
    31: "spill_r31",
}


def map_gpr(ppc_reg):
    """Return the ARM64 register name or spill-slot name for a PPC GPR."""
    if (
        not isinstance(ppc_reg, int)
        or isinstance(ppc_reg, bool)
        or not 0 <= ppc_reg < PPC_GPR_COUNT
    ):
        raise ValueError("PowerPC GPR index must be between 0 and 31")

    if ppc_reg in PPC_SPILL_REGISTERS:
        return PPC_SPILL_REGISTERS[ppc_reg]

    return f"x{PPC_TO_ARM64[ppc_reg]}"


def is_directly_mappable(ppc_reg):
    """Whether the GPR currently has a direct ARM64 register mapping."""
    map_gpr(ppc_reg)
    return ppc_reg in PPC_TO_ARM64
  
