
import os
import sys
import unittest

sys.path.insert(
    0,
    os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
)

from prototype.register_map import map_gpr, is_directly_mappable


class RegisterMapTests(unittest.TestCase):

    def test_maps_low_registers(self):
        self.assertEqual(map_gpr(0), "x0")
        self.assertEqual(map_gpr(30), "x30")

    def test_r31_uses_spill_slot(self):
        self.assertEqual(map_gpr(31), "spill_r31")
        self.assertFalse(is_directly_mappable(31))

    def test_direct_mapping_for_r30(self):
        self.assertTrue(is_directly_mappable(30))

    def test_rejects_invalid_registers(self):
        for value in (-1, 32, True, "3"):
            with self.subTest(value=value):
                with self.assertRaises(ValueError):
                    map_gpr(value)


if __name__ == "__main__":
    unittest.main()
  
