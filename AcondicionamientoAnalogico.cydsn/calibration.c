#include "calibration.h"
#include "calibration_tables.h"
#include "psoc_adc.h"

#ifndef CAL_AMUX_ADC_START
#define CAL_AMUX_ADC_START() AMux_ADC_Start()
#endif

#ifndef CAL_AMUX_ADC_SELECT
#define CAL_AMUX_ADC_SELECT(channel) AMux_ADC_Select(channel)
#endif

static PsocCalDiagHook g_cal_diag_hook = (PsocCalDiagHook)0;

void psoc_calibration_set_diag_hook(PsocCalDiagHook hook)
{
    g_cal_diag_hook = hook;
}

static void cal_diag(uint8 event, uint8 value)
{
    if (g_cal_diag_hook != (PsocCalDiagHook)0) {
        g_cal_diag_hook(event, value);
    }
}

static void cal_diag_i16(uint8 event, int32 value)
{
    uint16 u;
    if (value > 32767L) { value = 32767L; }
    if (value < -32768L) { value = -32768L; }
    u = (uint16)(int16)value;
    cal_diag(event, (uint8)(u >> 8));
    cal_diag(event, (uint8)(u & 0xFFu));
}

/* Igual que cal_diag_i16 pero sin clamping: manda los 32 bits crudos de
 * "measured" (cum_avg interno) en 4 bytes MSB-first con el mismo evento.
 * cal_diag_i16 satura a +-32767 para la telemetria historica; ese clamping
 * hace que cualquier |measured|>32767 se vea identico (siempre 32767 o
 * -32768) y oculta si el mejor punto quedo "apenas" o "muy" fuera de
 * CAL_OPERATING_RANGE_COUNTS (ver HANDOFF_CALIBRATION.md). */
static void cal_diag_i32(uint8 event, int32 value)
{
    uint32 u = (uint32)value;
    cal_diag(event, (uint8)((u >> 24) & 0xFFu));
    cal_diag(event, (uint8)((u >> 16) & 0xFFu));
    cal_diag(event, (uint8)((u >> 8) & 0xFFu));
    cal_diag(event, (uint8)(u & 0xFFu));
}

static void cal_diag_point(uint8 dac, int32 measured)
{
    cal_diag(PSOC_EVT_CAL_STAGE_DAC, dac);
    cal_diag_i16(PSOC_EVT_CAL_STAGE_MEAS, measured);
    cal_diag_i32(PSOC_EVT_CAL_STAGE_MEAS32, measured);
}

static void cal_diag_realcheck_point(uint8 dac, int32 measured)
{
    cal_diag(PSOC_EVT_CAL_REALCHECK_DAC, dac);
    cal_diag_i32(PSOC_EVT_CAL_REALCHECK_MEAS32, measured);
}

static void cal_diag_measure_point(uint8 dac, int32 measured, uint8 realcheck)
{
    if (realcheck) {
        cal_diag_realcheck_point(dac, measured);
    } else {
        cal_diag_point(dac, measured);
    }
}

static int32 abs_counts(int32 value)
{
    return (value < 0) ? -value : value;
}

/* Reduce las cuentas crudas del ADC (18 bits, +-131072) a una escala
 * comparable con los codigos del VDAC8 (8 bits, 0-255): 1 cuenta escalada
 * equivale aproximadamente a 1 codigo de DAC. Por redondeo de '>>' con
 * signo, valores negativos redondean hacia -infinito (ej: -1 >> 10 == -1,
 * no 0); se acepta la asimetria de unas pocas cuentas escaladas cerca de
 * cero sin compensarla. */
#define CAL_DAC_SCALE_SHIFT 10

static int32 cal_scale_counts(int32 value)
{
    return value >> CAL_DAC_SCALE_SHIFT;
}

/* Ver CAL_OPERATING_RANGE_COUNTS (calibration_tables.h): +-0.5V absolutos.
 * Una medicion mas alla de esto deja al operacional fuera de rango
 * operativo, sin importar si "ok" hubiera dado 1. */
static uint8 cal_measured_out_of_range(int32 measured)
{
    return (abs_counts(cal_scale_counts(measured)) > cal_scale_counts(CAL_OPERATING_RANGE_COUNTS)) ? 1u : 0u;
}

static uint8 cal_stage_saturated(const PsocCalStage *stage, int32 measured)
{
    if (stage->sat_counts <= 0L) {
        return 0u;
    }
    return (abs_counts(cal_scale_counts(measured)) >= cal_scale_counts(stage->sat_counts)) ? 1u : 0u;
}

static uint8 cal_stage_min_dac(const PsocCalStage *stage)
{
    return (stage->dac_center > stage->dac_max_change)
        ? (uint8)(stage->dac_center - stage->dac_max_change)
        : 0u;
}

static uint8 cal_stage_max_dac(const PsocCalStage *stage)
{
    uint16 max_dac = (uint16)stage->dac_center + (uint16)stage->dac_max_change;
    return (max_dac > 255u) ? 255u : (uint8)max_dac;
}

static uint8 cal_stage_clamp_dac(const PsocCalStage *stage, uint8 dac)
{
    uint8 lo = cal_stage_min_dac(stage);
    uint8 hi = cal_stage_max_dac(stage);
    if (dac < lo) { return lo; }
    if (dac > hi) { return hi; }
    return dac;
}

static uint8 cal_stage_center_dac(const PsocCalStage *stage)
{
    return cal_stage_clamp_dac(stage, stage->dac_center);
}

static uint8 cal_stage_probe_dac(const PsocCalStage *stage)
{
    uint8 center = cal_stage_center_dac(stage);
    uint8 lo = cal_stage_min_dac(stage);
    uint8 hi = cal_stage_max_dac(stage);

    if (stage->direction >= 0) {
        if ((uint16)center + (uint16)stage->probe_step <= hi) {
            return (uint8)(center + stage->probe_step);
        }
        if (center > lo) {
            uint8 down = (center > stage->probe_step) ? (uint8)(center - stage->probe_step) : lo;
            return cal_stage_clamp_dac(stage, down);
        }
    } else {
        if (center > lo) {
            uint8 down = (center > stage->probe_step) ? (uint8)(center - stage->probe_step) : lo;
            return cal_stage_clamp_dac(stage, down);
        }
        if ((uint16)center + (uint16)stage->probe_step <= hi) {
            return (uint8)(center + stage->probe_step);
        }
    }

    return (center < hi) ? hi : lo;
}

PsocCalResult g_psoc_cal_results[PSOC_CAL_MAX_STAGES];
uint8 g_psoc_cal_result_count = 0u;

#define CAL_ASYNC_EMPTY_POLL_LIMIT 2000000UL

/* Watchdog global: ticks de 10 ms. Debe quedar por debajo del timeout del ESP
 * para que CAL_DONE llegue antes de que el slave de por perdida la corrida. */
#ifndef CAL_WATCHDOG_TICKS
#define CAL_WATCHDOG_TICKS 40000UL
#endif

/* Periodo de telemetria de progreso (ticks de 10 ms => ~500 ms). */
#define CAL_PROGRESS_PERIOD_TICKS 50UL

typedef enum {
    CAL_ASYNC_IDLE = 0u,
    CAL_ASYNC_STAGE_BEGIN,
    CAL_ASYNC_MEASURE,
    CAL_ASYNC_EVAL_INIT,
    CAL_ASYNC_EVAL_PROBE,
    CAL_ASYNC_PLAN_ITER,
    CAL_ASYNC_EVAL_ITER,
    CAL_ASYNC_VERIFY_BEGIN,
    CAL_ASYNC_EVAL_VERIFY,
    CAL_ASYNC_REALCHECK_SWITCH,
    CAL_ASYNC_REALCHECK_BEGIN,
    CAL_ASYNC_EVAL_REALCHECK,
    CAL_ASYNC_DONE
} PsocCalAsyncState;

