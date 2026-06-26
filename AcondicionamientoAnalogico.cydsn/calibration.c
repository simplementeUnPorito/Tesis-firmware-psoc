#include "calibration.h"
#include "calibration_tables.h"
#include "psoc_adc.h"
#include "filter_coeffs.h"
#include "FIR_adquisition.h"
#include "FIR_calibration.h"

/* Wrapper para distinguir que algoritmo esta vivo en este archivo: el PI
 * (cal_pi_*, mas abajo) es el UNICO activo por default. Estas dos macros
 * son los unicos toggles -- todo lo que dice "#if CAL_ALGO_BISECTION_ENABLE"
 * o "#if CAL_ALGO_SERVO_ENABLE" es codigo legacy conservado como referencia,
 * nunca leido por el PI ni mezclado con sus parametros (ver
 * calibration_tables_{geo,hammer}_*.h, que separan lo mismo en sus propios
 * archivos). Poner cualquiera en 1 para reactivar esa ruta -- el codigo esta
 * intacto, no hay que reescribir nada. */
#ifndef CAL_ALGO_BISECTION_ENABLE
#define CAL_ALGO_BISECTION_ENABLE 0
#endif
#ifndef CAL_ALGO_SERVO_ENABLE
#define CAL_ALGO_SERVO_ENABLE 0
#endif

/* El lote HW (PSOC_ADC_LOTE_SAMPLES, psoc_adc.h) debe equivaler exactamente
 * a una ventana de calibración GEO (CAL_AVG_N_GEO_*) — async_measure_service
 * trata cada lote como una ventana ya sumada. Si alguna vez divergen, mejor
 * fallar el build que misconvergir en silencio. */
#if (PSOC_ADC_LOTE_SAMPLES != CAL_AVG_N_GEO_PGA) || \
    (PSOC_ADC_LOTE_SAMPLES != CAL_AVG_N_GEO_BP) || \
    (PSOC_ADC_LOTE_SAMPLES != CAL_AVG_N_GEO_ADDER) || \
    (PSOC_ADC_LOTE_SAMPLES != CAL_AVG_N_GEO_LP)
#error "PSOC_ADC_LOTE_SAMPLES (psoc_adc.h) debe ser igual a CAL_AVG_N_GEO_* (calibration_tables_geo_*.h)"
#endif

/* Canal AMux_ADC del capacitor de filtrado (100nF a Vss, pin AMuxCapacitor en
 * TopDesign). AMux_ADC esta configurado en "All Modes" (AMux_ADC_ATMOSTONE=0,
 * ver AMux_ADC.h), es decir que el HARDWARE no impide tener varios canales
 * conectados a la vez -- la exclusion mutua entre canales de SEÑAL la tiene
 * que garantizar el software. psoc_amux_select_exclusive() es el unico punto
 * de entrada que toca AMux_ADC_Connect/Disconnect para que esa garantia viva
 * en un solo lugar: nunca conecta un canal nuevo sin desconectar antes el
 * anterior, asi que dos canales de señal jamas quedan en paralelo
 * (cortocircuito). El capacitor se trackea aparte y se puede dejar conectado
 * en paralelo a cualquier canal de señal a propósito -- durante la medición
 * de una etapa de calibración ayuda a filtrar ruido chico; durante
 * captura/idle se desconecta para no cargar la señal en vivo con capacitancia
 * extra.
 * El capacitor solo se conecta si el AMux tiene un canal extra despues de
 * las señales reales. En HAMMER actual AMux_ADC tiene solo dos señales
 * (PGA/LP); tratar el ultimo canal como capacitor mezclaba la etapa LP con
 * la PGA durante calibracion. */
#if PSOC_HW_CLASS == PSOC_HW_GEO
#define CAL_AMUX_SIGNAL_CHANNEL_COUNT 4u
#else
#define CAL_AMUX_SIGNAL_CHANNEL_COUNT 2u
#endif

#ifndef CAL_AMUX_HAS_CAP_CHANNEL
#if (AMux_ADC_CHANNELS > CAL_AMUX_SIGNAL_CHANNEL_COUNT)
#define CAL_AMUX_HAS_CAP_CHANNEL 1u
#else
#define CAL_AMUX_HAS_CAP_CHANNEL 0u
#endif
#endif

#ifndef CAL_AMUX_CAP_CHANNEL
#define CAL_AMUX_CAP_CHANNEL ((uint8)(AMux_ADC_CHANNELS - 1u))
#endif

#ifndef CAL_AMUX_CAP_CLEANUP_MS
#define CAL_AMUX_CAP_CLEANUP_MS 2u
#endif

static uint8 g_amux_active_channel = AMux_ADC_NULL_CHANNEL; /* uno de 0..3, o NULL */
static uint8 g_amux_cap_connected  = 0u;

static void psoc_amux_select_exclusive(uint8 channel, uint8 with_cap)
{
    if (g_amux_active_channel != AMux_ADC_NULL_CHANNEL &&
        g_amux_active_channel != channel) {
        AMux_ADC_Disconnect(g_amux_active_channel);
        g_amux_active_channel = AMux_ADC_NULL_CHANNEL;
    }
    if (g_amux_active_channel != channel) {
        AMux_ADC_Connect(channel);
        g_amux_active_channel = channel;
    }

    if (with_cap && CAL_AMUX_HAS_CAP_CHANNEL) {
        if (!g_amux_cap_connected) {
            AMux_ADC_Connect(CAL_AMUX_CAP_CHANNEL);
            g_amux_cap_connected = 1u;
        }
    } else if (g_amux_cap_connected) {
        AMux_ADC_Disconnect(CAL_AMUX_CAP_CHANNEL);
        g_amux_cap_connected = 0u;
    }
}

static void psoc_amux_start(void)
{
    AMux_ADC_Start();   /* desconecta TODO (0..4), estado inicial conocido */
    g_amux_active_channel = AMux_ADC_NULL_CHANNEL;
    g_amux_cap_connected = 0u;
}

static void psoc_amux_disconnect_capacitor(void)
{
#if CAL_AMUX_HAS_CAP_CHANNEL
    AMux_ADC_Disconnect(CAL_AMUX_CAP_CHANNEL);
#endif
    g_amux_cap_connected = 0u;
}

