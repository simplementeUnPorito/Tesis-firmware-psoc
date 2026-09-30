from pathlib import Path
import unittest


class I2CLinkSpeedTest(unittest.TestCase):
    def test_i2c_link_uses_measured_safe_divider(self):
        source = (Path(__file__).parents[1] / "AcondicionamientoAnalogico.cydsn" / "psoc_hw.c").read_text()
        start = source.index("void psoc_link_start(void)")
        body = source[start : source.index("void psoc_link_wait_idle", start)]

        self.assertIn("LINK_I2C(Start)();", body)
        self.assertIn("I2C_CLKDIV1_REG = 7u;", body)
        self.assertIn("I2C_CLKDIV2_REG = 0u;", body)
        self.assertLess(body.index("LINK_I2C(Start)();"), body.index("I2C_CLKDIV1_REG = 7u;"))


if __name__ == "__main__":
    unittest.main()