typedef struct {
    PsocCalAsyncState state;
    PsocCalAsyncState after_measure;
    uint8 busy;
    uint8 done;
    uint8 ok;
    uint8 stage_index;
    uint8 dac;
    uint8 lo;
    uint8 hi;
    uint8 increasing;
    uint16 iter;
    uint8 best_dac;
    int32 measured;
    int32 base_measured;
    int32 best_measured;
    int32 best_abs_error;
    uint8 best_saturated;
    uint8 saturated_seen;
    uint8 non_saturated_seen;
    uint8 candidate_count;
    uint8 candidate_dac[CAL_BEST_CANDIDATE_COUNT];
    int32 candidate_measured[CAL_BEST_CANDIDATE_COUNT];
    int32 candidate_abs_error[CAL_BEST_CANDIDATE_COUNT];
    uint8 candidate_saturated[CAL_BEST_CANDIDATE_COUNT];
    uint8 visited_count;
    uint8 visited_pos;
    uint8 visited_dac[CAL_VISIT_HISTORY_COUNT];
    uint8 slope_known[PSOC_CAL_MAX_STAGES];
    uint8 slope_increasing[PSOC_CAL_MAX_STAGES];
    int32 acc;
    const PsocCalAvgCfg *avg_cfg;
    int32 window_buf[CAL_AVG_WINDOW_MAX];
    int32 window_sum;
    int32 prev_avg;
    uint16 avg_count;
    uint16 discard_count;
    uint16 total_samples;
    uint8 window_pos;
    uint8 window_filled_count;
    uint8 stable_streak_count;
    uint8 have_prev_avg;
    uint8 realcheck_diag;
    uint8 realcheck_candidate_active;
    uint8 realcheck_nudge_count;
    uint8 realcheck_current_dac;
    int32 realcheck_current_measured;
    int32 realcheck_current_abs_error;
    uint8 realcheck_current_saturated;
    int8 realcheck_last_nudge;
    uint32 empty_polls;
    uint32 start_ticks;
    uint32 last_progress_ticks;
} PsocCalAsync;

static PsocCalAsync g_cal_async = { CAL_ASYNC_IDLE };

static void cal_async_reset_stage_memory(void)
{
    uint8 i;
    g_cal_async.best_dac = 0u;
    g_cal_async.best_measured = 0L;
    g_cal_async.best_abs_error = 0x7FFFFFFFL;
    g_cal_async.best_saturated = 1u;
    g_cal_async.saturated_seen = 0u;
    g_cal_async.non_saturated_seen = 0u;
    g_cal_async.candidate_count = 0u;
    g_cal_async.visited_count = 0u;
    g_cal_async.visited_pos = 0u;
    for (i = 0u; i < CAL_BEST_CANDIDATE_COUNT; i++) {
        g_cal_async.candidate_dac[i] = 0u;
        g_cal_async.candidate_measured[i] = 0L;
        g_cal_async.candidate_abs_error[i] = 0x7FFFFFFFL;
        g_cal_async.candidate_saturated[i] = 1u;
    }
    for (i = 0u; i < CAL_VISIT_HISTORY_COUNT; i++) {
        g_cal_async.visited_dac[i] = 0u;
    }
}

static uint8 cal_async_seen_dac(uint8 dac)
{
    uint8 i;
    for (i = 0u; i < g_cal_async.visited_count; i++) {
        if (g_cal_async.visited_dac[i] == dac) {
            return 1u;
        }
    }
    return 0u;
}

static void cal_async_remember_dac(uint8 dac)
{
    if (g_cal_async.visited_count < CAL_VISIT_HISTORY_COUNT) {
        g_cal_async.visited_dac[g_cal_async.visited_count] = dac;
        g_cal_async.visited_count++;
        return;
    }
    g_cal_async.visited_dac[g_cal_async.visited_pos] = dac;
    g_cal_async.visited_pos =
        (uint8)((g_cal_async.visited_pos + 1u) % CAL_VISIT_HISTORY_COUNT);
}

static void cal_async_update_best(uint8 dac, int32 measured, int32 abs_error,
                                  uint8 saturated)
{
    if (saturated) {
        g_cal_async.saturated_seen = 1u;
        if (!g_cal_async.non_saturated_seen &&
            (g_cal_async.best_saturated ||
             abs_error < g_cal_async.best_abs_error)) {
            g_cal_async.best_abs_error = abs_error;
            g_cal_async.best_dac = dac;
            g_cal_async.best_measured = measured;
            g_cal_async.best_saturated = 1u;
        }
        return;
    }

    g_cal_async.non_saturated_seen = 1u;
    if (g_cal_async.best_saturated ||
        abs_error < g_cal_async.best_abs_error) {
        g_cal_async.best_abs_error = abs_error;
        g_cal_async.best_dac = dac;
        g_cal_async.best_measured = measured;
        g_cal_async.best_saturated = 0u;
    }
}

static void cal_async_store_candidate(uint8 dac, int32 measured, int32 abs_error,
                                      uint8 saturated)
{
    uint8 i;
    uint8 worst_i = 0u;
    int32 worst_score = -1L;

    cal_async_update_best(dac, measured, abs_error, saturated);

    for (i = 0u; i < g_cal_async.candidate_count; i++) {
        if (g_cal_async.candidate_dac[i] == dac) {
            if ((!saturated && g_cal_async.candidate_saturated[i]) ||
                (saturated == g_cal_async.candidate_saturated[i] &&
                 abs_error < g_cal_async.candidate_abs_error[i])) {
                g_cal_async.candidate_measured[i] = measured;
                g_cal_async.candidate_abs_error[i] = abs_error;
                g_cal_async.candidate_saturated[i] = saturated;
            }
            return;
        }
    }

    if (g_cal_async.candidate_count < CAL_BEST_CANDIDATE_COUNT) {
        i = g_cal_async.candidate_count;
        g_cal_async.candidate_count++;
        g_cal_async.candidate_dac[i] = dac;
        g_cal_async.candidate_measured[i] = measured;
        g_cal_async.candidate_abs_error[i] = abs_error;
        g_cal_async.candidate_saturated[i] = saturated;
        return;
    }

    for (i = 0u; i < CAL_BEST_CANDIDATE_COUNT; i++) {
        int32 score = g_cal_async.candidate_abs_error[i];
        if (g_cal_async.candidate_saturated[i]) {
            score += 0x10000000L;
        }
        if (score > worst_score) {
            worst_score = score;
            worst_i = i;
        }
    }

    {
        int32 new_score = abs_error;
        if (saturated) {
            new_score += 0x10000000L;
        }
        if (new_score < worst_score) {
            g_cal_async.candidate_dac[worst_i] = dac;
            g_cal_async.candidate_measured[worst_i] = measured;
            g_cal_async.candidate_abs_error[worst_i] = abs_error;
            g_cal_async.candidate_saturated[worst_i] = saturated;
        }
    }
}

static uint8 cal_async_record_measurement(const PsocCalStage *stage)
{
    int32 abs_error = abs_counts(cal_scale_counts(stage->target_counts) -
                                  cal_scale_counts(g_cal_async.measured));
    uint8 seen = cal_async_seen_dac(g_cal_async.dac);
    uint8 saturated = cal_stage_saturated(stage, g_cal_async.measured);

    cal_diag(PSOC_EVT_CAL_STAGE_SAT, saturated);
    cal_async_store_candidate(g_cal_async.dac, g_cal_async.measured,
                              abs_error, saturated);
    if (!seen) {
        cal_async_remember_dac(g_cal_async.dac);
    }
    return seen;
}

typedef enum {
    CAL_SERVO_IDLE = 0u,
    CAL_SERVO_MEASURE
} PsocCalServoState;

