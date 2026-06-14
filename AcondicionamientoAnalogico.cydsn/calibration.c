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

/* Reporta al ESP cada cambio de canal de AMux_IN (0=normal/entrada real,
 * 1=referencia/tierra virtual) para poder confirmar por telemetria los
 * cambios de canal durante calibracion y en los estados idle/activo de
 * main.c (ver psoc_calibration_amux_active/psoc_calibration_amux_idle). */
static void cal_amux_in_select(uint8 channel)
{
    CAL_AMUX_IN_SELECT(channel);
    cal_diag(PSOC_EVT_CAL_AMUX_IN, channel);
}

void psoc_calibration_amux_active(void)
{
    cal_amux_in_select(CAL_INPUT_NORMAL_CHANNEL);
}

void psoc_calibration_amux_idle(void)
{
    cal_amux_in_select(CAL_INPUT_REF_CHANNEL);
}

static int32 abs_counts(int32 value)
{
    return (value < 0) ? -value : value;
}

/* Ver CAL_OPERATING_RANGE_COUNTS (calibration_tables.h): +-0.5V absolutos.
 * Una medicion mas alla de esto deja al operacional fuera de rango
 * operativo, sin importar si "ok" hubiera dado 1. */
static uint8 cal_measured_out_of_range(int32 measured)
{
    return (abs_counts(measured) > CAL_OPERATING_RANGE_COUNTS) ? 1u : 0u;
}

PsocCalResult g_psoc_cal_results[PSOC_CAL_MAX_STAGES];
uint8 g_psoc_cal_result_count = 0u;

#define CAL_ASYNC_EMPTY_POLL_LIMIT 2000000UL

/* Watchdog global: si la calibracion no termino en este tiempo (ticks de
 * 10 ms => 200 s), se aborta y se restauran valores seguros. Queda por
 * debajo de PSOC_CAL_ACK_TIMEOUT_MS=240000 del ESP para que el CAL_DONE
 * llegue antes de que el ESP de por perdida la respuesta. */
#define CAL_WATCHDOG_TICKS 20000UL

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
    int32 acc;
    int32 cum_sum;
    int32 cum_count;
    int32 prev_avg;
    uint16 avg_count;
    uint16 discard_count;
    uint8 settle_windows;
    uint8 have_prev_avg;
    uint32 empty_polls;
    uint32 start_ticks;
    uint32 last_progress_ticks;
} PsocCalAsync;

static PsocCalAsync g_cal_async = { CAL_ASYNC_IDLE };

void psoc_calibration_start_references(void)
{
    uint8 i;

    CAL_AMUX_ADC_START();
    CAL_AMUX_IN_START();
    ADC_Stop();
    cal_amux_in_select(CAL_INPUT_NORMAL_CHANNEL);
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
        g_psoc_cal_stages[i].write(CAL_DAC_INIT);
    }
}

/* Deja el AMux_ADC en el canal de captura (GEO_LP) pero con AMux_IN en la
 * referencia/tierra virtual (CAL_INPUT_REF_CHANNEL): este es el estado IDLE,
 * en el que el ADC queda mirando GEO_LP vs. referencia para poder verificar
 * en cualquier momento si el front-end sigue calibrado. main.c
 * (psoc_set_amux_active, en psoc_arm/psoc_start_now) cambia a
 * CAL_INPUT_NORMAL_CHANNEL justo antes de armar/capturar, y vuelve a llamar
 * a esta funcion (vía psoc_set_amux_idle) al terminar. */
void psoc_calibration_restore_capture_path(void)
{
    ADC_Stop();
    cal_amux_in_select(CAL_INPUT_REF_CHANNEL);
    CAL_AMUX_ADC_SELECT(CAL_ADC_CAPTURE_CHANNEL);
    ADC_Start();
    ADC_StopConvert();
}

void psoc_calibration_reset_references(void)
{
    uint8 i;

    ADC_Stop();
    cal_amux_in_select(CAL_INPUT_REF_CHANNEL);
    for (i = 0u; i < PSOC_CAL_STAGE_COUNT; i++) {
        g_psoc_cal_stages[i].write(CAL_DAC_INIT);
        g_psoc_cal_results[i].final_dac = CAL_DAC_INIT;
        g_psoc_cal_results[i].final_measured = 0L;
        g_psoc_cal_results[i].ok = 1u;
    }
    g_psoc_cal_result_count = PSOC_CAL_STAGE_COUNT;
    psoc_calibration_restore_capture_path();
}

