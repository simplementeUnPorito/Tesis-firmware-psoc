#include "calibration.h"
#include "calibration_tables.h"
#include "psoc_adc.h"

#ifndef CAL_AMUX_ADC_START
#define CAL_AMUX_ADC_START() AMux_ADC_Start()
#endif

#ifndef CAL_AMUX_ADC_SELECT
#define CAL_AMUX_ADC_SELECT(channel) AMux_ADC_Select(channel)
#endif

#ifndef CAL_AMUX_IN_START
#define CAL_AMUX_IN_START() AMux_IN_Start()
#endif

#ifndef CAL_AMUX_IN_SELECT
#define CAL_AMUX_IN_SELECT(channel) AMux_IN_Select(channel)
#endif

static int32 abs_counts(int32 value)
{
    return (value < 0) ? -value : value;
}

static uint8 avg_counts(uint8 n, int32 *avg_out)
{
    uint8 i;
    int32 acc = 0;
    int32 sample;
    if (avg_out == (int32 *)0) { return 0u; }
    if (n == 0u) { n = 1u; }
    for (i = 0u; i < n; i++) {
        if (!psoc_adc_wait_fresh_sample(&sample, CAL_ADC_READ_TIMEOUT_US)) {
            return 0u;
        }
        acc += sample;
    }
    *avg_out = acc / (int32)n;
    return 1u;
}

/* Repite avg_counts() hasta que dos ventanas consecutivas difieran <=
 * CAL_SETTLE_TOL_COUNTS (solo queda ruido de cuantizacion), o hasta
 * CAL_SETTLE_MAX_WINDOWS intentos (se usa la ultima ventana medida). */
static uint8 stable_avg(uint8 n, int32 *avg_out)
{
    int32 prev_avg;
    int32 cur_avg;
    uint8 settle_iter;

    if (!avg_counts(n, &prev_avg)) { return 0u; }
    cur_avg = prev_avg;
    for (settle_iter = 0u; settle_iter < CAL_SETTLE_MAX_WINDOWS; settle_iter++) {
        if (!avg_counts(n, &cur_avg)) { return 0u; }
        if (abs_counts(cur_avg - prev_avg) <= CAL_SETTLE_TOL_COUNTS) {
            break;
        }
        prev_avg = cur_avg;
    }
    *avg_out = cur_avg;
    return 1u;
}

/* Busqueda binaria sobre el codigo DAC [0,255]:
 *  1) Arranca siempre en CAL_DAC_INIT (2.5V).
 *  2) Da un primer paso de CAL_PROBE_STEP en la direccion sugerida por
 *     stage->direction y, comparando ambas medidas, determina
 *     empiricamente si la lectura crece o decrece con el codigo DAC.
 *  3) Con esa pendiente, bisecta [0,255] hasta caer dentro de
 *     stage->tolerance_counts o agotar stage->max_iter.
 * Cada medida usa stable_avg(): espera a que el escalon anterior se asiente
 * (ruido de cuantizacion constante) antes de promediar. */
static uint8 calibrate_stage(const PsocCalStage *stage, PsocCalResult *result)
{
    uint8  dac;
    uint8  lo;
    uint8  hi;
    int32  measured;
    int32  abs_error;
    uint8  increasing;
    uint16 iter;
    uint8  best_dac;
    int32  best_measured;
    int32  best_abs_error;

    ADC_Stop();
    CAL_AMUX_ADC_SELECT(stage->adc_channel);
    CyDelay(CAL_AMUX_SETTLE_MS);
    ADC_Start();
    psoc_adc_clear_isr_sample();
    isr_DelSig_ClearPending();
    ADC_StartConvert();

    dac = CAL_DAC_INIT;
    stage->write(dac);
    CyDelay(CAL_DAC_SETTLE_MS);
    if (!stable_avg(stage->avg_n, &measured)) {
        /* 0x7FFF (saturado) en vez de 0: si el ADC no entrega muestras
         * (p.ej. CFG1 no quedo en modo Continuous), la telemetria no debe
         * parecer "0V perfecto" en esta etapa. */
        result->final_dac = dac;
        result->final_measured = 0x7FFFL;
        result->ok = 0u;
        return 0u;
    }

    best_dac = dac;
    best_measured = measured;
    best_abs_error = abs_counts(stage->target_counts - measured);

    if (best_abs_error <= stage->tolerance_counts) {
        result->final_dac = dac;
        result->final_measured = measured;
        result->ok = 1u;
        return 1u;
    }

    /* Primer paso: nudge en la direccion sugerida por la tabla (con guarda
     * de borde [0,255]) para medir la pendiente real. */
    {
        uint8 probe_dac;
        int32 probe_measured;

        if (stage->direction >= 0) {
            probe_dac = ((uint16)dac + CAL_PROBE_STEP > 255u)
                ? (uint8)(dac - CAL_PROBE_STEP)
                : (uint8)(dac + CAL_PROBE_STEP);
        } else {
            probe_dac = ((int16)dac - (int16)CAL_PROBE_STEP < 0)
                ? (uint8)(dac + CAL_PROBE_STEP)
                : (uint8)(dac - CAL_PROBE_STEP);
        }

        stage->write(probe_dac);
        CyDelay(CAL_DAC_SETTLE_MS);
        if (!stable_avg(stage->avg_n, &probe_measured)) {
            stage->write(best_dac);
            CyDelay(CAL_DAC_SETTLE_MS);
            result->final_dac = best_dac;
            result->final_measured = best_measured;
            result->ok = 0u;
            return 0u;
        }

        abs_error = abs_counts(stage->target_counts - probe_measured);
        if (abs_error < best_abs_error) {
            best_abs_error = abs_error;
            best_dac = probe_dac;
            best_measured = probe_measured;
        }
        if (abs_error <= stage->tolerance_counts) {
            result->final_dac = probe_dac;
            result->final_measured = probe_measured;
            result->ok = 1u;
            return 1u;
        }

        if (probe_measured == measured) {
            increasing = (stage->direction >= 0) ? 1u : 0u;
        } else {
            increasing = ((probe_measured > measured) == (probe_dac > dac)) ? 1u : 0u;
        }

        dac = probe_dac;
        measured = probe_measured;
    }

    /* Busqueda binaria sobre [0,255] usando la pendiente empirica. */
    lo = 0u;
    hi = 255u;

    for (iter = 0u; iter < stage->max_iter; iter++) {
        uint8 go_up = increasing
            ? ((measured < stage->target_counts) ? 1u : 0u)
            : ((measured > stage->target_counts) ? 1u : 0u);
        uint8 next_dac;

        if (go_up) {
            if (dac == 255u) { break; }
            lo = (uint8)(dac + 1u);
        } else {
            if (dac == 0u) { break; }
            hi = (uint8)(dac - 1u);
        }
        if (lo > hi) { break; }

        next_dac = (uint8)(lo + ((hi - lo) / 2u));
        if (next_dac == dac) { break; }
        dac = next_dac;

        stage->write(dac);
        CyDelay(CAL_DAC_SETTLE_MS);
        if (!stable_avg(stage->avg_n, &measured)) {
            stage->write(best_dac);
            CyDelay(CAL_DAC_SETTLE_MS);
            result->final_dac = best_dac;
            result->final_measured = best_measured;
            result->ok = 0u;
            return 0u;
        }

        abs_error = abs_counts(stage->target_counts - measured);
        if (abs_error < best_abs_error) {
            best_abs_error = abs_error;
            best_dac = dac;
            best_measured = measured;
        }
        if (abs_error <= stage->tolerance_counts) {
            result->final_dac = dac;
            result->final_measured = measured;
            result->ok = 1u;
            return 1u;
        }
    }

    stage->write(best_dac);
    CyDelay(CAL_DAC_SETTLE_MS);
    result->final_dac = best_dac;
    result->final_measured = best_measured;
    result->ok = 0u;
    return 0u;
}

