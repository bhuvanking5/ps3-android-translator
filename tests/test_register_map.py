import os
import sys
import unittest

sys.path.insert(
    0, os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
)

from prototype.register_map import map_gpr, is_directly_mappable


class RegisterMapTests(unittest.TestCase):

    def test_r0_maps_to_x0(self):
        self.assertEqual(map_gpr(0), "x0")
        self.assertTrue(is_directly_mappable(0))

    def test_r30_maps_to_x30(self):
        self.assertEqual(map_gpr(30), "x30")
        self.assertTrue(is_directly_mappable(30))

    def test_r31_is_spill_slot_not_x31(self):
        self.assertEqual(map_gpr(31), "spill_r31")
        self.assertFalse(is_directly_mappable(31))

    def test_invalid_register_indices_are_rejected(self):
        for value in (-1, 32, 1.5, "3", None, True, False):
            with self.subTest(value=value):
                with self.assertRaises(ValueError):
                    map_gpr(value)

    def test_direct_mapping_check_rejects_invalid_index(self):
        with self.assertRaises(ValueError):
            is_directly_mappable(32)


if __name__ == "__main__":
    unittest.main()