static void psoc_amux_capacitor_cleanup(void)
{
#if CAL_AMUX_HAS_CAP_CHANNEL
    if (g_amux_active_channel != AMux_ADC_NULL_CHANNEL) {
        AMux_ADC_Disconnect(g_amux_active_channel);
        g_amux_active_channel = AMux_ADC_NULL_CHANNEL;
    }
    if (!g_amux_cap_connected) {
        AMux_ADC_Connect(CAL_AMUX_CAP_CHANNEL);
        g_amux_cap_connected = 1u;
    }
    CyDelay(CAL_AMUX_CAP_CLEANUP_MS);
#endif
}

#ifndef CAL_AMUX_ADC_START
#define CAL_AMUX_ADC_START() psoc_amux_start()
#endif

/* Selección de canal para operación normal/idle (canal de captura): sin
 * capacitor, exclusiva contra cualquier otro canal de señal. */
#ifndef CAL_AMUX_ADC_SELECT
#define CAL_AMUX_ADC_SELECT(channel) psoc_amux_select_exclusive((channel), 0u)
#endif

/* Selección de canal para MEDIR una etapa durante calibración: agrega el
 * capacitor de filtrado (AMux_ADC_CHANNELS-1) en paralelo al canal de la
 * etapa, pedido por el usuario para atenuar ruido chico durante la medición. */
#ifndef CAL_AMUX_ADC_SELECT_STAGE
#define CAL_AMUX_ADC_SELECT_STAGE(channel) psoc_amux_select_exclusive((channel), 1u)
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

#if CAL_ALGO_BISECTION_ENABLE /* biseccion legacy -- unico consumidor era async_measure_service/
       * async_finish_stage, comentados mas abajo. */
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
#endif /* #if 0 -- biseccion legacy */

static int32 abs_counts(int32 value)
{
    return (value < 0) ? -value : value;
}

#ifndef CAL_DIAG_SWEEP_ENABLE
#define CAL_DIAG_SWEEP_ENABLE 0u
#endif

#ifndef CAL_DIAG_SWEEP_SETTLE_MS
#define CAL_DIAG_SWEEP_SETTLE_MS 30u
#endif

#ifndef CAL_ADC_DIRECT_WAIT_POLLS
#define CAL_ADC_DIRECT_WAIT_POLLS 60000u
#endif

static int32 cal_adc_read_direct_counts(void)
{
    int32 sample;
    uint16 guard;
    uint8 ready = 0u;

    ADC_StopConvert();
    ADC_StartConvert();
    for (guard = 0u; guard < (uint16)CAL_ADC_DIRECT_WAIT_POLLS; guard++) {
        if (ADC_IsEndConversion(ADC_RETURN_STATUS)) {
            ready = 1u;
            break;
        }
    }
    if (!ready) {
        ADC_StopConvert();
        return 0L;
    }
    sample = ADC_GetResult32();
    ADC_StopConvert();

    return psoc_adc_counts_right_aligned(sample);
}

static int32 cal_pi_compare_counts(int32 measured)
{
#if PSOC_HW_CLASS == PSOC_HW_HAMMER
    return abs_counts(measured);
#else
    return measured;
#endif
}

#if CAL_DIAG_SWEEP_ENABLE
static void cal_diag_sweep_stage(const PsocCalStage *stage)
{
    static const uint8 sweep_dac[3] = { 0u, 128u, 255u };
    uint8 i;

    for (i = 0u; i < 3u; i++) {
        stage->write(sweep_dac[i]);
        CyDelay(CAL_DIAG_SWEEP_SETTLE_MS);
        cal_diag(PSOC_EVT_CAL_SWEEP_DAC, sweep_dac[i]);
        cal_diag_i32(PSOC_EVT_CAL_SWEEP_MEAS32, cal_adc_read_direct_counts());
    }
}
#endif

/* ============================================================
 * BISECCION (legacy, comentada a pedido del usuario): GEO y HAMMER calibran
 * ahora exclusivamente vía el controlador PI (cal_pi_*, mas abajo). Todo lo
 * que sigue marcado "#if 0" es la biseccion vieja — se conserva como
 * referencia, no se borró, pero no compila. Ver calibration_tables.h,
 * bloque "Controlador PI de calibracion", para la ley de control activa.
 * ============================================================ */
#if CAL_ALGO_BISECTION_ENABLE
/* Reduce las cuentas crudas del ADC a una escala comparable con los codigos
 * del VDAC8. El ADC ve un span simplificado de 0..5V (signed +-2.5V alrededor
 * de VDDA/2), mientras el VDAC usa 0..4.080V; por eso al shift de 18->8 bits
 * se le aplica la correccion de spans 5000/4080. */
#define CAL_DAC_SCALE_SHIFT 10
#define CAL_ADC_SPAN_MV    5000L
#define CAL_VDAC_SPAN_MV   4080L

static int32 cal_div_round_signed(int32 value, int32 divisor)
{
    if (value >= 0L) {
        return (value + (divisor / 2L)) / divisor;
    }
    return -(((-value) + (divisor / 2L)) / divisor);
}