typedef struct {
    uint8 enabled;
    PsocCalServoState state;
    uint8 stage_index;
    uint8 dac;
    int32 acc;
    int32 cum_sum;
    int32 cum_count;
    int32 prev_avg;
    uint16 avg_count;
    uint16 discard_count;
    uint8 settle_windows;
    uint8 have_prev_avg;
    uint32 empty_polls;
    uint32 next_due_ticks;

    uint8 have_last[PSOC_CAL_MAX_STAGES];
    uint8 last_dac[PSOC_CAL_MAX_STAGES];
    int32 last_measured[PSOC_CAL_MAX_STAGES];
    uint8 have_slope[PSOC_CAL_MAX_STAGES];
    uint8 increasing[PSOC_CAL_MAX_STAGES];
    uint8 trial_active[PSOC_CAL_MAX_STAGES];
    uint8 prev_dac[PSOC_CAL_MAX_STAGES];
    int32 prev_abs_error[PSOC_CAL_MAX_STAGES];
    int8 last_dir[PSOC_CAL_MAX_STAGES];
    int32 integral[PSOC_CAL_MAX_STAGES];
} PsocCalServo;

typedef struct {
    int32 kp_num;
    int32 ki_num;
    int32 ki_div;
    int32 deadband_counts;
    uint8 fine_step;
    uint8 recovery_step;
} PsocCalServoTune;

static PsocCalServo g_cal_servo = { CAL_SERVO_ENABLE_DEFAULT };

static const PsocCalServoTune g_cal_servo_tune[PSOC_CAL_STAGE_COUNT] = {
#if PSOC_HW_CLASS == PSOC_HW_GEO
    { CAL_SERVO_KP_NUM_GEO_PGA,   CAL_SERVO_KI_NUM_GEO_PGA,   CAL_SERVO_KI_DIV_GEO_PGA,   CAL_SERVO_DEADBAND_GEO_PGA,   CAL_SERVO_FINE_STEP_GEO_PGA,   CAL_SERVO_RECOVERY_STEP_GEO_PGA },
    { CAL_SERVO_KP_NUM_GEO_BP,    CAL_SERVO_KI_NUM_GEO_BP,    CAL_SERVO_KI_DIV_GEO_BP,    CAL_SERVO_DEADBAND_GEO_BP,    CAL_SERVO_FINE_STEP_GEO_BP,    CAL_SERVO_RECOVERY_STEP_GEO_BP },
    { CAL_SERVO_KP_NUM_GEO_ADDER, CAL_SERVO_KI_NUM_GEO_ADDER, CAL_SERVO_KI_DIV_GEO_ADDER, CAL_SERVO_DEADBAND_GEO_ADDER, CAL_SERVO_FINE_STEP_GEO_ADDER, CAL_SERVO_RECOVERY_STEP_GEO_ADDER },
    { CAL_SERVO_KP_NUM_GEO_LP,    CAL_SERVO_KI_NUM_GEO_LP,    CAL_SERVO_KI_DIV_GEO_LP,    CAL_SERVO_DEADBAND_GEO_LP,    CAL_SERVO_FINE_STEP_GEO_LP,    CAL_SERVO_RECOVERY_STEP_GEO_LP },
#else
    { CAL_SERVO_KP_NUM_HAMMER_IN,  CAL_SERVO_KI_NUM_HAMMER_IN,  CAL_SERVO_KI_DIV_HAMMER_IN,  CAL_SERVO_DEADBAND_HAMMER_IN,  CAL_SERVO_FINE_STEP_HAMMER_IN,  CAL_SERVO_RECOVERY_STEP_HAMMER_IN },
    { CAL_SERVO_KP_NUM_HAMMER_PGA, CAL_SERVO_KI_NUM_HAMMER_PGA, CAL_SERVO_KI_DIV_HAMMER_PGA, CAL_SERVO_DEADBAND_HAMMER_PGA, CAL_SERVO_FINE_STEP_HAMMER_PGA, CAL_SERVO_RECOVERY_STEP_HAMMER_PGA },
    { CAL_SERVO_KP_NUM_HAMMER_LP,  CAL_SERVO_KI_NUM_HAMMER_LP,  CAL_SERVO_KI_DIV_HAMMER_LP,  CAL_SERVO_DEADBAND_HAMMER_LP,  CAL_SERVO_FINE_STEP_HAMMER_LP,  CAL_SERVO_RECOVERY_STEP_HAMMER_LP },
#endif
};

static int32 cal_servo_clip_integral(int32 value)
{
    if (value > CAL_SERVO_INTEGRAL_LIMIT) { return CAL_SERVO_INTEGRAL_LIMIT; }
    if (value < -CAL_SERVO_INTEGRAL_LIMIT) { return -CAL_SERVO_INTEGRAL_LIMIT; }
    return value;
}

static uint16 cal_servo_settle_samples(uint8 stage_index)
{
#if PSOC_HW_CLASS == PSOC_HW_GEO
    switch (stage_index) {
        case 0u: return CAL_SERVO_SETTLE_SAMPLES_GEO_PGA;
        case 1u: return CAL_SERVO_SETTLE_SAMPLES_GEO_BP;
        case 2u: return CAL_SERVO_SETTLE_SAMPLES_GEO_ADDER;
        default: return CAL_SERVO_SETTLE_SAMPLES_GEO_LP;
    }
#else
    switch (stage_index) {
        case 0u: return CAL_SERVO_SETTLE_SAMPLES_HAMMER_IN;
        case 1u: return CAL_SERVO_SETTLE_SAMPLES_HAMMER_PGA;
        default: return CAL_SERVO_SETTLE_SAMPLES_HAMMER_LP;
    }
#endif
}

static uint8 cal_stage_current_dac(uint8 stage_index)
{
    if (stage_index >= g_psoc_cal_result_count) {
        return cal_stage_center_dac(&g_psoc_cal_stages[stage_index]);
    }
    return cal_stage_clamp_dac(&g_psoc_cal_stages[stage_index],
                               g_psoc_cal_results[stage_index].final_dac);
}

static void cal_stage_write_result(uint8 stage_index, uint8 dac)
{
    if (stage_index < PSOC_CAL_STAGE_COUNT) {
        uint8 clamped = cal_stage_clamp_dac(&g_psoc_cal_stages[stage_index], dac);
        g_psoc_cal_stages[stage_index].write(clamped);
        g_psoc_cal_results[stage_index].final_dac = clamped;
        if (g_psoc_cal_result_count < PSOC_CAL_STAGE_COUNT) {
            g_psoc_cal_result_count = PSOC_CAL_STAGE_COUNT;
        }
    }
}

