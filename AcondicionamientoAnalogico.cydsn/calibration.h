#ifndef CALIBRATION_H
#define CALIBRATION_H

#include "project.h"
#include "psoc_hw.h"

#ifndef PSOC_STARTUP_CAL_DELAY_MS
#define PSOC_STARTUP_CAL_DELAY_MS 5000u
#endif

#ifndef CAL_ADC_READ_TIMEOUT_US
#define CAL_ADC_READ_TIMEOUT_US 20000u
#endif

#define PSOC_CAL_MAX_STAGES 4u

typedef void (*PsocCalVdacWrite)(uint8 value);

typedef struct {
    const char *name;
    uint8 adc_channel;
    int32 target_counts;
    uint8 avg_n;
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

void psoc_calibration_start_references(void);
void psoc_calibration_restore_capture_path(void);
void psoc_calibration_reset_references(void);
uint8 psoc_calibration_run_blocking(void);

#endif