static void async_measure_begin(uint8 dac, PsocCalAsyncState after_measure,
                                uint16 settle_samples, uint8 write_dac)
{
    const PsocCalStage *stage = &g_psoc_cal_stages[g_cal_async.stage_index];

    g_cal_async.dac = dac;
    g_cal_async.after_measure = after_measure;
    g_cal_async.acc = 0L;
    g_cal_async.cum_sum = 0L;
    g_cal_async.cum_count = 0L;
    g_cal_async.avg_count = 0u;
    g_cal_async.discard_count = settle_samples;
    g_cal_async.settle_windows = 0u;
    g_cal_async.have_prev_avg = 0u;
    g_cal_async.prev_avg = 0L;
    g_cal_async.empty_polls = 0UL;
    if (write_dac) {
        stage->write(dac);
    }
    psoc_adc_clear_isr_sample();
    g_cal_async.state = CAL_ASYNC_MEASURE;
}

/* Mide promediando ACUMULATIVAMENTE: cada ventana de avg_n muestras se suma
 * al total (cum_sum/cum_count) en vez de descartarse, y "measured" es siempre
 * ese promedio acumulado de baja-ruido (nunca el de una sola ventana). Se
 * sigue acumulando hasta que el promedio acumulado deje de moverse mas de
 * CAL_SETTLE_TOL_COUNTS de una ventana a la siguiente, o hasta
 * CAL_SETTLE_MAX_WINDOWS como limite de seguridad -- en ese caso tambien se
 * devuelve el promedio acumulado (no la ultima ventana), que ya es mucho mas
 * estable que una ventana sola. Esto evita que el ruido de una sola medicion
 * "convenza" a la busqueda binaria de que un punto es mejor de lo que es
 * (corregido 2026-06-14, ver HANDOFF: la busqueda terminaba sobre-ajustando
 * al ruido y saturando GEO_LP). */