PsocCalResult g_psoc_cal_results[PSOC_CAL_MAX_STAGES];
uint8 g_psoc_cal_result_count = 0u;

void psoc_calibration_start_references(void)
{
    uint8 i;

    CAL_AMUX_ADC_START();
    CAL_AMUX_IN_START();
    ADC_Stop();
    CAL_AMUX_IN_SELECT(CAL_INPUT_NORMAL_CHANNEL);
    CAL_AMUX_ADC_SELECT(CAL_ADC_CAPTURE_CHANNEL);
    CyDelay(CAL_AMUX_SETTLE_MS);
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
        g_psoc_cal_stages[i].write(CAL_DAC_INIT);
    }
}

void psoc_calibration_restore_capture_path(void)
{
    ADC_Stop();
    CAL_AMUX_IN_SELECT(CAL_INPUT_NORMAL_CHANNEL);
    CAL_AMUX_ADC_SELECT(CAL_ADC_CAPTURE_CHANNEL);
    CyDelay(CAL_AMUX_SETTLE_MS);
    ADC_Start();
    ADC_StopConvert();
}

void psoc_calibration_reset_references(void)
{
    uint8 i;

    ADC_Stop();
    CAL_AMUX_IN_SELECT(CAL_INPUT_REF_CHANNEL);
    CyDelay(CAL_AMUX_SETTLE_MS);
    for (i = 0u; i < PSOC_CAL_STAGE_COUNT; i++) {
        g_psoc_cal_stages[i].write(CAL_DAC_INIT);
        g_psoc_cal_results[i].final_dac = CAL_DAC_INIT;
        g_psoc_cal_results[i].final_measured = 0L;
        g_psoc_cal_results[i].ok = 1u;
    }
    g_psoc_cal_result_count = PSOC_CAL_STAGE_COUNT;
    psoc_calibration_restore_capture_path();
}

uint8 psoc_calibration_run_blocking(void)
{
    uint8 i;
    uint8 ok = 1u;

    /* AMux_IN=1 es la referencia/tierra virtual de 2.5 V. Se calibra cada
     * etapa para que su salida mida 2.5 V contra esa misma referencia. */
    isr_SyncIn_Disable();
    ADC_Stop();
    psoc_adc_select_calibration_config();

    ADC_Stop();
    CAL_AMUX_IN_SELECT(CAL_INPUT_REF_CHANNEL);
    CyDelay(CAL_AMUX_SETTLE_MS);

    g_psoc_cal_result_count = PSOC_CAL_STAGE_COUNT;
    for (i = 0u; i < PSOC_CAL_STAGE_COUNT; i++) {
        if (!calibrate_stage(&g_psoc_cal_stages[i], &g_psoc_cal_results[i])) {
            ok = 0u;
        }
    }

    ADC_Stop();
    psoc_calibration_restore_capture_path();
#if defined(SYNC_IN_INTSTAT)
    (void)SYNC_IN_ClearInterrupt();
#endif
    isr_SyncIn_ClearPending();
    isr_SyncIn_Enable();

    return ok;
}
