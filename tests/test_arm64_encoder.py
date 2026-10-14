
    def test_forward_branch(self):
        self.assertEqual(
            encode_instruction("b #16"),
            0x14000004,
        )

    def test_backward_branch(self):
        self.assertEqual(
            encode_instruction("b #-4"),
            0x17FFFFFF,
        )

    def test_branch_bytes(self):
        self.assertEqual(
            encode_bytes("b #16"),
            bytes.fromhex("04000014"),
        )

    def test_reject_unaligned_branch(self):
        with self.assertRaises(ValueError):
            encode_instruction("b #2")

    def test_reject_out_of_range_branch(self):
        with self.assertRaises(ValueError):
            encode_instruction("b #134217728")
            
