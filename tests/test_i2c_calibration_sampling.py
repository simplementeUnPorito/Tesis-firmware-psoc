import math
import pathlib
import re
import unittest


PSOC_ROOT = pathlib.Path(__file__).resolve().parents[1]
CALIBRATION_C = PSOC_ROOT / "AcondicionamientoAnalogico.cydsn" / "calibration.c"
CONTROL_RUNTIME = PSOC_ROOT / "AcondicionamientoAnalogico.cydsn" / "control_runtime.inc"
MAIN_C = PSOC_ROOT / "AcondicionamientoAnalogico.cydsn" / "main.c"
ADC_H = PSOC_ROOT / "AcondicionamientoAnalogico.cydsn" / "psoc_adc.h"
CONTROL_CONFIG_C = PSOC_ROOT / "AcondicionamientoAnalogico.cydsn" / "control_config.c"


def macro_u(source: str, name: str) -> int:
    match = re.search(rf"^#define\s+{name}\s+(\d+)u", source, re.MULTILINE)
    if not match:
        raise AssertionError(f"No se encontro {name}")
    return int(match.group(1))


class I2CCalibrationSamplingTest(unittest.TestCase):
    def test_ctl_learn_receives_samples_above_300_hz(self):
        calibration = CALIBRATION_C.read_text(encoding="utf-8")
        control = CONTROL_RUNTIME.read_text(encoding="utf-8")
        main = MAIN_C.read_text(encoding="utf-8")
        adc = ADC_H.read_text(encoding="utf-8")

        # `cal` conserva su lector directo legado. El aprendizaje operativo
        # (`ctl learn`) recibe el ADC continuo por el respaldo a 1 kHz.
        self.assertRegex(calibration, r"#define\s+CAL_PI_USE_DIRECT_ADC\s+1u")
        self.assertGreaterEqual(macro_u(main, "CONTROL_POLL_HZ"), 300)
        self.assertGreaterEqual(macro_u(adc, "PSOC_ADC_NATIVE_FS_HZ"),
                                macro_u(main, "CONTROL_POLL_HZ"))
        self.assertIn("dma_route_select(0u);", control)
        self.assertIn("ctl_select(0);ADC_StartConvert();", control)
        self.assertIn("control_on_filtered_sample(sample);", main)
        self.assertIn("control_adc_poll_service();", main)
        poll = re.search(r"control_adc_poll_service\(void\)(.*?)\n}", main, re.DOTALL)
        self.assertIsNotNone(poll)
        self.assertNotIn("ADC_IsEndConversion", poll.group(1))

    def test_runtime_average_rejects_50_hz(self):
        config = CONTROL_CONFIG_C.read_text(encoding="utf-8")
        defaults = re.search(r"defaults\[CP_COUNT\]\s*=\s*\{(.*?)\};", config, re.DOTALL)
        self.assertIsNotNone(defaults)
        body = re.sub(r"/\*.*?\*/", "", defaults.group(1), flags=re.DOTALL)
        values = [int(value) for value in re.findall(r"(?<![A-Za-z_])-?\d+", body)]
        average = values[2]
        self.assertEqual(average, 20)
        fs_hz = macro_u(MAIN_C.read_text(encoding="utf-8"), "CONTROL_POLL_HZ")
        omega = 2 * math.pi * 50 / fs_hz
        response_50_hz = abs(math.sin(average * omega / 2) /
                             (average * math.sin(omega / 2)))
        attenuation_db = 20 * math.log10(response_50_hz)
        self.assertLessEqual(attenuation_db, -40.0)


if __name__ == "__main__":
    unittest.main()