static int32 cal_scale_counts(int32 value)
{
    int32 shifted = value >> CAL_DAC_SCALE_SHIFT;
    return cal_div_round_signed(shifted * CAL_ADC_SPAN_MV, CAL_VDAC_SPAN_MV);
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
#endif /* #if 0 -- biseccion legacy */

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

#if CAL_ALGO_BISECTION_ENABLE /* biseccion legacy -- ver nota mas arriba */
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
#endif /* #if 0 -- biseccion legacy */

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

/* Muestras del PI que se envian al ESP durante calibracion. No puede ser cada
 * muestra a 3 kHz: cada valor int32 viaja como 4 eventos UART. */
#ifndef CAL_PI_TELEM_PERIOD
#define CAL_PI_TELEM_PERIOD 256u
#endif

#ifndef CAL_PI_HAMMER_PASS_COUNT
#define CAL_PI_HAMMER_PASS_COUNT 1u
#endif

#if PSOC_HW_CLASS == PSOC_HW_HAMMER
#define CAL_PI_PASS_COUNT CAL_PI_HAMMER_PASS_COUNT
#else
#define CAL_PI_PASS_COUNT 1u
#endif

#ifndef CAL_DIAG_SWEEP_ENABLE
#define CAL_DIAG_SWEEP_ENABLE 0u
#endif

#ifndef CAL_DIAG_SWEEP_SETTLE_MS
#define CAL_DIAG_SWEEP_SETTLE_MS 30u
#endif

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
    uint8 pass_index;
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

#if CAL_ALGO_BISECTION_ENABLE /* biseccion legacy -- ver nota junto a cal_stage_saturated mas arriba */
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
#endif /* #if 0 -- biseccion legacy */

#if CAL_ALGO_SERVO_ENABLE /* servo lento legacy -- comentado a pedido del usuario, ver
       * stubs no-op de psoc_calibration_servo_* mas abajo */
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
#if defined(VDAC_ref_BP_DEFAULT_DATA) || defined(CY_DVDAC_VDAC_ref_BP_H)
    { CAL_SERVO_KP_NUM_GEO_BP,    CAL_SERVO_KI_NUM_GEO_BP,    CAL_SERVO_KI_DIV_GEO_BP,    CAL_SERVO_DEADBAND_GEO_BP,    CAL_SERVO_FINE_STEP_GEO_BP,    CAL_SERVO_RECOVERY_STEP_GEO_BP },
#endif
    { CAL_SERVO_KP_NUM_GEO_ADDER, CAL_SERVO_KI_NUM_GEO_ADDER, CAL_SERVO_KI_DIV_GEO_ADDER, CAL_SERVO_DEADBAND_GEO_ADDER, CAL_SERVO_FINE_STEP_GEO_ADDER, CAL_SERVO_RECOVERY_STEP_GEO_ADDER },
    { CAL_SERVO_KP_NUM_GEO_LP,    CAL_SERVO_KI_NUM_GEO_LP,    CAL_SERVO_KI_DIV_GEO_LP,    CAL_SERVO_DEADBAND_GEO_LP,    CAL_SERVO_FINE_STEP_GEO_LP,    CAL_SERVO_RECOVERY_STEP_GEO_LP },
#else
    { CAL_SERVO_KP_NUM_HAMMER_PGA, CAL_SERVO_KI_NUM_HAMMER_PGA, CAL_SERVO_KI_DIV_HAMMER_PGA, CAL_SERVO_DEADBAND_HAMMER_PGA, CAL_SERVO_FINE_STEP_HAMMER_PGA, CAL_SERVO_RECOVERY_STEP_HAMMER_PGA },
    { CAL_SERVO_KP_NUM_HAMMER_LP,  CAL_SERVO_KI_NUM_HAMMER_LP,  CAL_SERVO_KI_DIV_HAMMER_LP,  CAL_SERVO_DEADBAND_HAMMER_LP,  CAL_SERVO_FINE_STEP_HAMMER_LP,  CAL_SERVO_RECOVERY_STEP_HAMMER_LP },
#endif
};
#endif /* #if 0 -- servo lento legacy */

/* Compartido con cal_pi_run_service (anti-windup del PI de calibracion) a
 * pesar del nombre "servo" -- no se movio/renombro para no aumentar el
 * diff, pero NO es parte del servo lento comentado arriba. */
static int32 cal_servo_clip_integral(int32 value)
{
    if (value > CAL_SERVO_INTEGRAL_LIMIT) { return CAL_SERVO_INTEGRAL_LIMIT; }
    if (value < -CAL_SERVO_INTEGRAL_LIMIT) { return -CAL_SERVO_INTEGRAL_LIMIT; }
    return value;
}

#if CAL_ALGO_SERVO_ENABLE /* servo lento legacy -- ver nota junto a CAL_SERVO_IDLE mas arriba */
static uint16 cal_servo_settle_samples(uint8 stage_index)
{
#if PSOC_HW_CLASS == PSOC_HW_GEO
    switch (stage_index) {
        case 0u: return CAL_SERVO_SETTLE_SAMPLES_GEO_PGA;
#if defined(VDAC_ref_BP_DEFAULT_DATA) || defined(CY_DVDAC_VDAC_ref_BP_H)
        case 1u: return CAL_SERVO_SETTLE_SAMPLES_GEO_BP;
        case 2u: return CAL_SERVO_SETTLE_SAMPLES_GEO_ADDER;
#else
        case 1u: return CAL_SERVO_SETTLE_SAMPLES_GEO_ADDER;
#endif
        default: return CAL_SERVO_SETTLE_SAMPLES_GEO_LP;
    }
#else
    switch (stage_index) {
        case 0u: return CAL_SERVO_SETTLE_SAMPLES_HAMMER_PGA;
        default: return CAL_SERVO_SETTLE_SAMPLES_HAMMER_LP;
    }
#endif
}
#endif /* #if 0 -- servo lento legacy */

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

uint8 psoc_calibration_stage_count(void)
{
    return PSOC_CAL_STAGE_COUNT;
}

void psoc_calibration_seed_dac(const uint8 *dac_values, uint8 count)
{
    uint8 i;
    for (i = 0u; i < count && i < PSOC_CAL_STAGE_COUNT; i++) {
        cal_stage_write_result(i, dac_values[i]);
    }
}

void psoc_calibration_report_adc_snapshot(void)
{
    uint8 i;

    cal_diag(PSOC_EVT_BOOT, PSOC_HW_CLASS);

    for (i = 0u; i < PSOC_CAL_STAGE_COUNT; i++) {
        const PsocCalStage *stage = &g_psoc_cal_stages[i];
        uint8 saved_dac = cal_stage_current_dac(i);

        ADC_Stop();
        CAL_AMUX_ADC_SELECT(stage->adc_channel);
        ADC_Start();

        stage->write(saved_dac);
        CyDelay(CAL_DIAG_SWEEP_SETTLE_MS);
        cal_diag(PSOC_EVT_ADC_SNAPSHOT_BEGIN, i);
        cal_diag_i32(PSOC_EVT_CAL_STAGE_TARGET32, stage->target_counts);
        cal_diag(PSOC_EVT_CAL_STAGE_DAC, saved_dac);
        cal_diag_i32(PSOC_EVT_ADC_RAW32, cal_adc_read_direct_counts());

#if CAL_DIAG_SWEEP_ENABLE
        cal_diag_sweep_stage(stage);
        stage->write(saved_dac);
        CyDelay(CAL_DIAG_SWEEP_SETTLE_MS);
        cal_diag(PSOC_EVT_CAL_STAGE_DAC, saved_dac);
        cal_diag_i32(PSOC_EVT_ADC_RAW32, cal_adc_read_direct_counts());
#endif
    }

    psoc_calibration_restore_capture_path();
}