static uint8 async_measure_service(void)
{
    int32 sample;
    int32 cum_avg;
    const PsocCalStage *stage = &g_psoc_cal_stages[g_cal_async.stage_index];

    if (!psoc_adc_take_isr_sample(&sample)) {
        g_cal_async.empty_polls++;
        if (g_cal_async.empty_polls >= CAL_ASYNC_EMPTY_POLL_LIMIT) {
            g_cal_async.measured = 0x7FFFL;
            cal_diag_point(g_cal_async.dac, g_cal_async.measured);
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
    if (g_cal_async.avg_count < stage->avg_n) {
        return 0u;
    }

    g_cal_async.cum_sum += g_cal_async.acc;
    g_cal_async.cum_count += (int32)stage->avg_n;
    g_cal_async.acc = 0L;
    g_cal_async.avg_count = 0u;

    cum_avg = g_cal_async.cum_sum / g_cal_async.cum_count;
    g_cal_async.measured = cum_avg;

    if (g_cal_async.have_prev_avg &&
        (abs_counts(cum_avg - g_cal_async.prev_avg) <= CAL_SETTLE_TOL_COUNTS)) {
        cal_diag_point(g_cal_async.dac, g_cal_async.measured);
        g_cal_async.state = g_cal_async.after_measure;
        return 1u;
    }

    g_cal_async.prev_avg = cum_avg;
    g_cal_async.have_prev_avg = 1u;
    g_cal_async.settle_windows++;
    if (g_cal_async.settle_windows >= CAL_SETTLE_MAX_WINDOWS) {
        cal_diag_point(g_cal_async.dac, g_cal_async.measured);
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

    if (cal_measured_out_of_range(final_measured)) {
        /* Aunque "ok" hubiera sido 1, una salida a mas de +-0.5V deja al
         * operacional fuera de rango operativo: preferible quedar sin
         * calibrar (CAL_DAC_INIT, ~2.5V) a arriesgar saturacion en cascada. */
        final_dac = CAL_DAC_INIT;
        final_measured = g_cal_async.base_measured;
        ok = 0u;

        if (g_cal_async.stage_index == (PSOC_CAL_STAGE_COUNT - 1u) &&
            cal_measured_out_of_range(final_measured)) {
            /* Ni siquiera en el punto de partida (2.5V) la ultima etapa
             * (GEO_LP, la que alimenta al canal de captura del ADC) queda en
             * rango: los datos capturados van a ser inutiles. */
            cal_diag(PSOC_EVT_CAL_LP_BAD, (uint8)g_cal_async.stage_index);
        }
    }

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

    g_cal_async.stage_index++;
    g_cal_async.state = CAL_ASYNC_STAGE_BEGIN;
}

/* Aborta la calibracion por timeout (CAL_WATCHDOG_TICKS sin terminar).
 * Las etapas ya finalizadas (indices < stage_index durante la busqueda)
 * conservan su mejor valor encontrado; la etapa en curso y las que faltan
 * se fuerzan a CAL_DAC_INIT (punto medio, no saturado). Durante la
 * verificacion (todas las etapas ya calibradas) no se toca ningun DAC. */
static void cal_async_abort_watchdog(void)
{
    uint8 i;
    uint8 verify_phase = (g_cal_async.state == CAL_ASYNC_VERIFY_BEGIN ||
                           g_cal_async.state == CAL_ASYNC_EVAL_VERIFY) ? 1u : 0u;

    if (!verify_phase) {
        for (i = g_cal_async.stage_index; i < PSOC_CAL_STAGE_COUNT; i++) {
            g_psoc_cal_stages[i].write(CAL_DAC_INIT);
            g_psoc_cal_results[i].final_dac = CAL_DAC_INIT;
            g_psoc_cal_results[i].final_measured = 0L;
            g_psoc_cal_results[i].ok = 0u;
        }
    }
    g_cal_async.ok = 0u;
    cal_diag(PSOC_EVT_CAL_WATCHDOG, g_cal_async.stage_index);

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

uint8 psoc_calibration_start_async(void)
{
    if (g_cal_async.busy) {
        return 0u;
    }

    isr_SyncIn_Disable();
    ADC_Stop();
    psoc_adc_select_capture_config();
    ADC_Stop();
    cal_amux_in_select(CAL_INPUT_REF_CHANNEL);

    g_psoc_cal_result_count = PSOC_CAL_STAGE_COUNT;
    g_cal_async.busy = 1u;
    g_cal_async.done = 0u;
    g_cal_async.ok = 1u;
    g_cal_async.stage_index = 0u;
    g_cal_async.state = CAL_ASYNC_STAGE_BEGIN;
    g_cal_async.start_ticks = psoc_now_ticks();
    g_cal_async.last_progress_ticks = g_cal_async.start_ticks;
    return 1u;
}

uint8 psoc_calibration_async_busy(void)
{
    return g_cal_async.busy;
}

uint8 psoc_calibration_async_result_ok(void)
{
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
            cal_diag(PSOC_EVT_CAL_STAGE_BEGIN, g_cal_async.stage_index);
            ADC_Stop();
            CAL_AMUX_ADC_SELECT(stage->adc_channel);
            ADC_Start();
            isr_DelSig_ClearPending();
            ADC_StartConvert();
            async_measure_begin(CAL_DAC_INIT, CAL_ASYNC_EVAL_INIT,
                                stage->settle_samples, 1u);
            break;

        case CAL_ASYNC_MEASURE:
            (void)async_measure_service();
            break;

        case CAL_ASYNC_EVAL_INIT:
            stage = &g_psoc_cal_stages[g_cal_async.stage_index];
            g_cal_async.base_measured = g_cal_async.measured;
            g_cal_async.best_dac = g_cal_async.dac;
            g_cal_async.best_measured = g_cal_async.measured;
            g_cal_async.best_abs_error = abs_counts(stage->target_counts - g_cal_async.measured);
            if (g_cal_async.best_abs_error <= stage->tolerance_counts) {
                async_finish_stage(1u);
                break;
            }
            if (stage->direction >= 0) {
                g_cal_async.dac = ((uint16)CAL_DAC_INIT + CAL_PROBE_STEP > 255u)
                    ? (uint8)(CAL_DAC_INIT - CAL_PROBE_STEP)
                    : (uint8)(CAL_DAC_INIT + CAL_PROBE_STEP);
            } else {
                g_cal_async.dac = ((int16)CAL_DAC_INIT - (int16)CAL_PROBE_STEP < 0)
                    ? (uint8)(CAL_DAC_INIT + CAL_PROBE_STEP)
                    : (uint8)(CAL_DAC_INIT - CAL_PROBE_STEP);
            }
            async_measure_begin(g_cal_async.dac, CAL_ASYNC_EVAL_PROBE,
                                stage->settle_samples, 1u);
            break;

        case CAL_ASYNC_EVAL_PROBE:
            stage = &g_psoc_cal_stages[g_cal_async.stage_index];
            abs_error = abs_counts(stage->target_counts - g_cal_async.measured);
            if (abs_error < g_cal_async.best_abs_error) {
                g_cal_async.best_abs_error = abs_error;
                g_cal_async.best_dac = g_cal_async.dac;
                g_cal_async.best_measured = g_cal_async.measured;
            }
            if (abs_error <= stage->tolerance_counts) {
                async_finish_stage(1u);
                break;
            }
            if (g_cal_async.measured == g_cal_async.base_measured) {
                g_cal_async.increasing = (stage->direction >= 0) ? 1u : 0u;
            } else {
                g_cal_async.increasing =
                    ((g_cal_async.measured > g_cal_async.base_measured) ==
                     (g_cal_async.dac > CAL_DAC_INIT)) ? 1u : 0u;
            }
            g_cal_async.lo = 0u;
            g_cal_async.hi = 255u;
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
                uint8 go_up = g_cal_async.increasing
                    ? ((g_cal_async.measured < stage->target_counts) ? 1u : 0u)
                    : ((g_cal_async.measured > stage->target_counts) ? 1u : 0u);
                uint8 next_dac;

                if (go_up) {
                    if (g_cal_async.dac == 255u) {
                        async_finish_stage(0u);
                        break;
                    }
                    g_cal_async.lo = (uint8)(g_cal_async.dac + 1u);
                } else {
                    if (g_cal_async.dac == 0u) {
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
                if (next_dac == g_cal_async.dac) {
                    async_finish_stage(0u);
                    break;
                }
                g_cal_async.iter++;
                async_measure_begin(next_dac, CAL_ASYNC_EVAL_ITER,
                                    stage->settle_samples, 1u);
            }
            break;

        case CAL_ASYNC_EVAL_ITER:
            stage = &g_psoc_cal_stages[g_cal_async.stage_index];
            abs_error = abs_counts(stage->target_counts - g_cal_async.measured);
            if (abs_error < g_cal_async.best_abs_error) {
                g_cal_async.best_abs_error = abs_error;
                g_cal_async.best_dac = g_cal_async.dac;
                g_cal_async.best_measured = g_cal_async.measured;
            }
            if (abs_error <= stage->tolerance_counts) {
                async_finish_stage(1u);
            } else {
                g_cal_async.state = CAL_ASYNC_PLAN_ITER;
            }
            break;

        case CAL_ASYNC_VERIFY_BEGIN:
            if (g_cal_async.stage_index >= PSOC_CAL_STAGE_COUNT) {
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
                return 1u;
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
                                stage->verify_settle_samples,
                                0u);
            break;

        case CAL_ASYNC_EVAL_VERIFY:
            stage = &g_psoc_cal_stages[g_cal_async.stage_index];
            {
                PsocCalResult *result = &g_psoc_cal_results[g_cal_async.stage_index];
                uint8 verify_ok;

                result->final_measured = g_cal_async.measured;
                abs_error = abs_counts(stage->target_counts - g_cal_async.measured);
                verify_ok = (abs_error <= stage->tolerance_counts) ? 1u : 0u;
                result->ok = (uint8)(result->ok && verify_ok);
                if (!verify_ok) {
                    g_cal_async.ok = 0u;
                }
                cal_diag(PSOC_EVT_CAL_VERIFY_OK, verify_ok);
                g_cal_async.stage_index++;
                g_cal_async.state = CAL_ASYNC_VERIFY_BEGIN;
            }
            break;

        default:
            break;
    }

    return 0u;
}
