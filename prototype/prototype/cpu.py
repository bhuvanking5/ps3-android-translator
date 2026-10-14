
"""Reference execution engine for a small PowerPC subset."""

from prototype.ppc_decoder import decode_word

MASK64 = 0xFFFFFFFFFFFFFFFF


class CPUState:
    def __init__(self):
        self.gpr = [0] * 32
        self.pc = 0
        self.lr = 0

    def execute(self, word):
        """Execute one instruction and return its decoded operation."""
        ins = decode_word(word)
        op = ins["mnemonic"]
        old_pc = self.pc

        if op == "addi":
            ra = ins["ra"]
            base = 0 if ra == 0 else self.gpr[ra]
            self.gpr[ins["rt"]] = (
                base + ins["immediate"]
            ) & MASK64
            self.pc = old_pc + 4

        elif op == "add":
            self.gpr[ins["rt"]] = (
                self.gpr[ins["ra"]]
                + self.gpr[ins["rb"]]
            ) & MASK64
            self.pc = old_pc + 4

        elif op == "b":
            if ins["link"]:
                self.lr = old_pc + 4

            if ins["absolute"]:
                self.pc = ins["displacement"] & MASK64
            else:
                self.pc = (
                    old_pc + ins["displacement"]
                ) & MASK64

        else:
            raise ValueError(
                f"Unsupported execution operation: {op}"
            )

        return ins
              
