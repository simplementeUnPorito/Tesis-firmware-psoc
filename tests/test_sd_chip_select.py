import pathlib
import re
import unittest


PSOC_ROOT = pathlib.Path(__file__).resolve().parents[1]
PSOC_HW_C = PSOC_ROOT / "AcondicionamientoAnalogico.cydsn" / "psoc_hw.c"


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

    def test_sd_miso_uses_lvttl_threshold_for_3v3_card_output(self):
        source = PSOC_HW_C.read_text(encoding="utf-8")
        function = re.search(
            r"static void sd_miso_use_lvttl\(void\)\s*\{(.*?)\n\}",
            source,
            re.DOTALL,
        )

        self.assertIn("#define SD_GPIO_VTRIP_LVTTL", source)
        self.assertIsNotNone(function)
        self.assertIn("*ctl |= SD_GPIO_VTRIP_LVTTL;", function.group(1))
        self.assertIn(
            "CyPins_SetPinDriveMode(SPIp_MISO, CY_PINS_DM_RES_UP);",
            function.group(1),
        )
        self.assertRegex(
            source,
            r"if \(!g_sd_spi_started\)\s*\{\s*sd_miso_use_lvttl\(\);",
        )


if __name__ == "__main__":
    unittest.main()