#if CAL_ALGO_SERVO_ENABLE /* servo lento legacy -- ver nota junto a CAL_SERVO_IDLE mas arriba.
       * psoc_calibration_servo_enable/enabled/abort/service (API publica,
       * llamada desde main.c) tienen stubs no-op mas abajo; sus cuerpos
       * reales (que usaban estas funciones) tambien quedaron comentados,
       * ver el segundo bloque "#if 0 -- servo lento legacy". */
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
    CAL_AMUX_ADC_SELECT_STAGE(stage->adc_channel);
    cal_diag(PSOC_EVT_CAL_AMUX_IN, stage->adc_channel);
#if CAL_AMUX_HAS_CAP_CHANNEL
    cal_diag(PSOC_EVT_CAL_AMUX_CAP, CAL_AMUX_CAP_CHANNEL);
#endif
    ADC_Start();
    isr_DMA_DelSig_RAM_ClearPending();
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
#endif /* #if 0 -- servo lento legacy */

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
#if defined(VDAC_ref_BP_DEFAULT_DATA) || defined(CY_DVDAC_VDAC_ref_BP_H)
    VDAC_ref_BP_Start();
#endif
    VDAC_Ref_Adder_Start();
    VDAC_ref_LP_Start();
#else
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

#if CAL_ALGO_SERVO_ENABLE /* servo lento legacy -- comentado a pedido del usuario (calibracion
       * pasa a ser PI-only, ver bloque "Controlador PI de calibracion" mas
       * abajo). Stubs no-op de la API publica justo despues de este bloque. */
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
#endif /* #if 0 -- servo lento legacy */

/* Stubs no-op: el servo lento de mantenimiento esta comentado (arriba) a
 * pedido del usuario -- calibracion es 100% PI ahora. Se mantiene la firma
 * publica porque main.c los llama incondicionalmente. */
void psoc_calibration_servo_enable(uint8 enable)
{
    (void)enable;
}

uint8 psoc_calibration_servo_enabled(void)
{
    return 0u;
}

void psoc_calibration_servo_abort(void)
{
}

uint8 psoc_calibration_servo_service(void)
{
    return 0u;
}

#if CAL_ALGO_BISECTION_ENABLE /* biseccion legacy -- ver nota junto a cal_stage_saturated mas arriba */
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
    /* discard_samples llega en unidades de muestra cruda; el lote HW entrega
     * ventanas completas, así que se convierte a lotes-a-descartar con
     * redondeo hacia arriba (nunca menos descarte que antes). */
    g_cal_async.discard_count = (uint16)((discard_samples + (PSOC_ADC_LOTE_SAMPLES - 1u)) /
                                          PSOC_ADC_LOTE_SAMPLES);
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
    psoc_adc_clear_isr_window();
    g_cal_async.state = CAL_ASYNC_MEASURE;
}

/* Mide con una ventana deslizante de peso constante: cada ventana de avg_n
 * muestras entra a un buffer circular de window_count sumas. El promedio se
 * calcula sobre las ultimas avg_n*window_count muestras; solo se considera
 * estable cuando ese promedio varia poco durante stable_streak comparaciones
 * consecutivas, con el buffer ya lleno. */
