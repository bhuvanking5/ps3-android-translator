"""Tiny PowerPC-style assembly to AArch64 translator.
Learning prototype only; not for PS3 game binaries.
"""

import re
import sys


def translate(line):
    line = line.split("#", 1)[0].strip()
    if not line:
        return ""

    parts = re.split(r"[\s,]+", line)
    op = parts[0].lower()
    args = parts[1:]

    if op == "addi" and len(args) == 3:
        rt, ra, imm = args
        if not rt.startswith("r") or not ra.startswith("r"):
            raise ValueError("Registers must use rN format")
        if not rt[1:].isdigit() or not ra[1:].isdigit():
            raise ValueError("Invalid register")
        if not re.fullmatch(r"-?\d+", imm):
            raise ValueError("Immediate must be an integer")

        dest = "x" + rt[1:]
        base = "xzr" if ra == "r0" else "x" + ra[1:]
        return f"    add {dest}, {base}, #{imm}"

    if op == "add" and len(args) == 3:
        rd, ra, rb = args
        regs = [rd, ra, rb]
        if any(not re.fullmatch(r"r\d+", r) for r in regs):
            raise ValueError("Registers must use rN format")
        return "    add " + ", ".join(
            "x" + r[1:] for r in regs
        )

    raise ValueError(f"Unsupported instruction: {line}")


def main():
    if len(sys.argv) != 2:
        print("Usage: python prototype/ppc_to_aarch64.py FILE")
        return

    with open(sys.argv[1], encoding="utf-8") as file:
        for line in file:
            try:
                result = translate(line)
                if result:
                    print(result)
            except ValueError as error:
                print(f"ERROR: {error}")


if __name__ == "__main__":
    main()
