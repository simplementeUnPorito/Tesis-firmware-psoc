import math
import pathlib
import re
import unittest


PSOC_ROOT = pathlib.Path(__file__).resolve().parents[1]
CALIBRATION_C = PSOC_ROOT / "AcondicionamientoAnalogico.cydsn" / "calibration.c"
ADC_H = PSOC_ROOT / "AcondicionamientoAnalogico.cydsn" / "psoc_adc.h"
FIR_H = PSOC_ROOT / "AcondicionamientoAnalogico.cydsn" / "FIR_calibration.h"


def macro_u(source: str, name: str) -> int:
    match = re.search(rf"^#define\s+{name}\s+(\d+)u", source, re.MULTILINE)
    if not match:
        raise AssertionError(f"No se encontro {name}")
    return int(match.group(1))


class I2CCalibrationSamplingTest(unittest.TestCase):
    def test_calibration_uses_filtered_stream_above_300_hz(self):
        calibration = CALIBRATION_C.read_text(encoding="utf-8")
        adc = ADC_H.read_text(encoding="utf-8")

        self.assertRegex(calibration, r"#define\s+CAL_PI_USE_DIRECT_ADC\s+0u")
        self.assertGreaterEqual(macro_u(adc, "PSOC_ADC_NATIVE_FS_HZ"), 300)
        self.assertIn("return psoc_adc_take_isr_filtered_sample(sample);", calibration)

    def test_calibration_fir_rejects_50_hz(self):
        fir = FIR_H.read_text(encoding="utf-8")
        raw = [int(value, 16) for value in re.findall(r"0x([0-9A-Fa-f]{2})u", fir)]
        self.assertEqual(len(raw), 128 * 4)

        taps = []
        for offset in range(0, len(raw), 4):
            value = raw[offset] | raw[offset + 1] << 8 | raw[offset + 2] << 16
            if value & 0x800000:
                value -= 1 << 24
            taps.append(value / float(1 << 23))

        fs_hz = macro_u(ADC_H.read_text(encoding="utf-8"), "PSOC_ADC_NATIVE_FS_HZ")
        dc_gain = abs(sum(taps))
        response_50_hz = abs(sum(
            tap * complex(math.cos(-2 * math.pi * 50 * n / fs_hz),
                          math.sin(-2 * math.pi * 50 * n / fs_hz))
            for n, tap in enumerate(taps)
        ))
        attenuation_db = 20 * math.log10(response_50_hz / dc_gain)
        self.assertLessEqual(attenuation_db, -40.0)


if __name__ == "__main__":
    unittest.main()