static uint8 async_measure_service(void)
{
    int32 win_sum;
    uint8 win_n;
    int32 sliding_avg;
    const PsocCalAvgCfg *cfg = g_cal_async.avg_cfg;
    uint8 avg_n = cal_avg_cfg_avg_n(cfg);
    uint8 window_count = cal_avg_cfg_window_count(cfg);
    uint16 max_samples = cal_avg_cfg_max_samples(cfg);

    if (!psoc_adc_take_isr_window(&win_sum, &win_n)) {
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

    /* win_n solo puede valer PSOC_ADC_LOTE_SAMPLES (garantizado por el
     * #error de arriba == avg_n en todo build posible); se ignora a
     * propósito en vez de ramificar de forma defensiva sin salida sensata. */
    (void)win_n;
    g_cal_async.acc = win_sum;
    g_cal_async.avg_count = avg_n;

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
#endif /* #if 0 -- biseccion legacy */

/* Compartido con cal_pi_finish_stage -- cierra la corrida de calibracion
 * (restaura AMux/captura, re-habilita isr_SyncIn) sin importar que
 * controlador la corrio. */
static void cal_async_complete(void)
{
    ADC_Stop();
    psoc_amux_capacitor_cleanup();
    psoc_amux_disconnect_capacitor();
    psoc_calibration_restore_capture_path();
    /* Restaura el FIR de adquisicion -- cal_pi_start() cargo el de
     * calibracion al empezar esta corrida; fuera de una calibracion el
     * Filter de hardware debe volver a tener el diseño de adquisicion. */
    (void)psoc_filter_load_fir_coefficients(g_fir_adquisition_coeffs_q23, FILTER_FIR_NTAPS);
#if defined(SYNC_IN_INTSTAT)
    (void)SYNC_IN_ClearInterrupt();
#endif
    isr_SyncIn_ClearPending();
    isr_SyncIn_Enable();
    g_cal_async.busy = 0u;
    g_cal_async.done = 1u;
    g_cal_async.state = CAL_ASYNC_DONE;
}

#if CAL_ALGO_BISECTION_ENABLE /* biseccion legacy -- ver nota junto a cal_stage_saturated mas arriba */
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
#endif /* #if 0 -- biseccion legacy */

/* Aborta la calibracion por timeout (CAL_WATCHDOG_TICKS sin terminar).
 * Compartido con el PI (psoc_calibration_service_async la llama antes de
 * ramificar). Las etapas ya finalizadas (indices < stage_index) conservan su
 * mejor valor encontrado; la etapa en curso y las que faltan se fuerzan al
 * centro nominal de cada etapa. La condicion preserve_results es reliquia de
 * la biseccion (estados de verify/realcheck que el PI nunca setea) -- queda
 * inerte pero no rompe nada, no afecta el comportamiento. */
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

/* ============================================================
 * Controlador PI de calibracion: UNICO algoritmo activo (CAL_ALGO_BISECTION_
 * ENABLE=0 mas arriba desactiva la biseccion vieja, que queda intacta como
 * referencia). Puerto directo de Subsystem_step() en
 * src/matlab/Simulaciones Controladores/Desacople/Subsystem_grt_rtw/Subsystem.c
 * -- ESE es el diseño de referencia, no una variacion: misma ley PI
 * posicional, sin paso de "refine"/promediado al cerrar la etapa. Toda
 * la suavizacion viene del FIR de hardware (Filter, FIR_calibration.h) leido
 * via DMA_Filter_RAM -- nunca un promedio de software.
 *
 * Superficie de ajuste por etapa, A PROPOSITO reducida a esto y nada mas
 * (ver calibration_tables_{geo,hammer}_*.h): Kp (num/div), Ki (num/div),
 * ganancia absoluta VDAC->medida y samples para lock (M muestras en la misma
 * celda de error cuantizado). target_mv y adelanto_mv son aparte (planteo del
 * problema, no ganancias del algoritmo).
 *
 * Reutiliza g_cal_async.busy/done/ok/stage_index/start_ticks (watchdog) y
 * cal_async_complete/cal_async_abort_watchdog tal cual: para el resto del
 * firmware (EEPROM, diagnostico UART, ESP/web) esto es indistinguible de la
 * biseccion que reemplaza.
 * ============================================================ */

typedef enum {
    CAL_PI_STAGE_BEGIN = 0u,
    CAL_PI_SETTLE,
    CAL_PI_RUN
} PsocCalPiState;

typedef struct {
    int32 kp_num;
    int32 kp_div;
    int32 ki_num;
    int32 ki_div;
    int32 gain_x1000;       /* ganancia fija VDAC->medida; 0 = dinamica por etapa */
    uint16 lock_samples;    /* M muestras en la misma celda de error */
} PsocCalPiCfg;

typedef struct {
    PsocCalPiState state;
    int32 integral;
    int32 last_fir_output;
    int32 last_error_dac;
    int32 last_error_bucket;
    uint16 samples_taken;
    uint16 stable_count;
    uint16 settle_remaining;
    uint8 have_last_error;
    uint8 last_dac_target;
    uint8 dac_current;
    uint32 empty_polls;
} PsocCalPi;

static PsocCalPi g_cal_pi;

/* Division entera con redondeo al mas cercano (no truncado hacia 0). */
static int32 cal_round_div_i64(int64 num, int64 den)
{
    int64 half;

    if (den == 0LL) {
        den = 1LL;
    }
    half = (den < 0LL) ? -den / 2LL : den / 2LL;
    if ((num < 0LL) != (den < 0LL)) {
        return (num - half) / den;
    }
    return (num + half) / den;
}

static int32 cal_pi_clip_integral(int32 value)
{
    if (value > CAL_PI_INTEGRAL_LIMIT) { return CAL_PI_INTEGRAL_LIMIT; }
    if (value < -CAL_PI_INTEGRAL_LIMIT) { return -CAL_PI_INTEGRAL_LIMIT; }
    return value;
}

#if PSOC_HW_CLASS == PSOC_HW_GEO
static const PsocCalPiCfg g_cal_pi_cfg[PSOC_CAL_STAGE_COUNT] = {
    { CAL_PI_KP_NUM_GEO_PGA,   CAL_PI_KP_DIV_GEO_PGA,   CAL_PI_KI_NUM_GEO_PGA,   CAL_PI_KI_DIV_GEO_PGA,   CAL_PI_GAIN_GEO_PGA_X1000,   CAL_PI_LOCK_SAMPLES_GEO_PGA },
#if defined(VDAC_ref_BP_DEFAULT_DATA) || defined(CY_DVDAC_VDAC_ref_BP_H)
    { CAL_PI_KP_NUM_GEO_BP,    CAL_PI_KP_DIV_GEO_BP,    CAL_PI_KI_NUM_GEO_BP,    CAL_PI_KI_DIV_GEO_BP,    CAL_PI_GAIN_GEO_BP_X1000,    CAL_PI_LOCK_SAMPLES_GEO_BP },
#endif
    { CAL_PI_KP_NUM_GEO_ADDER, CAL_PI_KP_DIV_GEO_ADDER, CAL_PI_KI_NUM_GEO_ADDER, CAL_PI_KI_DIV_GEO_ADDER, CAL_PI_GAIN_GEO_ADDER_X1000, CAL_PI_LOCK_SAMPLES_GEO_ADDER },
    { CAL_PI_KP_NUM_GEO_LP,    CAL_PI_KP_DIV_GEO_LP,    CAL_PI_KI_NUM_GEO_LP,    CAL_PI_KI_DIV_GEO_LP,    CAL_PI_GAIN_GEO_LP_X1000,    CAL_PI_LOCK_SAMPLES_GEO_LP },
};
#else
static const PsocCalPiCfg g_cal_pi_cfg[PSOC_CAL_STAGE_COUNT] = {
    { CAL_PI_KP_NUM_HAMMER_PGA, CAL_PI_KP_DIV_HAMMER_PGA, CAL_PI_KI_NUM_HAMMER_PGA, CAL_PI_KI_DIV_HAMMER_PGA, CAL_PI_GAIN_HAMMER_PGA_X1000, CAL_PI_LOCK_SAMPLES_HAMMER_PGA },
    { CAL_PI_KP_NUM_HAMMER_LP,  CAL_PI_KP_DIV_HAMMER_LP,  CAL_PI_KI_NUM_HAMMER_LP,  CAL_PI_KI_DIV_HAMMER_LP,  CAL_PI_GAIN_HAMMER_LP_X1000,  CAL_PI_LOCK_SAMPLES_HAMMER_LP },
};
#endif

static int32 cal_counts_error_to_dac_scale(int32 error_counts)
{
    return cal_round_div_i64((int64)error_counts * (int64)CAL_ADC_SPAN_MV * (int64)CAL_VDAC_CODE_MAX,
                             (int64)CAL_ADC_FULL_SCALE_COUNTS * (int64)CAL_VDAC_SPAN_MV);
}

static int32 cal_pi_stage_gain_x1000(uint8 stage_index)
{
    if (stage_index == 0u) {
        int32 pga_gain = (int32)psoc_hw_pga_gain_x1000();
        return 1000L - pga_gain;
    }
    return g_cal_pi_cfg[stage_index].gain_x1000;
}

static int32 cal_pi_deadband_dac_codes(uint8 stage_index)
{
    int32 gain_x1000 = cal_pi_stage_gain_x1000(stage_index);
    int64 num;
    int64 den;
    int32 deadband;

    if (gain_x1000 < 0L) {
        gain_x1000 = -gain_x1000;
    }
    num = (int64)gain_x1000 * (int64)CAL_PI_DEADBAND_MARGIN_NUM;
    den = 1000LL * (int64)CAL_PI_DEADBAND_MARGIN_DEN;
    deadband = (int32)((num + den - 1LL) / den);
    if (deadband < CAL_PI_DEADBAND_MIN_DAC_CODES) {
        deadband = CAL_PI_DEADBAND_MIN_DAC_CODES;
    }
    return deadband;
}

static int32 cal_pi_error_bucket(int32 error_dac, int32 deadband_dac)
{
    int32 abs_error;
    int32 span;
    int32 bucket;

    span = (deadband_dac <= 0L) ? 1L : deadband_dac;
    abs_error = abs_counts(error_dac);
    if (abs_error <= span) {
        return 0L;
    }
    bucket = 1L + ((abs_error - span - 1L) / span);
    return (error_dac < 0L) ? -bucket : bucket;
}

static uint8 cal_pi_measurement_valid(int32 measured)
{
    return (abs_counts(measured) <= CAL_ADC_FULL_SCALE_COUNTS) ? 1u : 0u;
}

static int32 cal_pi_gain_scaled_term(int32 value, int32 num, int32 div, int32 gain_x1000)
{
    int32 sign = 1L;

    if (div == 0L) {
        div = 1L;
    }
    if (gain_x1000 < 0L) {
        sign = -1L;
        gain_x1000 = -gain_x1000;
    }
    if (gain_x1000 == 0L) {
        gain_x1000 = 1000L;
    }
    return sign * cal_round_div_i64((int64)value * (int64)num * 1000LL,
                                    (int64)div * (int64)gain_x1000);
}

static int8 cal_pi_effort_delta_sign(int32 control_error, int8 direction, int32 gain_x1000)
{
    int8 sign;

    if (control_error == 0L || direction == 0 || gain_x1000 == 0L) {
        return 0;
    }

    sign = (control_error > 0L) ? 1 : -1;
    if (direction < 0) {
        sign = (int8)-sign;
    }
    if (gain_x1000 < 0L) {
        sign = (int8)-sign;
    }
    return sign;
}

/* Cierra la etapa con lo que el lazo ya tiene -- sin promediar, sin barrer
 * candidatos vecinos. ok llega del llamador (cal_pi_run_service): 1 si
 * lockeo por M muestras en la misma celda de error cuantizado, 0 si se rindio
 * por timeout. */
static uint8 cal_pi_finish_stage(uint8 ok)
{
    const PsocCalStage *stage = &g_psoc_cal_stages[g_cal_async.stage_index];
    PsocCalResult *result = &g_psoc_cal_results[g_cal_async.stage_index];
    uint8 final_pass;
    uint8 final_dac = g_cal_pi.dac_current;

    if (!ok && (final_dac == cal_stage_min_dac(stage) ||
                final_dac == cal_stage_max_dac(stage))) {
        final_dac = cal_stage_center_dac(stage);
        stage->write(final_dac);
    }

    result->final_dac = final_dac;
    result->final_measured = g_cal_pi.last_fir_output;
    result->ok = ok;
    final_pass = ((uint8)(g_cal_async.pass_index + 1u) >= (uint8)CAL_PI_PASS_COUNT) ? 1u : 0u;
    if (!ok && final_pass) {
        g_cal_async.ok = 0u;
    }

    cal_diag(PSOC_EVT_CAL_STAGE_DAC, result->final_dac);
    cal_diag_i16(PSOC_EVT_CAL_STAGE_MEAS, result->final_measured);
    cal_diag_i32(PSOC_EVT_CAL_STAGE_MEAS32, result->final_measured);
    cal_diag(PSOC_EVT_CAL_STAGE_OK, result->ok);

    g_cal_async.stage_index++;
    if (g_cal_async.stage_index >= PSOC_CAL_STAGE_COUNT) {
        if (!final_pass) {
            g_cal_async.pass_index++;
            g_cal_async.stage_index = 0u;
            g_cal_pi.state = CAL_PI_STAGE_BEGIN;
            return 0u;
        }
        cal_async_complete();
        return 1u;
    }
    g_cal_pi.state = CAL_PI_STAGE_BEGIN;
    return 0u;
}

static void cal_pi_stage_begin(void)
{
    const PsocCalStage *stage = &g_psoc_cal_stages[g_cal_async.stage_index];
    int32 stage_gain_x1000;
    int32 deadband_dac;

    cal_diag(PSOC_EVT_CAL_STAGE_BEGIN, g_cal_async.stage_index);
    cal_diag_i32(PSOC_EVT_CAL_STAGE_TARGET32, stage->target_counts);
    stage_gain_x1000 = cal_pi_stage_gain_x1000(g_cal_async.stage_index);
    deadband_dac = cal_pi_deadband_dac_codes(g_cal_async.stage_index);
    cal_diag_i32(PSOC_EVT_CAL_PI_GAIN32, stage_gain_x1000);
    cal_diag(PSOC_EVT_CAL_PI_DEADBAND,
             (deadband_dac > 255L) ? 255u : (uint8)deadband_dac);

    g_cal_pi.integral = 0L;
    g_cal_pi.samples_taken = 0u;
    g_cal_pi.stable_count = 0u;
    g_cal_pi.have_last_error = 0u;
    g_cal_pi.last_error_dac = 0L;
    g_cal_pi.last_error_bucket = 0L;
    g_cal_pi.last_dac_target = 0u;
    g_cal_pi.last_fir_output = 0L;
    g_cal_pi.empty_polls = 0UL;
    g_cal_pi.settle_remaining = (uint16)CAL_PI_FIR_SETTLE_SAMPLES;
    g_cal_pi.dac_current = cal_stage_center_dac(stage);
    stage->write(g_cal_pi.dac_current);

    ADC_Stop();
    CAL_AMUX_ADC_SELECT_STAGE(stage->adc_channel);
    cal_diag(PSOC_EVT_CAL_AMUX_IN, stage->adc_channel);
#if CAL_AMUX_HAS_CAP_CHANNEL
    cal_diag(PSOC_EVT_CAL_AMUX_CAP, CAL_AMUX_CAP_CHANNEL);
#endif
    ADC_Start();

    stage->write(g_cal_pi.dac_current);

    /* El PI no promedia/EMA en software: extrae el DC con el FIR de hardware
     * (Canal A del Filter, coeficientes de FIR_calibration.h cargados en
     * cal_pi_start) -- ADC -> Filter_STAGEA -> DMA_Filter_RAM, leido muestra
     * a muestra en cal_pi_run_service. */
    dma_route_select(1u);
    psoc_filter_reset_history();
    psoc_adc_clear_isr_filtered_sample();
    isr_DMA_Filter_RAM_ClearPending();
    ADC_StartConvert();

    g_cal_pi.state = (g_cal_pi.settle_remaining == 0u) ? CAL_PI_RUN : CAL_PI_SETTLE;
}

static uint8 cal_pi_settle_service(void)
{
    int32 sample;

    if (!psoc_adc_take_isr_filtered_sample(&sample)) {
        g_cal_pi.empty_polls++;
        if (g_cal_pi.empty_polls >= CAL_ASYNC_EMPTY_POLL_LIMIT) {
            return cal_pi_finish_stage(0u);
        }
        return 0u;
    }

    g_cal_pi.empty_polls = 0UL;
    g_cal_pi.last_fir_output = psoc_adc_counts_right_aligned(sample);

    if (g_cal_pi.settle_remaining > 0u) {
        g_cal_pi.settle_remaining--;
    }
    if (g_cal_pi.settle_remaining == 0u) {
        g_cal_pi.integral = 0L;
        g_cal_pi.samples_taken = 0u;
        g_cal_pi.stable_count = 0u;
        g_cal_pi.have_last_error = 0u;
        g_cal_pi.last_error_dac = 0L;
        g_cal_pi.last_error_bucket = 0L;
        g_cal_pi.empty_polls = 0UL;
        g_cal_pi.state = CAL_PI_RUN;
    }

    return 0u;
}

static uint8 cal_pi_run_service(void)
{
    const PsocCalStage *stage = &g_psoc_cal_stages[g_cal_async.stage_index];
    const PsocCalPiCfg *cfg = &g_cal_pi_cfg[g_cal_async.stage_index];
    int32 sample;
    int32 control_sample;
    int32 error_counts;
    int32 error_dac;
    int32 control_error;
    int32 effort;
    int32 p_term;
    int32 i_term;
    int32 stage_gain_x1000;
    int32 deadband_dac;
    int32 error_bucket;
    int32 dac_max_step_up;
    int32 dac_max_step_down;
    uint16 sample_index;
    uint16 lock_n;
    uint8 dac_sample;
    uint8 dac_lo;
    uint8 dac_hi;
    uint8 dac_target;
    uint8 dac_step;
    uint8 can_integrate;
    uint8 dac_changed;
    int8 effort_delta_sign;

    if (!psoc_adc_take_isr_filtered_sample(&sample)) {
        g_cal_pi.empty_polls++;
        if (g_cal_pi.empty_polls >= CAL_ASYNC_EMPTY_POLL_LIMIT) {
            return cal_pi_finish_stage(0u);
        }
        return 0u;
    }
    g_cal_pi.empty_polls = 0UL;

    /* El Filter de hardware lee de ADC_DEC_SAMP_PTR igual que el camino
     * crudo -- aplicar el mismo ajuste de escala (psoc_adc_counts_right_aligned,
     * ver bug ya encontrado una vez con esto) antes de comparar contra
     * target_counts. */
    sample = psoc_adc_counts_right_aligned(sample);

    /* Sin promediado/EMA en software: el FIR de hardware (Canal A del
     * Filter, FIR_calibration.h) ya extrajo el DC -- fir_output es
     * directamente la muestra filtrada. */
    g_cal_pi.last_fir_output = sample;
    control_sample = cal_pi_compare_counts(g_cal_pi.last_fir_output);
    error_counts = stage->target_counts - control_sample;
    error_dac = cal_counts_error_to_dac_scale(error_counts);
    stage_gain_x1000 = cal_pi_stage_gain_x1000(g_cal_async.stage_index);
    deadband_dac = cal_pi_deadband_dac_codes(g_cal_async.stage_index);
    control_error = (abs_counts(error_dac) <= deadband_dac) ? 0L : error_dac;
    error_bucket = cal_pi_error_bucket(error_dac, deadband_dac);
    dac_sample = g_cal_pi.dac_current;

    dac_lo = cal_stage_min_dac(stage);
    dac_hi = cal_stage_max_dac(stage);
    can_integrate = 0u;

    if (stage_gain_x1000 == 0L) {
        /* PGA con ganancia directa 1 implica ganancia efectiva VDAC->medida 0
         * (1 - GainDirecta). No hay autoridad fisica para corregir, asi que
         * no se debe perseguir el error hasta saturar el DAC. */
        effort = (int32)dac_sample;
    } else if (control_error == 0L) {
        g_cal_pi.integral = 0L;
        effort = (int32)dac_sample;
    } else {
        /* PI posicional en escala DAC: error_counts -> error_dac antes de
         * entrar al PI; P/I se dividen por la ganancia fisica VDAC->medida.
         * En los PGA esa ganancia es firmada y dinamica: 1 - GainDirecta. */
        p_term = cal_pi_gain_scaled_term(control_error, cfg->kp_num, cfg->kp_div, stage_gain_x1000);
        i_term = cal_pi_gain_scaled_term(g_cal_pi.integral, cfg->ki_num, cfg->ki_div, stage_gain_x1000);
        effort = (int32)stage->dac_center + (int32)stage->direction * (p_term + i_term);
    }

    effort_delta_sign = cal_pi_effort_delta_sign(control_error, stage->direction, stage_gain_x1000);

    if (effort < (int32)dac_lo) {
        dac_target = dac_lo;
        can_integrate = (effort_delta_sign > 0) ? 1u : 0u;
    } else if (effort > (int32)dac_hi) {
        dac_target = dac_hi;
        can_integrate = (effort_delta_sign < 0) ? 1u : 0u;
    } else {
        dac_target = (uint8)effort;
        can_integrate = 1u;
    }

    dac_step = (uint8)CAL_PI_MAX_DAC_STEP_PER_SAMPLE;
    if (dac_step == 0u) {
        dac_step = 1u;
    }
    dac_max_step_up = (int32)dac_sample + (int32)dac_step;
    dac_max_step_down = (int32)dac_sample - (int32)dac_step;
    if ((int32)dac_target > dac_max_step_up) {
        dac_target = (dac_max_step_up > 255L) ? 255u : (uint8)dac_max_step_up;
    } else if ((int32)dac_target < dac_max_step_down) {
        dac_target = (dac_max_step_down < 0L) ? 0u : (uint8)dac_max_step_down;
    }
    dac_changed = (dac_target != dac_sample) ? 1u : 0u;

    /* anti-windup tipo clamping y zona muerta: dentro de deadband no se
     * acumula error para no perseguir pasos imposibles del VDAC. */
    if (can_integrate && stage_gain_x1000 != 0L && control_error != 0L) {
        g_cal_pi.integral = cal_pi_clip_integral(g_cal_pi.integral + control_error);
    }

    /* Lock por M muestras: dentro de deadband cierra OK. Fuera de deadband,
     * mantener el mismo bucket no alcanza para cerrar mientras la integral
     * todavia puede acumular el siguiente LSB de DAC. Un cambio efectivo de
     * DAC reinicia el contador de estabilidad. */
    lock_n = (cfg->lock_samples == 0u || cfg->lock_samples > CAL_PI_LOCK_N_MAX)
        ? CAL_PI_LOCK_N_MAX : cfg->lock_samples;

    if (!g_cal_pi.have_last_error ||
        error_bucket != g_cal_pi.last_error_bucket ||
        dac_changed) {
        g_cal_pi.stable_count = 1u;
    } else if (g_cal_pi.stable_count < 65535u) {
        g_cal_pi.stable_count++;
    }
    g_cal_pi.have_last_error = 1u;
    g_cal_pi.last_error_dac = error_dac;
    g_cal_pi.last_error_bucket = error_bucket;
    g_cal_pi.last_dac_target = dac_target;

    sample_index = (uint16)(g_cal_pi.samples_taken + 1u);
    g_cal_pi.samples_taken = sample_index;
    if (sample_index == 1u || (sample_index % CAL_PI_TELEM_PERIOD) == 0u) {
        cal_diag(PSOC_EVT_CAL_STAGE_DAC, dac_sample);
        cal_diag_i32(PSOC_EVT_CAL_STAGE_MEAS32, g_cal_pi.last_fir_output);
        cal_diag_i32(PSOC_EVT_CAL_PI_ERROR32, error_dac);
        cal_diag_i32(PSOC_EVT_CAL_PI_BUCKET32, error_bucket);
        cal_diag(PSOC_EVT_CAL_PI_STABLE,
                 (g_cal_pi.stable_count > 255u) ? 255u : (uint8)g_cal_pi.stable_count);
    }

    g_cal_pi.dac_current = dac_target;
    stage->write(g_cal_pi.dac_current);

    if (g_cal_pi.stable_count >= lock_n) {
        if (error_bucket == 0L && cal_pi_measurement_valid(g_cal_pi.last_fir_output)) {
            return cal_pi_finish_stage(1u);
        }
        if (stage_gain_x1000 == 0L) {
            return cal_pi_finish_stage(0u);
        }
    }

    if (g_cal_pi.samples_taken >= CAL_PI_TIMEOUT_SAMPLES) {
        return cal_pi_finish_stage((error_bucket == 0L &&
                                    cal_pi_measurement_valid(g_cal_pi.last_fir_output)) ? 1u : 0u);
    }

    return 0u;
}

static void cal_pi_start(void)
{
    /* Carga el FIR pensado para calibracion (suavizado, distinto del de
     * adquisicion) en el Filter de hardware -- se restaura el de
     * adquisicion en cal_async_complete() al cerrar la corrida. */
    (void)psoc_filter_load_fir_coefficients(g_fir_calibration_coeffs_q23, FILTER_FIR_NTAPS);
    g_cal_pi.state = CAL_PI_STAGE_BEGIN;
}

static uint8 cal_pi_service(void)
{
    switch (g_cal_pi.state) {
        case CAL_PI_STAGE_BEGIN:
            cal_pi_stage_begin();
            return 0u;
        case CAL_PI_SETTLE:
            return cal_pi_settle_service();
        case CAL_PI_RUN:
            return cal_pi_run_service();
        default:
            return 0u;
    }
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
    g_cal_async.pass_index = 0u;
    g_cal_async.start_ticks = psoc_now_ticks();
    g_cal_async.last_progress_ticks = g_cal_async.start_ticks;
    for (i = 0u; i < PSOC_CAL_MAX_STAGES; i++) {
        g_cal_async.slope_known[i] = 0u;
        g_cal_async.slope_increasing[i] = 0u;
    }
    for (i = 0u; i < PSOC_CAL_STAGE_COUNT; i++) {
        uint8 center = cal_stage_center_dac(&g_psoc_cal_stages[i]);
        g_psoc_cal_stages[i].write(center);
        g_psoc_cal_results[i].final_dac = center;
        g_psoc_cal_results[i].final_measured = 0L;
        g_psoc_cal_results[i].ok = 0u;
    }
    cal_pi_start();   /* unico camino de calibracion, GEO y HAMMER por igual */
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

#if CAL_ALGO_BISECTION_ENABLE /* biseccion legacy -- el chequeo de rango operativo de GEO_LP que
       * hacia esta funcion (cal_measured_out_of_range) quedo sin uso junto
       * con la biseccion; psoc_calibration_async_result_ok() de arriba ya
       * no lo necesita. */
static uint8 psoc_calibration_async_result_ok_geo_legacy(void)
{
    if (g_psoc_cal_result_count >= PSOC_CAL_STAGE_COUNT) {
        return cal_measured_out_of_range(
            g_psoc_cal_results[PSOC_CAL_STAGE_COUNT - 1u].final_measured
        ) ? 0u : 1u;
    }
    return g_cal_async.ok;
}
#endif /* #if 0 -- biseccion legacy */

uint8 psoc_calibration_service_async(void)
{
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

    return cal_pi_service();   /* unico camino de calibracion */
}

#if CAL_ALGO_BISECTION_ENABLE /* biseccion legacy -- ver nota junto a cal_stage_saturated mas arriba.
       * Este era el cuerpo real de psoc_calibration_service_async() para
       * GEO antes de unificar todo en el PI. */
static uint8 psoc_calibration_service_async_geo_legacy(void)
{
    {
    const PsocCalStage *stage;
    int32 abs_error;

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
            CAL_AMUX_ADC_SELECT_STAGE(stage->adc_channel);
            ADC_Start();
            isr_DMA_DelSig_RAM_ClearPending();
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
            CAL_AMUX_ADC_SELECT_STAGE(stage->adc_channel);
            ADC_Start();
            isr_DMA_DelSig_RAM_ClearPending();
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
            CAL_AMUX_ADC_SELECT_STAGE(stage->adc_channel);
            ADC_Start();
            isr_DMA_DelSig_RAM_ClearPending();
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
    }

    return 0u;
}
#endif /* #if 0 -- biseccion legacy */