static uint8 cal_servo_apply_measurement(uint8 stage_index, uint8 dac, int32 measured)
{
    const PsocCalStage *stage = &g_psoc_cal_stages[stage_index];
    const PsocCalServoTune *tune = &g_cal_servo_tune[stage_index];
    PsocCalResult *result = &g_psoc_cal_results[stage_index];
    int32 error = stage->target_counts - measured;
    int32 abs_error = abs_counts(error);
    int8 dir;
    uint8 step;
    int16 next;
    uint8 slope_increasing;

    result->final_measured = measured;
    result->ok = (abs_error <= stage->tolerance_counts) ? 1u : 0u;

    if (g_cal_servo.have_last[stage_index] &&
        g_cal_servo.last_dac[stage_index] != dac &&
        g_cal_servo.last_measured[stage_index] != measured) {
        slope_increasing =
            ((measured > g_cal_servo.last_measured[stage_index]) ==
             (dac > g_cal_servo.last_dac[stage_index])) ? 1u : 0u;
        g_cal_servo.increasing[stage_index] = slope_increasing;
        g_cal_servo.have_slope[stage_index] = 1u;
    }

    if (abs_error <= tune->deadband_counts) {
        g_cal_servo.integral[stage_index] /= 2L;
        g_cal_servo.trial_active[stage_index] = 0u;
        g_cal_servo.have_last[stage_index] = 1u;
        g_cal_servo.last_dac[stage_index] = dac;
        g_cal_servo.last_measured[stage_index] = measured;
        return 1u;
    }

    if (g_cal_servo.trial_active[stage_index] &&
        abs_error > (g_cal_servo.prev_abs_error[stage_index] +
                     CAL_SERVO_WORSE_HYST_COUNTS) &&
        g_cal_servo.last_dir[stage_index] != 0) {
        uint8 restore_dac = g_cal_servo.last_dac[stage_index];
        cal_stage_write_result(stage_index, restore_dac);
        result->final_measured = g_cal_servo.last_measured[stage_index];
        result->ok = (g_cal_servo.prev_abs_error[stage_index] <=
                      stage->tolerance_counts) ? 1u : 0u;
        g_cal_servo.integral[stage_index] = 0L;
        g_cal_servo.trial_active[stage_index] = 0u;
        cal_diag(PSOC_EVT_CAL_LOOP, restore_dac);
        return 1u;
    } else {
        int32 control;
        uint8 assume_increasing;

        g_cal_servo.integral[stage_index] =
            cal_servo_clip_integral(g_cal_servo.integral[stage_index] + error);
        control = (error * tune->kp_num) +
            ((g_cal_servo.integral[stage_index] * tune->ki_num) /
             ((tune->ki_div == 0L) ? 1L : tune->ki_div));
        if (control == 0L) {
            control = error;
        }

        assume_increasing = g_cal_servo.have_slope[stage_index]
            ? g_cal_servo.increasing[stage_index]
            : ((stage->direction >= 0) ? 1u : 0u);
        if (assume_increasing) {
            dir = (control > 0L) ? 1 : -1;
        } else {
            dir = (control > 0L) ? -1 : 1;
        }
    }

    step = (abs_error > CAL_OPERATING_RANGE_COUNTS)
        ? tune->recovery_step
        : tune->fine_step;
    next = (int16)dac + (int16)((int16)dir * (int16)step);
    if (next < (int16)cal_stage_min_dac(stage)) {
        next = (int16)cal_stage_min_dac(stage);
    }
    if (next > (int16)cal_stage_max_dac(stage)) {
        next = (int16)cal_stage_max_dac(stage);
    }

    g_cal_servo.have_last[stage_index] = 1u;
    g_cal_servo.last_dac[stage_index] = dac;
    g_cal_servo.last_measured[stage_index] = measured;

    if ((uint8)next != dac) {
        if (g_cal_servo.trial_active[stage_index] &&
            (uint8)next == g_cal_servo.prev_dac[stage_index]) {
            g_cal_servo.integral[stage_index] = 0L;
            g_cal_servo.trial_active[stage_index] = 0u;
            cal_diag(PSOC_EVT_CAL_LOOP, (uint8)next);
            return 1u;
        }
        g_cal_servo.prev_dac[stage_index] = dac;
        g_cal_servo.prev_abs_error[stage_index] = abs_error;
        g_cal_servo.last_dir[stage_index] = dir;
        g_cal_servo.trial_active[stage_index] = 1u;
        cal_stage_write_result(stage_index, (uint8)next);
        cal_diag(PSOC_EVT_SERVO_STEP, (uint8)next);
        return 0u;
    } else {
        g_cal_servo.trial_active[stage_index] = 0u;
        return 1u;
    }
}

static void cal_servo_measure_begin(uint8 stage_index)
{
    const PsocCalStage *stage = &g_psoc_cal_stages[stage_index];

    g_cal_servo.stage_index = stage_index;
    g_cal_servo.dac = cal_stage_current_dac(stage_index);
    g_cal_servo.acc = 0L;
    g_cal_servo.cum_sum = 0L;
    g_cal_servo.cum_count = 0L;
    g_cal_servo.prev_avg = 0L;
    g_cal_servo.avg_count = 0u;
    g_cal_servo.discard_count = cal_servo_settle_samples(stage_index);
    g_cal_servo.settle_windows = 0u;
    g_cal_servo.have_prev_avg = 0u;
    g_cal_servo.empty_polls = 0UL;

    ADC_Stop();
    CAL_AMUX_ADC_SELECT(stage->adc_channel);
    ADC_Start();
    isr_DelSig_ClearPending();
    psoc_adc_clear_isr_sample();
    ADC_StartConvert();
    cal_diag(PSOC_EVT_SERVO_STAGE, stage_index);
    g_cal_servo.state = CAL_SERVO_MEASURE;
}

static uint8 cal_servo_measure_service(void)
{
    int32 sample;
    int32 cum_avg;
    uint8 stage_index = g_cal_servo.stage_index;

    if (!psoc_adc_take_isr_sample(&sample)) {
        g_cal_servo.empty_polls++;
        if (g_cal_servo.empty_polls >= CAL_ASYNC_EMPTY_POLL_LIMIT) {
            g_cal_servo.state = CAL_SERVO_IDLE;
            psoc_calibration_restore_capture_path();
            return 1u;
        }
        return 0u;
    }

    g_cal_servo.empty_polls = 0UL;
    if (g_cal_servo.discard_count > 0u) {
        g_cal_servo.discard_count--;
        return 0u;
    }

    g_cal_servo.acc += sample;
    g_cal_servo.avg_count++;
    if (g_cal_servo.avg_count < CAL_SERVO_AVG_N) {
        return 0u;
    }

    g_cal_servo.cum_sum += g_cal_servo.acc;
    g_cal_servo.cum_count += (int32)CAL_SERVO_AVG_N;
    g_cal_servo.acc = 0L;
    g_cal_servo.avg_count = 0u;

    cum_avg = g_cal_servo.cum_sum / g_cal_servo.cum_count;
    if (g_cal_servo.have_prev_avg &&
        abs_counts(cum_avg - g_cal_servo.prev_avg) <= CAL_SERVO_SETTLE_TOL_COUNTS) {
        uint8 locked;
        cal_diag(PSOC_EVT_CAL_STAGE_DAC, g_cal_servo.dac);
        cal_diag_i16(PSOC_EVT_CAL_STAGE_MEAS, cum_avg);
        cal_diag_i32(PSOC_EVT_CAL_STAGE_MEAS32, cum_avg);
        locked = cal_servo_apply_measurement(stage_index, g_cal_servo.dac, cum_avg);
        psoc_calibration_restore_capture_path();
        if (locked) {
            g_cal_servo.stage_index = (uint8)((stage_index + 1u) % PSOC_CAL_STAGE_COUNT);
        }
        g_cal_servo.next_due_ticks = psoc_now_ticks() + CAL_SERVO_PERIOD_TICKS;
        g_cal_servo.state = CAL_SERVO_IDLE;
        return 1u;
    }

    g_cal_servo.prev_avg = cum_avg;
    g_cal_servo.have_prev_avg = 1u;
    g_cal_servo.settle_windows++;
    if (g_cal_servo.settle_windows >= CAL_SERVO_SETTLE_MAX_WINDOWS) {
        uint8 locked;
        cal_diag(PSOC_EVT_CAL_STAGE_DAC, g_cal_servo.dac);
        cal_diag_i16(PSOC_EVT_CAL_STAGE_MEAS, cum_avg);
        cal_diag_i32(PSOC_EVT_CAL_STAGE_MEAS32, cum_avg);
        locked = cal_servo_apply_measurement(stage_index, g_cal_servo.dac, cum_avg);
        psoc_calibration_restore_capture_path();
        if (locked) {
            g_cal_servo.stage_index = (uint8)((stage_index + 1u) % PSOC_CAL_STAGE_COUNT);
        }
        g_cal_servo.next_due_ticks = psoc_now_ticks() + CAL_SERVO_PERIOD_TICKS;
        g_cal_servo.state = CAL_SERVO_IDLE;
        return 1u;
    }

    return 0u;
}

