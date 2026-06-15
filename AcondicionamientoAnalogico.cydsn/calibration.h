#ifndef CALIBRATION_H
#define CALIBRATION_H

#include "project.h"
#include "psoc_hw.h"

#define PSOC_CAL_MAX_STAGES 4u

typedef void (*PsocCalVdacWrite)(uint8 value);
typedef void (*PsocCalDiagHook)(uint8 event, uint8 value);

/* Configuracion de promediado por ventana deslizante (ver calibration.c,
 * async_measure_service). "measured" = promedio de las ultimas
 * avg_n*window_count muestras (peso constante, no se diluye). Se considera
 * "estable" cuando ese promedio varia <= settle_tol_counts durante
 * stable_streak comparaciones consecutivas con el buffer ya lleno; si se
 * llega a max_samples sin racha estable, se usa el promedio igual. */
typedef struct {
    uint8  avg_n;
    uint8  window_count;
    uint16 max_samples;
    int32  settle_tol_counts;
    uint8  stable_streak;
} PsocCalAvgCfg;

/* Fase final con la señal real del geofono (entrada fija, sin selector):
 * ajuste fino de +-1 LSB sobre el resultado de biseccion/verify, hasta
 * max_nudges veces, revirtiendo si un nudge satura o empeora |error|.
 * enable=0 salta la etapa (se conserva el resultado de verify tal cual). */
typedef struct {
    uint8  enable;
    int32  tol_counts;
    uint8  nudge_step;
    uint8  max_nudges;
    uint16 discard_samples;
    uint16 nudge_discard_samples;
    PsocCalAvgCfg avg;
} PsocCalRealcheckCfg;

typedef struct {
    const char *name;
    uint8 adc_channel;
    int32 target_counts;
    int8 direction;
    uint8 dac_center;
    uint8 dac_max_change;
    uint8 probe_step;
    uint16 max_iter;
    int32 tolerance_counts;
    int32 deadband_counts;
    int32 sat_counts;
    uint16 settle_samples;
    uint16 verify_settle_samples;
    PsocCalAvgCfg avg;
    PsocCalAvgCfg verify_avg;
    PsocCalRealcheckCfg realcheck;
    PsocCalVdacWrite write;
} PsocCalStage;

/* Resultado por etapa para telemetria post-calibracion (ver uart_send_diag
 * en main.c, eventos PSOC_EVT_CAL_STAGE_DAC / PSOC_EVT_CAL_STAGE_MEAS). */
typedef struct {
    uint8 final_dac;
    int32 final_measured;
    uint8 ok;
} PsocCalResult;

extern PsocCalResult g_psoc_cal_results[PSOC_CAL_MAX_STAGES];
extern uint8 g_psoc_cal_result_count;

void psoc_calibration_set_diag_hook(PsocCalDiagHook hook);
void psoc_calibration_start_references(void);
void psoc_calibration_restore_capture_path(void);
void psoc_calibration_reset_references(void);

uint8 psoc_calibration_start_async(void);
uint8 psoc_calibration_service_async(void);
uint8 psoc_calibration_async_busy(void);
uint8 psoc_calibration_async_result_ok(void);
void psoc_calibration_servo_enable(uint8 enable);
uint8 psoc_calibration_servo_enabled(void);
void psoc_calibration_servo_abort(void);
uint8 psoc_calibration_servo_service(void);

#endif
