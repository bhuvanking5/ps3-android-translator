"""Reference execution engine for a small PowerPC subset."""

from prototype.ppc_decoder import decode_word

MASK64 = 0xFFFFFFFFFFFFFFFF
MASK32 = 0xFFFFFFFF


class CPUState:
    def __init__(self):
        self.gpr = [0] * 32
        self.pc = 0
        self.lr = 0
        self.ctr = 0
        # PowerPC condition register as 32 individually addressable bits.
        self.cr = 0

    def execute(self, word):
        """Execute one supported instruction and return its decoded operation."""
        ins = decode_word(word)
        op = ins["mnemonic"]
        old_pc = self.pc

        if op == "addi":
            ra = ins["ra"]
            base = 0 if ra == 0 else self.gpr[ra]
            self.gpr[ins["rt"]] = (base + ins["immediate"]) & MASK64
            self.pc = old_pc + 4

        elif op == "add":
            self.gpr[ins["rt"]] = (
                self.gpr[ins["ra"]] + self.gpr[ins["rb"]]
            ) & MASK64
            self.pc = old_pc + 4

        elif op == "b":
            if ins["link"]:
                self.lr = old_pc + 4
            self.pc = (
                ins["displacement"] if ins["absolute"]
                else old_pc + ins["displacement"]
            ) & MASK64

        elif op == "bc":
            bo = ins["bo"]
            bi = ins["bi"]
            # BO bit 0 controls whether CTR is ignored; BO bit 1
            # selects CTR==0 versus CTR!=0. BO bit 2 ignores CR;
            # BO bit 3 selects the expected CR bit value.
            if not (bo & 0b10000):
                self.ctr = (self.ctr - 1) & MASK32
                ctr_ok = (self.ctr == 0) == bool(bo & 0b01000)
            else:
                ctr_ok = True

            if not (bo & 0b00100):
                cr_bit = (self.cr >> (31 - bi)) & 1
                cr_ok = cr_bit == int(bool(bo & 0b00010))
            else:
                cr_ok = True

            if ins["link"]:
                self.lr = old_pc + 4
            if ctr_ok and cr_ok:
                self.pc = (
                    ins["displacement"] if ins["absolute"]
                    else old_pc + ins["displacement"]
                ) & MASK64
            else:
                self.pc = old_pc + 4

        else:
            raise ValueError(f"Unsupported execution operation: {op}")

        return ins
