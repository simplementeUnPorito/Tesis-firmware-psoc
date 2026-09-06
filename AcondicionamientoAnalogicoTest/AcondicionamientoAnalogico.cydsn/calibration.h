/* Este archivo vive en el proyecto de campo. Aca solo se lo incluye.
 *
 * Desde el 2026-09-05 el proyecto de autotest no guarda copias: cada fuente
 * compartida existe UNA sola vez, en AcondicionamientoAnalogico.cydsn. Antes
 * eran copias mantenidas a mano y la calibracion llego a divergir en silencio.
 * Ver el comentario largo en calibration.c.
 */
#include "../../AcondicionamientoAnalogico.cydsn/calibration.h"

/* ==========================================================================
 * Autotest: lo unico que este proyecto agrega a la calibracion.
 * --------------------------------------------------------------------------
 * El AMux y el lector directo del ADC son static en calibration.c. El
 * secuenciador de autotest vive en psoc_selftest.h (incluido desde main.c) y
 * necesita esas primitivas para barrer cada IDAC contra cada tap del AMux.
 * Se exponen aca en vez de duplicar el manejo del AMux: la exclusion mutua
 * entre canales de senal tiene que seguir viviendo en un solo lugar.
 * ========================================================================== */

int32 psoc_selftest_counts_to_uv(int32 counts);
uint8 psoc_selftest_stage_count(void);
uint8 psoc_selftest_stage_channel(uint8 stage, uint8 *out_channel);
uint8 psoc_selftest_amux_channel_count(void);
/* Codigos CON SIGNO en las tres: negativo = referencia por debajo de Vref. */
uint8 psoc_selftest_write_stage_dac(uint8 stage, int16 code);
uint8 psoc_selftest_current_stage_dac(uint8 stage, int16 *out_code);
uint8 psoc_selftest_stage_result(uint8 stage, int16 *out_dac, int32 *out_meas, uint8 *out_ok);
void  psoc_selftest_select_channel(uint8 channel, uint8 with_cap);
void  psoc_selftest_restore(void);
uint8 psoc_selftest_measure_dc(uint8 channel, uint16 settle_ms, uint16 n,
                               uint8 with_cap, int32 *out_mean, int32 *out_pp);
uint8 psoc_selftest_measure_series(uint8 channel, uint16 settle_ms, uint16 n,
                                   uint8 with_cap, uint16 tone_hz,
                                   int32 *out_mean, int32 *out_rms,
                                   int32 *out_pp, int32 *out_tone);