void psoc_calibration_start_references(void)
{
    uint8 i;

    CAL_AMUX_ADC_START();
    ADC_Stop();
    CAL_AMUX_ADC_SELECT(CAL_ADC_CAPTURE_CHANNEL);
    ADC_Start();
    ADC_StopConvert();

#if PSOC_HW_CLASS == PSOC_HW_GEO
    VDAC_ref_PGA_Start();
    VDAC_ref_BP_Start();
    VDAC_Ref_Adder_Start();
    VDAC_ref_LP_Start();
#else
    HAMMER_VDAC_IN_START();
    VDAC_PGA_Start();
    VDAC_LP_Start();
#endif

    for (i = 0u; i < PSOC_CAL_STAGE_COUNT; i++) {
        uint8 center = cal_stage_center_dac(&g_psoc_cal_stages[i]);
        g_psoc_cal_stages[i].write(center);
        g_psoc_cal_results[i].final_dac = center;
        g_psoc_cal_results[i].final_measured = 0L;
        g_psoc_cal_results[i].ok = 1u;
    }
    g_psoc_cal_result_count = PSOC_CAL_STAGE_COUNT;
}

/* Deja el AMux_ADC en el canal de captura (GEO_LP): este es el estado IDLE,
 * en el que el ADC queda mirando GEO_LP para poder verificar en cualquier
 * momento si el front-end sigue calibrado. */
void psoc_calibration_restore_capture_path(void)
{
    ADC_Stop();
    CAL_AMUX_ADC_SELECT(CAL_ADC_CAPTURE_CHANNEL);
    ADC_Start();
    ADC_StopConvert();
}

void psoc_calibration_reset_references(void)
{
    uint8 i;

    ADC_Stop();
    for (i = 0u; i < PSOC_CAL_STAGE_COUNT; i++) {
        uint8 center = cal_stage_center_dac(&g_psoc_cal_stages[i]);
        g_psoc_cal_stages[i].write(center);
        g_psoc_cal_results[i].final_dac = center;
        g_psoc_cal_results[i].final_measured = 0L;
        g_psoc_cal_results[i].ok = 1u;
    }
    g_psoc_cal_result_count = PSOC_CAL_STAGE_COUNT;
    psoc_calibration_restore_capture_path();
}

void psoc_calibration_servo_enable(uint8 enable)
{
    g_cal_servo.enabled = enable ? 1u : 0u;
    if (!g_cal_servo.enabled) {
        psoc_calibration_servo_abort();
    } else {
        g_cal_servo.next_due_ticks = psoc_now_ticks();
    }
}

uint8 psoc_calibration_servo_enabled(void)
{
    return g_cal_servo.enabled;
}

void psoc_calibration_servo_abort(void)
{
    if (g_cal_servo.state == CAL_SERVO_MEASURE) {
        ADC_StopConvert();
        psoc_calibration_restore_capture_path();
    }
    g_cal_servo.state = CAL_SERVO_IDLE;
    g_cal_servo.next_due_ticks = psoc_now_ticks() + CAL_SERVO_PERIOD_TICKS;
}

uint8 psoc_calibration_servo_service(void)
{
    if (!g_cal_servo.enabled || g_cal_async.busy) {
        return 0u;
    }

    if (g_cal_servo.state == CAL_SERVO_IDLE) {
        uint32 now = psoc_now_ticks();
        if ((int32)(now - g_cal_servo.next_due_ticks) >= 0) {
            cal_servo_measure_begin(g_cal_servo.stage_index);
        }
        return 0u;
    }

    return cal_servo_measure_service();
}

static uint8 cal_avg_cfg_avg_n(const PsocCalAvgCfg *cfg)
{
    return (cfg->avg_n == 0u) ? 1u : cfg->avg_n;
}

static uint8 cal_avg_cfg_window_count(const PsocCalAvgCfg *cfg)
{
    if (cfg->window_count == 0u) {
        return 1u;
    }
    if (cfg->window_count > CAL_AVG_WINDOW_MAX) {
        return CAL_AVG_WINDOW_MAX;
    }
    return cfg->window_count;
}

static uint16 cal_avg_cfg_max_samples(const PsocCalAvgCfg *cfg)
{
    uint16 floor_samples =
        (uint16)((uint16)cal_avg_cfg_avg_n(cfg) *
                 (uint16)cal_avg_cfg_window_count(cfg));
    if (cfg->max_samples < floor_samples) {
        return floor_samples;
    }
    return cfg->max_samples;
}

static uint8 cal_avg_cfg_stable_streak(const PsocCalAvgCfg *cfg)
{
    return (cfg->stable_streak == 0u) ? 1u : cfg->stable_streak;
}

static void async_measure_begin(uint8 dac, PsocCalAsyncState after_measure,
                                const PsocCalAvgCfg *avg_cfg,
                                uint16 discard_samples, uint8 write_dac,
                                uint8 realcheck_diag)
{
    const PsocCalStage *stage = &g_psoc_cal_stages[g_cal_async.stage_index];
    uint8 i;

    g_cal_async.dac = dac;
    g_cal_async.after_measure = after_measure;
    g_cal_async.avg_cfg = avg_cfg;
    g_cal_async.acc = 0L;
    g_cal_async.window_sum = 0L;
    g_cal_async.avg_count = 0u;
    g_cal_async.discard_count = discard_samples;
    g_cal_async.total_samples = 0u;
    g_cal_async.window_pos = 0u;
    g_cal_async.window_filled_count = 0u;
    g_cal_async.stable_streak_count = 0u;
    g_cal_async.have_prev_avg = 0u;
    g_cal_async.prev_avg = 0L;
    g_cal_async.empty_polls = 0UL;
    g_cal_async.realcheck_diag = realcheck_diag;
    for (i = 0u; i < CAL_AVG_WINDOW_MAX; i++) {
        g_cal_async.window_buf[i] = 0L;
    }
    if (write_dac) {
        stage->write(dac);
    }
    psoc_adc_clear_isr_sample();
    g_cal_async.state = CAL_ASYNC_MEASURE;
}

/* Mide con una ventana deslizante de peso constante: cada ventana de avg_n
 * muestras entra a un buffer circular de window_count sumas. El promedio se
 * calcula sobre las ultimas avg_n*window_count muestras; solo se considera
 * estable cuando ese promedio varia poco durante stable_streak comparaciones
 * consecutivas, con el buffer ya lleno. */
static uint8 async_measure_service(void)
{
    int32 sample;
    int32 sliding_avg;
    const PsocCalAvgCfg *cfg = g_cal_async.avg_cfg;
    uint8 avg_n = cal_avg_cfg_avg_n(cfg);
    uint8 window_count = cal_avg_cfg_window_count(cfg);
    uint16 max_samples = cal_avg_cfg_max_samples(cfg);

    if (!psoc_adc_take_isr_sample(&sample)) {
        g_cal_async.empty_polls++;
        if (g_cal_async.empty_polls >= CAL_ASYNC_EMPTY_POLL_LIMIT) {
            g_cal_async.measured = 0x7FFFL;
            cal_diag_measure_point(g_cal_async.dac, g_cal_async.measured,
                                   g_cal_async.realcheck_diag);
            g_cal_async.state = g_cal_async.after_measure;
            return 1u;
        }
        return 0u;
    }

    g_cal_async.empty_polls = 0UL;
    if (g_cal_async.discard_count > 0u) {
        g_cal_async.discard_count--;
        return 0u;
    }

    g_cal_async.acc += sample;
    g_cal_async.avg_count++;
    if (g_cal_async.avg_count < avg_n) {
        return 0u;
    }

    if (g_cal_async.window_filled_count < window_count) {
        g_cal_async.window_buf[g_cal_async.window_pos] = g_cal_async.acc;
        g_cal_async.window_sum += g_cal_async.acc;
        g_cal_async.window_filled_count++;
    } else {
        g_cal_async.window_sum -= g_cal_async.window_buf[g_cal_async.window_pos];
        g_cal_async.window_buf[g_cal_async.window_pos] = g_cal_async.acc;
        g_cal_async.window_sum += g_cal_async.acc;
    }
    g_cal_async.window_pos++;
    if (g_cal_async.window_pos >= window_count) {
        g_cal_async.window_pos = 0u;
    }
    g_cal_async.total_samples =
        (uint16)(g_cal_async.total_samples + (uint16)avg_n);
    g_cal_async.acc = 0L;
    g_cal_async.avg_count = 0u;

    sliding_avg = g_cal_async.window_sum /
        ((int32)avg_n * (int32)g_cal_async.window_filled_count);
    g_cal_async.measured = sliding_avg;

    if (g_cal_async.window_filled_count >= window_count) {
        if (g_cal_async.have_prev_avg &&
            abs_counts(sliding_avg - g_cal_async.prev_avg) <=
                cfg->settle_tol_counts) {
            g_cal_async.stable_streak_count++;
        } else {
            g_cal_async.stable_streak_count = 0u;
        }

        if (g_cal_async.stable_streak_count >=
            cal_avg_cfg_stable_streak(cfg)) {
            cal_diag_measure_point(g_cal_async.dac, g_cal_async.measured,
                                   g_cal_async.realcheck_diag);
            g_cal_async.state = g_cal_async.after_measure;
            return 1u;
        }

        g_cal_async.prev_avg = sliding_avg;
        g_cal_async.have_prev_avg = 1u;
    }

    if (g_cal_async.total_samples >= max_samples) {
        cal_diag_measure_point(g_cal_async.dac, g_cal_async.measured,
                               g_cal_async.realcheck_diag);
        g_cal_async.state = g_cal_async.after_measure;
        return 1u;
    }

    return 0u;
}

