import pathlib
import re
import unittest


PSOC_ROOT = pathlib.Path(__file__).resolve().parents[1]
PSOC_HW_C = PSOC_ROOT / "AcondicionamientoAnalogico.cydsn" / "psoc_hw.c"
PSOC_MAIN_C = PSOC_ROOT / "AcondicionamientoAnalogico.cydsn" / "main.c"


class SDChipSelectTest(unittest.TestCase):
    def test_sd_cs_is_detached_from_dsi_for_software_control(self):
        source = PSOC_HW_C.read_text(encoding="utf-8")
        function = re.search(
            r"static void sd_cs_detach_dsi\(void\)\s*\{(.*?)\n\}",
            source,
            re.DOTALL,
        )

        self.assertIsNotNone(function)
        body = function.group(1)
        self.assertIn("*byp     &= (uint8)~SD_CS_MASK;", body)
        self.assertNotIn("*byp     |= SD_CS_MASK;", body)

    def test_sd_initialization_runs_after_esp_handshake(self):
        source = PSOC_MAIN_C.read_text(encoding="utf-8")
        main_body = source[source.index("int main(void)") :]

        self.assertLess(main_body.index("wait_for_esp();"), main_body.index("sd_spi_init();"))


if __name__ == "__main__":
    unittest.main()
