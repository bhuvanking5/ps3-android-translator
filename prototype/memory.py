
"""Simple byte-addressable memory for instruction fetching."""


class Memory:
    def __init__(self, base_address, data):
        if not isinstance(base_address, int) or base_address < 0:
            raise ValueError("Invalid base address")
        if not isinstance(data, (bytes, bytearray)):
            raise ValueError("Memory data must be bytes")

        self.base_address = base_address
        self.data = bytearray(data)

    def read(self, address, size):
        offset = address - self.base_address

        if size < 0 or offset < 0:
            raise ValueError("Memory read out of bounds")

        end = offset + size
        if end > len(self.data):
            raise ValueError("Memory read out of bounds")

        return bytes(self.data[offset:end])

    def read_u32_be(self, address):
        if address % 4 != 0:
            raise ValueError("Instruction address must be 4-byte aligned")

        return int.from_bytes(self.read(address, 4), "big")
      
