
"""Small PowerPC-style to AArch64 learning prototype."""

import re
import sys

REGISTER = re.compile(r"r(\d+)\Z")
INTEGER = re.compile(r"-?\d+\Z")


def register_number(token):
    match = REGISTER.fullmatch(token)
    if not match:
        raise ValueError(f"Invalid register: {token}")

    number = int(match.group(1))
    if not 0 <= number <= 30:
        raise ValueError(f"Register outside supported range r0-r30: {token}")
    return number


def translate(line):
    line = line.split("#", 1)[0].strip()
    if not line:
        return ""

    parts = re.split(r"[\s,]+", line)
    op, args = parts[0].lower(), parts[1:]

    if op == "addi" and len(args) == 3:
        rt, ra, immediate = args
        dest_num = register_number(rt)
        base_num = register_number(ra)

        if not INTEGER.fullmatch(immediate):
            raise ValueError("Immediate must be an integer")

        value = int(immediate)
        if not -4095 <= value <= 4095:
            raise ValueError("Immediate must be between -4095 and 4095")

        dest = f"x{dest_num}"

        # PowerPC addi treats base register r0 as literal zero.
        if base_num == 0:
            return f"    mov {dest}, #{value}"

        base = f"x{base_num}"
        if value < 0:
            return f"    sub {dest}, {base}, #{-value}"
        return f"    add {dest}, {base}, #{value}"

    if op == "add" and len(args) == 3:
        numbers = [register_number(arg) for arg in args]
        regs = [f"x{number}" for number in numbers]
        return "    add " + ", ".join(regs)

    raise ValueError(f"Unsupported instruction: {line}")


def main():
    if len(sys.argv) != 2:
        print("Usage: python prototype/ppc_to_aarch64.py FILE")
        return 2

    try:
        with open(sys.argv[1], encoding="utf-8") as source:
            for number, line in enumerate(source, start=1):
                try:
                    result = translate(line)
                    if result:
                        print(result)
                except ValueError as error:
                    print(f"Line {number}: ERROR: {error}", file=sys.stderr)
                    return 1
    except OSError as error:
        print(f"Could not read input file: {error}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
        
