#ifndef CALIBRATION_H
#define CALIBRATION_H

#include "project.h"
#include "psoc_hw.h"

#ifndef PSOC_STARTUP_CAL_DELAY_MS
#define PSOC_STARTUP_CAL_DELAY_MS 5000u
#endif

#define PSOC_CAL_MAX_STAGES 4u

typedef void (*PsocCalVdacWrite)(uint8 value);
typedef void (*PsocCalDiagHook)(uint8 event, uint8 value);

typedef struct {
    const char *name;
    uint8 adc_channel;
    int32 target_counts;
    uint8 avg_n;
    uint16 settle_samples;
    uint16 verify_settle_samples;
    uint16 max_iter;
    int32 tolerance_counts;
    int8 direction;
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

/* Alternan el canal de AMux_IN y reportan el cambio via PSOC_EVT_CAL_AMUX_IN.
 * Usadas por main.c para alternar entre el estado IDLE (AMux_IN=referencia/
 * tierra virtual, monitoreo continuo de GEO_LP vs. referencia) y el estado
 * activo (AMux_IN=entrada real, a punto de medir/capturar, ver
 * psoc_arm/psoc_start_now). */
void psoc_calibration_amux_active(void);
void psoc_calibration_amux_idle(void);
uint8 psoc_calibration_start_async(void);
uint8 psoc_calibration_service_async(void);
uint8 psoc_calibration_async_busy(void);
uint8 psoc_calibration_async_result_ok(void);

#endif