static void async_finish_stage(uint8 ok)
{
    const PsocCalStage *stage = &g_psoc_cal_stages[g_cal_async.stage_index];
    PsocCalResult *result = &g_psoc_cal_results[g_cal_async.stage_index];
    uint8 final_dac = g_cal_async.best_dac;
    int32 final_measured = g_cal_async.best_measured;

    if (g_cal_async.best_abs_error <= cal_scale_counts(stage->tolerance_counts)) {
        ok = 1u;
    }

    if (g_cal_async.best_saturated && !g_cal_async.non_saturated_seen &&
        g_cal_async.saturated_seen) {
        cal_diag(PSOC_EVT_CAL_STAGE_SAT_ALL, g_cal_async.stage_index);
        ok = 0u;
    }

    if (cal_measured_out_of_range(final_measured)) {
        /* No volver al default: si no hay punto perfecto, igual dejamos el
         * mejor candidato real encontrado y lo marcamos como no OK. */
        ok = 0u;

        if (g_cal_async.stage_index == (PSOC_CAL_STAGE_COUNT - 1u)) {
            /* La ultima etapa alimenta al canal de captura del ADC; avisar si
             * incluso el mejor candidato quedo fuera del rango operativo. */
            cal_diag(PSOC_EVT_CAL_LP_BAD, (uint8)g_cal_async.stage_index);
        }
    }

    final_dac = cal_stage_clamp_dac(stage, final_dac);
    stage->write(final_dac);
    result->final_dac = final_dac;
    result->final_measured = final_measured;
    result->ok = ok;
    if (!ok) {
        g_cal_async.ok = 0u;
    }
    cal_diag(PSOC_EVT_CAL_STAGE_DAC, result->final_dac);
    cal_diag_i16(PSOC_EVT_CAL_STAGE_MEAS, result->final_measured);
    cal_diag_i32(PSOC_EVT_CAL_STAGE_MEAS32, result->final_measured);
    cal_diag(PSOC_EVT_CAL_STAGE_OK, result->ok);

#if CAL_FAIL_FAST_ON_STAGE_FAIL
    if (!ok) {
        g_cal_async.stage_index = PSOC_CAL_STAGE_COUNT;
        g_cal_async.state = CAL_ASYNC_VERIFY_BEGIN;
        return;
    }
#endif

    g_cal_async.stage_index++;
    g_cal_async.state = CAL_ASYNC_STAGE_BEGIN;
}

static void cal_async_complete(void)
{
    ADC_Stop();
    psoc_calibration_restore_capture_path();
#if defined(SYNC_IN_INTSTAT)
    (void)SYNC_IN_ClearInterrupt();
#endif
    isr_SyncIn_ClearPending();
    isr_SyncIn_Enable();
    g_cal_async.busy = 0u;
    g_cal_async.done = 1u;
    g_cal_async.state = CAL_ASYNC_DONE;
}

static uint8 cal_async_realcheck_slope_increasing(void)
{
    const PsocCalStage *stage = &g_psoc_cal_stages[g_cal_async.stage_index];
    if (g_cal_async.slope_known[g_cal_async.stage_index]) {
        return g_cal_async.slope_increasing[g_cal_async.stage_index];
    }
    return (stage->direction >= 0) ? 1u : 0u;
}

static void cal_async_finish_realcheck_stage(void)
{
    const PsocCalStage *stage = &g_psoc_cal_stages[g_cal_async.stage_index];
    PsocCalResult *result = &g_psoc_cal_results[g_cal_async.stage_index];
    uint8 ok = (!g_cal_async.realcheck_current_saturated &&
                g_cal_async.realcheck_current_abs_error <=
                    cal_scale_counts(stage->realcheck.tol_counts)) ? 1u : 0u;

    g_cal_async.realcheck_current_dac =
        cal_stage_clamp_dac(stage, g_cal_async.realcheck_current_dac);
    stage->write(g_cal_async.realcheck_current_dac);
    result->final_dac = g_cal_async.realcheck_current_dac;
    result->final_measured = g_cal_async.realcheck_current_measured;
    result->ok = ok;
    if (!ok) {
        g_cal_async.ok = 0u;
    }
    cal_diag(PSOC_EVT_CAL_REALCHECK_OK, ok);
    g_cal_async.stage_index++;
    g_cal_async.state = CAL_ASYNC_REALCHECK_BEGIN;
}

static void cal_async_plan_realcheck_nudge(void)
{
    const PsocCalStage *stage = &g_psoc_cal_stages[g_cal_async.stage_index];
    uint8 step = stage->realcheck.nudge_step;
    uint8 increasing;
    uint8 go_up;
    int16 next;
    int16 delta;

    if (!g_cal_async.realcheck_current_saturated &&
        g_cal_async.realcheck_current_abs_error <= cal_scale_counts(stage->realcheck.tol_counts)) {
        cal_async_finish_realcheck_stage();
        return;
    }
    if (g_cal_async.realcheck_nudge_count >= stage->realcheck.max_nudges ||
        step == 0u) {
        cal_async_finish_realcheck_stage();
        return;
    }

    increasing = cal_async_realcheck_slope_increasing();
    go_up = increasing
        ? ((g_cal_async.realcheck_current_measured < stage->target_counts) ? 1u : 0u)
        : ((g_cal_async.realcheck_current_measured > stage->target_counts) ? 1u : 0u);
    delta = go_up ? (int16)step : (int16)(-((int16)step));
    next = (int16)g_cal_async.realcheck_current_dac + delta;
    if (next < (int16)cal_stage_min_dac(stage)) {
        next = (int16)cal_stage_min_dac(stage);
    }
    if (next > (int16)cal_stage_max_dac(stage)) {
        next = (int16)cal_stage_max_dac(stage);
    }
    if ((uint8)next == g_cal_async.realcheck_current_dac) {
        cal_async_finish_realcheck_stage();
        return;
    }

    g_cal_async.realcheck_candidate_active = 1u;
    g_cal_async.realcheck_last_nudge = (int8)delta;
    g_cal_async.realcheck_nudge_count++;
    async_measure_begin((uint8)next, CAL_ASYNC_EVAL_REALCHECK,
                        &stage->realcheck.avg,
                        stage->realcheck.nudge_discard_samples,
                        1u, 1u);
}

