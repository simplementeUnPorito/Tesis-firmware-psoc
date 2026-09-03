#ifndef CALIBRATION_H
#define CALIBRATION_H

#include "project.h"
#include "psoc_hw.h"

#define PSOC_CAL_MAX_STAGES 4u

/* Codigo CON SIGNO: la magnitud va al IDAC y el signo a polarity_reg. */
typedef void (*PsocCalVdacWrite)(int16 value);
typedef void (*PsocCalDiagHook)(uint8 event, uint8 value);

typedef struct {
    const char *name;       /* Nombre para logs. */
    uint8 adc_channel;      /* Canal AMux_ADC que mide esta etapa. */
    int32 target_counts;    /* Objetivo ADC; en GEO normalmente 0 counts diferencial. */
    int8 direction;         /* Signo del esfuerzo PI respecto del VDAC. */
    int16 dac_center;       /* Arranque en codigos de IDAC con signo; 0 = Vref. */
    int16 dac_max_change;   /* Rango permitido: [center-max_change, center+max_change]. */
    PsocCalVdacWrite write; /* Funcion que escribe el IDAC fisico de la etapa. */
} PsocCalStage;

/* Resultado por etapa para telemetria post-calibracion (ver uart_send_diag
 * en main.c, eventos PSOC_EVT_CAL_STAGE_DAC / PSOC_EVT_CAL_STAGE_MEAS). */
typedef struct {
    int16 final_dac;        /* Con signo: negativo = referencia por debajo de Vref. */
    int32 final_measured;
    uint8 ok;
} PsocCalResult;

extern PsocCalResult g_psoc_cal_results[PSOC_CAL_MAX_STAGES];
extern uint8 g_psoc_cal_result_count;

void psoc_calibration_set_diag_hook(PsocCalDiagHook hook);
void psoc_calibration_start_references(void);
void psoc_calibration_restore_capture_path(void);
void psoc_calibration_reset_references(void);
void psoc_calibration_seed_default_dac(void);

/* Aplica dac_values[count] como punto de inicio de calibración:
 * escribe cada DAC al hardware y puebla g_psoc_cal_results.
 * Llamar después de psoc_calibration_start_references() para arrancar
 * desde los valores guardados en EEPROM en vez de los defaults. */
/* Los valores vienen SESGADOS por PSOC_IDAC_EEPROM_BIAS: el byte 128 es el
 * codigo 0, o sea la referencia justo en Vref. Asi un codigo con signo entra
 * en el uint8 del slot de EEPROM sin cambiar el layout ni el CRC. */
/* Los codigos son CON SIGNO: negativo = referencia por debajo de Vref. El 0 es
 * exactamente Vref, que es de donde conviene arrancar a calibrar. */
void psoc_calibration_seed_dac(const int16 *dac_values, uint8 count);
/* Convierten entre el codigo con signo y el byte sesgado de la EEPROM. */
uint8 psoc_calibration_dac_to_eeprom(int16 code);
int16 psoc_calibration_dac_from_eeprom(uint8 stored);
void psoc_calibration_report_adc_snapshot(void);

/* Etapas reales de la cascada activa (2 en HAMMER, 3-4 en GEO segun
 * VDAC_ref_BP). main.c no incluye calibration_tables.h (donde vive
 * PSOC_CAL_STAGE_COUNT), así que lo necesita vía este accessor — p.ej. para
 * el gate de PSOC_CMD_SAVE_EEPROM contra el conteo real de etapas, no contra
 * el tope fijo PSOC_NV_CAL_STAGES del layout EEPROM. */
uint8 psoc_calibration_stage_count(void);

/* Retorna: 0=no arranco, 1=calibracion completa en curso,
 * 2=verificacion previa OK; no hizo falta mover DACs. */
uint8 psoc_calibration_start_async(void);
uint8 psoc_calibration_service_async(void);
uint8 psoc_calibration_async_busy(void);
uint8 psoc_calibration_async_result_ok(void);
/* El servo lento se borro. Su unica API que main.c todavia necesitaba era
 * abortar una corrida en curso, que ya la cubre el PI asincrono. */

/* ==========================================================================
 * Autotest (solo en el proyecto AcondicionamientoAnalogicoTest)
 * --------------------------------------------------------------------------
 * El AMux y el lector directo del ADC son static en calibration.c. El
 * secuenciador de autotest vive en psoc_selftest.h (incluido desde main.c) y
 * necesita esas primitivas para barrer cada IDAC contra cada tap del AMux.
 * Se exponen aca en vez de duplicar el manejo del AMux: la exclusion mutua
 * entre canales de señal tiene que seguir viviendo en un solo lugar.
 * ========================================================================== */

int32 psoc_selftest_counts_to_uv(int32 counts);
uint8 psoc_selftest_stage_count(void);
uint8 psoc_selftest_stage_channel(uint8 stage, uint8 *out_channel);
uint8 psoc_selftest_amux_channel_count(void);
uint8 psoc_selftest_write_stage_dac(uint8 stage, uint8 code);
uint8 psoc_selftest_current_stage_dac(uint8 stage, uint8 *out_code);
uint8 psoc_selftest_stage_result(uint8 stage, uint8 *out_dac, int32 *out_meas, uint8 *out_ok);
void  psoc_selftest_select_channel(uint8 channel, uint8 with_cap);
void  psoc_selftest_restore(void);
uint8 psoc_selftest_measure_dc(uint8 channel, uint16 settle_ms, uint16 n,
                               uint8 with_cap, int32 *out_mean, int32 *out_pp);
uint8 psoc_selftest_measure_series(uint8 channel, uint16 settle_ms, uint16 n,
                                   uint8 with_cap, uint16 tone_hz,
                                   int32 *out_mean, int32 *out_rms,
                                   int32 *out_pp, int32 *out_tone);

#endif
