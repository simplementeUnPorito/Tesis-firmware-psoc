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

/* Aplica dac_values[count] como punto de inicio de calibración: escribe cada
 * IDAC al hardware y puebla g_psoc_cal_results. Llamar después de
 * psoc_calibration_start_references() para arrancar desde los valores
 * guardados en EEPROM en vez de los defaults.
 *
 * Los códigos son CON SIGNO: negativo = referencia por debajo de Vref, y el 0
 * es Vref exacto, que es de donde conviene arrancar a calibrar. La conversión
 * a los bytes del slot de EEPROM (magnitud + máscara de signos) la hace
 * psoc_nv.c, que es quien conoce el layout de la fila. */
void psoc_calibration_seed_dac(const int16 *dac_values, uint8 count);
/* Ajuste manual de una referencia. Actualiza el codigo aplicado y marca esa
 * etapa como no verificada; una calibracion posterior vuelve a determinarla. */
uint8 psoc_calibration_set_stage_dac(uint8 stage_index, int16 dac);
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

#endif