/* Aborta la calibracion por timeout (CAL_WATCHDOG_TICKS sin terminar).
 * Las etapas ya finalizadas (indices < stage_index durante la busqueda)
 * conservan su mejor valor encontrado; la etapa en curso y las que faltan
 * se fuerzan al centro nominal de cada etapa. Durante la
 * verificacion (todas las etapas ya calibradas) no se toca ningun DAC. */
static void cal_async_abort_watchdog(void)
{
    uint8 i;
    uint8 preserve_results =
        (g_cal_async.state == CAL_ASYNC_VERIFY_BEGIN ||
         g_cal_async.state == CAL_ASYNC_EVAL_VERIFY ||
         g_cal_async.state == CAL_ASYNC_REALCHECK_SWITCH ||
         g_cal_async.state == CAL_ASYNC_REALCHECK_BEGIN ||
         g_cal_async.state == CAL_ASYNC_EVAL_REALCHECK) ? 1u : 0u;

    if (!preserve_results) {
        for (i = g_cal_async.stage_index; i < PSOC_CAL_STAGE_COUNT; i++) {
            uint8 center = cal_stage_center_dac(&g_psoc_cal_stages[i]);
            g_psoc_cal_stages[i].write(center);
            g_psoc_cal_results[i].final_dac = center;
            g_psoc_cal_results[i].final_measured = 0L;
            g_psoc_cal_results[i].ok = 0u;
        }
    }
    g_cal_async.ok = 0u;
    cal_diag(PSOC_EVT_CAL_WATCHDOG, g_cal_async.stage_index);

    cal_async_complete();
}

uint8 psoc_calibration_start_async(void)
{
    uint8 i;

    if (g_cal_async.busy) {
        return 0u;
    }

    isr_SyncIn_Disable();
    ADC_Stop();
    psoc_adc_select_capture_config();
    ADC_Stop();

    g_psoc_cal_result_count = PSOC_CAL_STAGE_COUNT;
    g_cal_async.busy = 1u;
    g_cal_async.done = 0u;
    g_cal_async.ok = 1u;
    g_cal_async.stage_index = 0u;
    g_cal_async.state = CAL_ASYNC_STAGE_BEGIN;
    g_cal_async.start_ticks = psoc_now_ticks();
    g_cal_async.last_progress_ticks = g_cal_async.start_ticks;
    for (i = 0u; i < PSOC_CAL_MAX_STAGES; i++) {
        g_cal_async.slope_known[i] = 0u;
        g_cal_async.slope_increasing[i] = 0u;
    }
    return 1u;
}

uint8 psoc_calibration_async_busy(void)
{
    return g_cal_async.busy;
}

uint8 psoc_calibration_async_result_ok(void)
{
#if PSOC_HW_CLASS == PSOC_HW_GEO
    /* En GEO el criterio de uso para captura es la salida final (GEO_LP).
     * Las etapas intermedias se reportan igual como diagnostico, pero si la
     * cascada termina centrada no queremos marcar la calibracion completa como
     * fallida en el ESP/web. */
    if (g_psoc_cal_result_count >= PSOC_CAL_STAGE_COUNT) {
        return g_psoc_cal_results[PSOC_CAL_STAGE_COUNT - 1u].ok;
    }
#endif
    return g_cal_async.ok;
}

uint8 psoc_calibration_service_async(void)
{
    const PsocCalStage *stage;
    int32 abs_error;

    if (!g_cal_async.busy) {
        return 0u;
    }

    {
        uint32 now = psoc_now_ticks();
        if ((now - g_cal_async.start_ticks) >= CAL_WATCHDOG_TICKS) {
            cal_async_abort_watchdog();
            return 1u;
        }
        if ((now - g_cal_async.last_progress_ticks) >= CAL_PROGRESS_PERIOD_TICKS) {
            g_cal_async.last_progress_ticks = now;
            cal_diag(PSOC_EVT_CAL_PROGRESS, g_cal_async.stage_index);
        }
    }

    switch (g_cal_async.state) {
        case CAL_ASYNC_STAGE_BEGIN:
            if (g_cal_async.stage_index >= PSOC_CAL_STAGE_COUNT) {
                g_cal_async.stage_index = 0u;
                g_cal_async.state = CAL_ASYNC_VERIFY_BEGIN;
                break;
            }
            stage = &g_psoc_cal_stages[g_cal_async.stage_index];
            cal_async_reset_stage_memory();
            cal_diag(PSOC_EVT_CAL_STAGE_BEGIN, g_cal_async.stage_index);
            ADC_Stop();
            CAL_AMUX_ADC_SELECT(stage->adc_channel);
            ADC_Start();
            isr_DelSig_ClearPending();
            ADC_StartConvert();
            async_measure_begin(cal_stage_center_dac(stage), CAL_ASYNC_EVAL_INIT,
                                &stage->avg, stage->settle_samples, 1u, 0u);
            break;

        case CAL_ASYNC_MEASURE:
            (void)async_measure_service();
            break;

        case CAL_ASYNC_EVAL_INIT:
            stage = &g_psoc_cal_stages[g_cal_async.stage_index];
            g_cal_async.base_measured = g_cal_async.measured;
            (void)cal_async_record_measurement(stage);
            if (g_cal_async.best_abs_error <= cal_scale_counts(stage->tolerance_counts)) {
                async_finish_stage(1u);
                break;
            }
            g_cal_async.dac = cal_stage_probe_dac(stage);
            if (g_cal_async.dac == g_cal_async.best_dac) {
                async_finish_stage(0u);
                break;
            }
            async_measure_begin(g_cal_async.dac, CAL_ASYNC_EVAL_PROBE,
                                &stage->avg, stage->settle_samples, 1u, 0u);
            break;

        case CAL_ASYNC_EVAL_PROBE:
            stage = &g_psoc_cal_stages[g_cal_async.stage_index];
            if (cal_async_record_measurement(stage)) {
                cal_diag(PSOC_EVT_CAL_LOOP, g_cal_async.dac);
                async_finish_stage((g_cal_async.best_abs_error <= cal_scale_counts(stage->tolerance_counts)) ? 1u : 0u);
                break;
            }
            if (g_cal_async.best_abs_error <= cal_scale_counts(stage->tolerance_counts)) {
                async_finish_stage(1u);
                break;
            }
            if (cal_scale_counts(g_cal_async.measured) == cal_scale_counts(g_cal_async.base_measured)) {
                g_cal_async.increasing = (stage->direction >= 0) ? 1u : 0u;
            } else {
                g_cal_async.increasing =
                    ((cal_scale_counts(g_cal_async.measured) > cal_scale_counts(g_cal_async.base_measured)) ==
                     (g_cal_async.dac > cal_stage_center_dac(stage))) ? 1u : 0u;
            }
            g_cal_async.slope_known[g_cal_async.stage_index] = 1u;
            g_cal_async.slope_increasing[g_cal_async.stage_index] =
                g_cal_async.increasing;
            g_cal_async.lo = cal_stage_min_dac(stage);
            g_cal_async.hi = cal_stage_max_dac(stage);
            g_cal_async.iter = 0u;
            g_cal_async.state = CAL_ASYNC_PLAN_ITER;
            break;

        case CAL_ASYNC_PLAN_ITER:
            stage = &g_psoc_cal_stages[g_cal_async.stage_index];
            if (g_cal_async.iter >= stage->max_iter) {
                async_finish_stage(0u);
                break;
            }
            {
                int32 cur_error = abs_counts(cal_scale_counts(stage->target_counts) -
                                              cal_scale_counts(g_cal_async.measured));
                uint8 go_up;
                uint8 next_dac;

                if (cur_error <= cal_scale_counts(stage->deadband_counts)) {
                    async_finish_stage(1u);
                    break;
                }

                go_up = g_cal_async.increasing
                    ? ((cal_scale_counts(g_cal_async.measured) < cal_scale_counts(stage->target_counts)) ? 1u : 0u)
                    : ((cal_scale_counts(g_cal_async.measured) > cal_scale_counts(stage->target_counts)) ? 1u : 0u);

                if (go_up) {
                    if (g_cal_async.dac >= cal_stage_max_dac(stage)) {
                        async_finish_stage(0u);
                        break;
                    }
                    g_cal_async.lo = (uint8)(g_cal_async.dac + 1u);
                } else {
                    if (g_cal_async.dac <= cal_stage_min_dac(stage)) {
                        async_finish_stage(0u);
                        break;
                    }
                    g_cal_async.hi = (uint8)(g_cal_async.dac - 1u);
                }
                if (g_cal_async.lo > g_cal_async.hi) {
                    async_finish_stage(0u);
                    break;
                }
                next_dac = (uint8)(g_cal_async.lo + ((g_cal_async.hi - g_cal_async.lo) / 2u));
                next_dac = cal_stage_clamp_dac(stage, next_dac);
                if (next_dac == g_cal_async.dac) {
                    async_finish_stage(0u);
                    break;
                }
                g_cal_async.iter++;
                async_measure_begin(next_dac, CAL_ASYNC_EVAL_ITER,
                                    &stage->avg, stage->settle_samples, 1u, 0u);
            }
            break;

        case CAL_ASYNC_EVAL_ITER:
            stage = &g_psoc_cal_stages[g_cal_async.stage_index];
            if (cal_async_record_measurement(stage)) {
                cal_diag(PSOC_EVT_CAL_LOOP, g_cal_async.dac);
                async_finish_stage((g_cal_async.best_abs_error <= cal_scale_counts(stage->tolerance_counts)) ? 1u : 0u);
                break;
            }
            if (g_cal_async.best_abs_error <= cal_scale_counts(stage->tolerance_counts)) {
                async_finish_stage(1u);
            } else {
                g_cal_async.state = CAL_ASYNC_PLAN_ITER;
            }
            break;

        case CAL_ASYNC_VERIFY_BEGIN:
            if (g_cal_async.stage_index >= PSOC_CAL_STAGE_COUNT) {
                g_cal_async.state = CAL_ASYNC_REALCHECK_SWITCH;
                break;
            }
            stage = &g_psoc_cal_stages[g_cal_async.stage_index];
            cal_diag(PSOC_EVT_CAL_VERIFY_BEGIN, g_cal_async.stage_index);
            ADC_Stop();
            CAL_AMUX_ADC_SELECT(stage->adc_channel);
            ADC_Start();
            isr_DelSig_ClearPending();
            ADC_StartConvert();
            async_measure_begin(g_psoc_cal_results[g_cal_async.stage_index].final_dac,
                                CAL_ASYNC_EVAL_VERIFY,
                                &stage->verify_avg,
                                stage->verify_settle_samples,
                                0u, 0u);
            break;

        case CAL_ASYNC_EVAL_VERIFY:
            stage = &g_psoc_cal_stages[g_cal_async.stage_index];
            {
                PsocCalResult *result = &g_psoc_cal_results[g_cal_async.stage_index];
                uint8 verify_ok;
                uint8 saturated;

                result->final_measured = g_cal_async.measured;
                abs_error = abs_counts(cal_scale_counts(stage->target_counts) -
                                        cal_scale_counts(g_cal_async.measured));
                saturated = cal_stage_saturated(stage, g_cal_async.measured);
                cal_diag(PSOC_EVT_CAL_STAGE_SAT, saturated);
                verify_ok = (!saturated && abs_error <= cal_scale_counts(stage->tolerance_counts)) ? 1u : 0u;
                result->ok = (uint8)(result->ok && verify_ok);
                if (!verify_ok) {
                    g_cal_async.ok = 0u;
                }
                cal_diag(PSOC_EVT_CAL_VERIFY_OK, verify_ok);
                g_cal_async.stage_index++;
                g_cal_async.state = CAL_ASYNC_VERIFY_BEGIN;
            }
            break;

        case CAL_ASYNC_REALCHECK_SWITCH:
            ADC_Stop();
            g_cal_async.stage_index = 0u;
            g_cal_async.state = CAL_ASYNC_REALCHECK_BEGIN;
            break;

        case CAL_ASYNC_REALCHECK_BEGIN:
            if (g_cal_async.stage_index >= PSOC_CAL_STAGE_COUNT) {
                cal_async_complete();
                return 1u;
            }
            stage = &g_psoc_cal_stages[g_cal_async.stage_index];
            if (!stage->realcheck.enable) {
                g_cal_async.stage_index++;
                break;
            }
            g_cal_async.realcheck_candidate_active = 0u;
            g_cal_async.realcheck_nudge_count = 0u;
            g_cal_async.realcheck_current_dac =
                g_psoc_cal_results[g_cal_async.stage_index].final_dac;
            g_cal_async.realcheck_current_dac =
                cal_stage_clamp_dac(stage, g_cal_async.realcheck_current_dac);
            g_cal_async.realcheck_current_measured =
                g_psoc_cal_results[g_cal_async.stage_index].final_measured;
            g_cal_async.realcheck_current_abs_error = 0x7FFFFFFFL;
            g_cal_async.realcheck_current_saturated = 0u;
            g_cal_async.realcheck_last_nudge = 0;
            stage->write(g_cal_async.realcheck_current_dac);
            cal_diag(PSOC_EVT_CAL_REALCHECK_BEGIN, g_cal_async.stage_index);
            ADC_Stop();
            CAL_AMUX_ADC_SELECT(stage->adc_channel);
            ADC_Start();
            isr_DelSig_ClearPending();
            ADC_StartConvert();
            async_measure_begin(g_cal_async.realcheck_current_dac,
                                CAL_ASYNC_EVAL_REALCHECK,
                                &stage->realcheck.avg,
                                stage->realcheck.discard_samples,
                                0u, 1u);
            break;

        case CAL_ASYNC_EVAL_REALCHECK:
            stage = &g_psoc_cal_stages[g_cal_async.stage_index];
            {
                uint8 saturated =
                    cal_stage_saturated(stage, g_cal_async.measured);
                int32 real_abs_error =
                    abs_counts(cal_scale_counts(stage->target_counts) -
                               cal_scale_counts(g_cal_async.measured));

                cal_diag(PSOC_EVT_CAL_STAGE_SAT, saturated);
                if (g_cal_async.realcheck_candidate_active) {
                    if (saturated ||
                        (!g_cal_async.realcheck_current_saturated &&
                         real_abs_error >= g_cal_async.realcheck_current_abs_error)) {
                        stage->write(g_cal_async.realcheck_current_dac);
                        cal_diag(PSOC_EVT_CAL_REALCHECK_NUDGE, 0u);
                        g_cal_async.realcheck_candidate_active = 0u;
                        cal_async_finish_realcheck_stage();
                    } else {
                        g_cal_async.realcheck_current_dac = g_cal_async.dac;
                        g_cal_async.realcheck_current_measured =
                            g_cal_async.measured;
                        g_cal_async.realcheck_current_abs_error =
                            real_abs_error;
                        g_cal_async.realcheck_current_saturated = saturated;
                        cal_diag(PSOC_EVT_CAL_REALCHECK_NUDGE,
                                 (uint8)g_cal_async.realcheck_last_nudge);
                        g_cal_async.realcheck_candidate_active = 0u;
                        cal_async_plan_realcheck_nudge();
                    }
                } else {
                    g_cal_async.realcheck_current_dac = g_cal_async.dac;
                    g_cal_async.realcheck_current_measured = g_cal_async.measured;
                    g_cal_async.realcheck_current_abs_error = real_abs_error;
                    g_cal_async.realcheck_current_saturated = saturated;
                    cal_async_plan_realcheck_nudge();
                }
            }
            break;

        default:
            break;
    }

    return 0u;
}
