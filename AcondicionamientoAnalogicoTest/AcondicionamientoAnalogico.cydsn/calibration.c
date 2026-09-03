#include "calibration.h"
#include "calibration_tables.h"
#include "psoc_adc.h"
#include "filter_coeffs.h"
#include "FIR_adquisition.h"
#include "FIR_calibration.h"

/* El servo lento y la busqueda binaria se borraron. El PI es el unico
 * algoritmo de calibracion; no hay selector ni ramas apagadas. */

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
#if CAL_AMUX_HAS_CAP_CHANNEL
        /* Si el canal que se esta soltando ERA el del capacitor, el flag tiene
         * que caer con el. Si no, queda diciendo "conectado" sobre un canal ya
         * desconectado y el capacitor no se vuelve a conectar nunca: las
         * mediciones siguientes corren sin el filtro que creen tener. Solo
         * pasa si alguien mide el canal del capacitor como si fuera senal,
         * que es justo lo que hace el autotest en D1b. */
        if (g_amux_active_channel == CAL_AMUX_CAP_CHANNEL) {
            g_amux_cap_connected = 0u;
        }
#endif
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

static int16 cal_stage_min_dac(const PsocCalStage *stage)
{
    return (stage->dac_center > stage->dac_max_change)
        ? (uint8)(stage->dac_center - stage->dac_max_change)
        : 0u;
}

static int16 cal_stage_max_dac(const PsocCalStage *stage)
{
    uint16 max_dac = (uint16)stage->dac_center + (uint16)stage->dac_max_change;
    return (max_dac > 255u) ? 255u : (uint8)max_dac;
}

static int16 cal_stage_clamp_dac(const PsocCalStage *stage, int16 dac)
{
    uint8 lo = cal_stage_min_dac(stage);
    uint8 hi = cal_stage_max_dac(stage);
    if (dac < lo) { return lo; }
    if (dac > hi) { return hi; }
    return dac;
}

static int16 cal_stage_center_dac(const PsocCalStage *stage)
{
    return cal_stage_clamp_dac(stage, stage->dac_center);
}

PsocCalResult g_psoc_cal_results[PSOC_CAL_MAX_STAGES];
uint8 g_psoc_cal_result_count = 0u;

#define CAL_ASYNC_EMPTY_POLL_LIMIT 2000000UL

/* Watchdog global expresado como ticks legacy de 10 ms para mantener la escala
 * de configuracion anterior; en runtime lo ejecuta Timer_3 como one-shot. */
#ifndef CAL_WATCHDOG_TICKS
#define CAL_WATCHDOG_TICKS 40000UL
#endif

/* Periodo de telemetria de progreso (ticks legacy de 10 ms => ~500 ms). */
#define CAL_PROGRESS_PERIOD_TICKS 50UL
#define CAL_LEGACY_TICK_MS 10UL
#define CAL_WATCHDOG_MS (CAL_WATCHDOG_TICKS * CAL_LEGACY_TICK_MS)
#define CAL_PROGRESS_PERIOD_MS (CAL_PROGRESS_PERIOD_TICKS * CAL_LEGACY_TICK_MS)

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
    CAL_ASYNC_DONE
} PsocCalAsyncState;

typedef struct {
    PsocCalAsyncState state;
    uint8 busy;
    uint8 done;
    uint8 ok;
    uint8 stage_index;
    uint8 pass_index;
} PsocCalAsync;

static PsocCalAsync g_cal_async = { CAL_ASYNC_IDLE };




static int16 cal_stage_current_dac(uint8 stage_index)
{
    if (stage_index >= g_psoc_cal_result_count) {
        return cal_stage_center_dac(&g_psoc_cal_stages[stage_index]);
    }
    return cal_stage_clamp_dac(&g_psoc_cal_stages[stage_index],
                               g_psoc_cal_results[stage_index].final_dac);
}

static void cal_stage_write_result(uint8 stage_index, int16 dac)
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

void psoc_calibration_seed_dac(const int16 *dac_values, uint8 count)
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
    VDAC_Ref_Sum_Start();
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

void psoc_calibration_seed_default_dac(void)
{
    uint8 i;

    for (i = 0u; i < PSOC_CAL_STAGE_COUNT; i++) {
        uint8 center = cal_stage_center_dac(&g_psoc_cal_stages[i]);
        g_psoc_cal_stages[i].write(center);
        g_psoc_cal_results[i].final_dac = center;
        g_psoc_cal_results[i].final_measured = 0L;
        g_psoc_cal_results[i].ok = 1u;
    }
    g_psoc_cal_result_count = PSOC_CAL_STAGE_COUNT;
}


/* Stubs no-op: el servo lento de mantenimiento esta comentado (arriba) a
 * pedido del usuario -- calibracion es 100% PI ahora. Se mantiene la firma
 * publica porque main.c los llama incondicionalmente. */
/* Compartido con cal_pi_finish_stage -- cierra la corrida de calibracion
 * (restaura AMux/captura, re-habilita isr_SyncIn) sin importar que
 * controlador la corrio. */
static void cal_async_complete(void)
{
    psoc_cal_timer_stop();
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
#ifdef CY_ISR_isr_SyncIn_H
    isr_SyncIn_ClearPending();
    isr_SyncIn_Enable();
#endif
    g_cal_async.busy = 0u;
    g_cal_async.done = 1u;
    g_cal_async.state = CAL_ASYNC_DONE;
}

/* Aborta la calibracion por timeout global. Las etapas ya finalizadas se
 * conservan; la etapa en curso y las pendientes vuelven al adelanto nominal. */
static void cal_async_abort_watchdog(void)
{
    uint8 i;

    for (i = g_cal_async.stage_index; i < PSOC_CAL_STAGE_COUNT; i++) {
        uint8 center = cal_stage_center_dac(&g_psoc_cal_stages[i]);
        g_psoc_cal_stages[i].write(center);
        g_psoc_cal_results[i].final_dac = center;
        g_psoc_cal_results[i].final_measured = 0L;
        g_psoc_cal_results[i].ok = 0u;
    }
    g_cal_async.ok = 0u;
    cal_diag(PSOC_EVT_CAL_WATCHDOG, g_cal_async.stage_index);

    cal_async_complete();
}

/* ============================================================
 * Controlador PI de calibracion: unico algoritmo activo. Puerto directo de
 * Subsystem_step() en
 * modelado/matlab/Simulaciones Controladores/Desacople/Subsystem_grt_rtw/Subsystem.c
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
 * Reutiliza g_cal_async.busy/done/ok/stage_index y los flags de Timer_3
 * cal_async_complete/cal_async_abort_watchdog tal cual: para el resto del
 * firmware (EEPROM, diagnostico UART, ESP/web) mantiene la misma API externa.
 * ============================================================ */

typedef enum {
    CAL_PI_STAGE_BEGIN = 0u,
    CAL_PI_SETTLE,
    CAL_PI_RUN,
    CAL_PI_REFINE_SETTLE
} PsocCalPiState;

typedef struct {
    int32 kp_num;
    int32 kp_div;
    int32 ki_num;
    int32 ki_div;
    int32 gain_x1000;       /* ganancia fija VDAC->medida; 0 = dinamica por etapa */
    int32 deadband_dac;     /* Delta_i usado; 0 = derivar del piso fisico */
    uint16 lock_samples;    /* M muestras en la misma celda de error */
    uint16 settle_samples;  /* espera inicial del FIR al cambiar AMux/VDAC */
    uint16 timeout_samples; /* techo de muestras de PI para esta etapa */
    uint8 refine_enable;    /* prueba final de +/-1 codigo VDAC */
    uint16 refine_settle_samples;
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
    int32 refine_base_measured;
    int32 refine_base_abs_error;
    int16 refine_base_dac;
    int16 refine_trial_dac;
    uint8 refine_ok;
    uint8 have_last_error;
    int16 last_dac_target;
    int16 base_dac;
    int16 dac_current;
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
    { CAL_PI_KP_NUM_GEO_PGA, CAL_PI_KP_DIV_GEO_PGA, CAL_PI_KI_NUM_GEO_PGA, CAL_PI_KI_DIV_GEO_PGA, CAL_PI_GAIN_GEO_PGA_X1000, CAL_PI_DEADBAND_GEO_PGA_DAC_CODES, CAL_PI_LOCK_SAMPLES_GEO_PGA, CAL_PI_SETTLE_SAMPLES_GEO_PGA, CAL_PI_TIMEOUT_SAMPLES_GEO_PGA, CAL_PI_REFINE_ENABLE_GEO_PGA, CAL_PI_REFINE_SETTLE_SAMPLES_GEO_PGA },
#if defined(VDAC_ref_BP_DEFAULT_DATA) || defined(CY_DVDAC_VDAC_ref_BP_H)
    { CAL_PI_KP_NUM_GEO_BP, CAL_PI_KP_DIV_GEO_BP, CAL_PI_KI_NUM_GEO_BP, CAL_PI_KI_DIV_GEO_BP, CAL_PI_GAIN_GEO_BP_X1000, CAL_PI_DEADBAND_GEO_BP_DAC_CODES, CAL_PI_LOCK_SAMPLES_GEO_BP, CAL_PI_SETTLE_SAMPLES_GEO_BP, CAL_PI_TIMEOUT_SAMPLES_GEO_BP, CAL_PI_REFINE_ENABLE_GEO_BP, CAL_PI_REFINE_SETTLE_SAMPLES_GEO_BP },
#endif
    { CAL_PI_KP_NUM_GEO_SUM, CAL_PI_KP_DIV_GEO_SUM, CAL_PI_KI_NUM_GEO_SUM, CAL_PI_KI_DIV_GEO_SUM, CAL_PI_GAIN_GEO_SUM_X1000, CAL_PI_DEADBAND_GEO_SUM_DAC_CODES, CAL_PI_LOCK_SAMPLES_GEO_SUM, CAL_PI_SETTLE_SAMPLES_GEO_SUM, CAL_PI_TIMEOUT_SAMPLES_GEO_SUM, CAL_PI_REFINE_ENABLE_GEO_SUM, CAL_PI_REFINE_SETTLE_SAMPLES_GEO_SUM },
    { CAL_PI_KP_NUM_GEO_LP, CAL_PI_KP_DIV_GEO_LP, CAL_PI_KI_NUM_GEO_LP, CAL_PI_KI_DIV_GEO_LP, CAL_PI_GAIN_GEO_LP_X1000, CAL_PI_DEADBAND_GEO_LP_DAC_CODES, CAL_PI_LOCK_SAMPLES_GEO_LP, CAL_PI_SETTLE_SAMPLES_GEO_LP, CAL_PI_TIMEOUT_SAMPLES_GEO_LP, CAL_PI_REFINE_ENABLE_GEO_LP, CAL_PI_REFINE_SETTLE_SAMPLES_GEO_LP },
};
#else
static const PsocCalPiCfg g_cal_pi_cfg[PSOC_CAL_STAGE_COUNT] = {
    { CAL_PI_KP_NUM_HAMMER_PGA, CAL_PI_KP_DIV_HAMMER_PGA, CAL_PI_KI_NUM_HAMMER_PGA, CAL_PI_KI_DIV_HAMMER_PGA, CAL_PI_GAIN_HAMMER_PGA_X1000, CAL_PI_DEADBAND_HAMMER_PGA_DAC_CODES, CAL_PI_LOCK_SAMPLES_HAMMER_PGA, CAL_PI_SETTLE_SAMPLES_HAMMER_PGA, CAL_PI_TIMEOUT_SAMPLES_HAMMER_PGA, CAL_PI_REFINE_ENABLE_HAMMER_PGA, CAL_PI_REFINE_SETTLE_SAMPLES_HAMMER_PGA },
    { CAL_PI_KP_NUM_HAMMER_LP, CAL_PI_KP_DIV_HAMMER_LP, CAL_PI_KI_NUM_HAMMER_LP, CAL_PI_KI_DIV_HAMMER_LP, CAL_PI_GAIN_HAMMER_LP_X1000, CAL_PI_DEADBAND_HAMMER_LP_DAC_CODES, CAL_PI_LOCK_SAMPLES_HAMMER_LP, CAL_PI_SETTLE_SAMPLES_HAMMER_LP, CAL_PI_TIMEOUT_SAMPLES_HAMMER_LP, CAL_PI_REFINE_ENABLE_HAMMER_LP, CAL_PI_REFINE_SETTLE_SAMPLES_HAMMER_LP },
};
#endif

/* counts de ADC -> codigos de IDAC. Entero puro, en int64, y sin precalcular
 * el cociente: son 98,304 counts por codigo y redondear eso a 98 mete un error
 * sistematico del 0,3 % en todo el lazo. */
static int32 cal_counts_error_to_dac_scale(int32 error_counts)
{
    return cal_round_div_i64((int64)error_counts * (int64)CAL_COUNTS_PER_IDAC_CODE_DEN,
                             (int64)CAL_COUNTS_PER_IDAC_CODE_NUM);
}

/* Ganancia fisica de la etapa, referencia -> tap, x1000.
 *
 * La etapa 0 es la unica que NO tiene ganancia fija: su tap es la salida del
 * PGA de entrada, asi que lo que la referencia mueve alla escala con la
 * ganancia que tenga puesto el PGA. Medido en la placa: 57,7 / 108,7 / 220,2 /
 * 448,7 uV por codigo para 1x / 2x / 4x / 8x, o sea proporcional. Sin esto el
 * PI usaba la ganancia de 1x para todas y a 8x pedia un esfuerzo ocho veces
 * mayor que el necesario.
 *
 * Que la etapa 0 tenga la ganancia mas chica es justamente lo que la hace
 * calibrable con precision: 57,7 uV por codigo es el paso mas fino de las
 * cuatro etapas. */
static int32 cal_pi_stage_gain_x1000(uint8 stage_index)
{
    int32 configured_gain = g_cal_pi_cfg[stage_index].gain_x1000;

    if (stage_index == 0u && configured_gain > 0L) {
        int32 pga_gain_x1000 = (int32)psoc_hw_pga_gain_x1000();
        if (pga_gain_x1000 > 0L) {
            return cal_round_div_i64((int64)configured_gain * (int64)pga_gain_x1000,
                                     1000LL);
        }
    }
    return configured_gain;
}

static int32 cal_pi_deadband_dac_codes(uint8 stage_index)
{
#ifdef CAL_PI_FORCE_MIN_DEADBAND
    (void)stage_index;
    return CAL_PI_DEADBAND_MIN_DAC_CODES;
#else
    int32 deadband = g_cal_pi_cfg[stage_index].deadband_dac;
    int32 gain_x1000;
    int64 num;
    int64 den;

    if (deadband <= 0L) {
        gain_x1000 = cal_pi_stage_gain_x1000(stage_index);
        if (gain_x1000 < 0L) {
            gain_x1000 = -gain_x1000;
        }
        num = (int64)gain_x1000 * (int64)CAL_PI_DEADBAND_MARGIN_NUM;
        den = 1000LL * (int64)CAL_PI_DEADBAND_MARGIN_DEN;
        deadband = (int32)((num + den - 1LL) / den);
    }
    if (deadband < CAL_PI_DEADBAND_MIN_DAC_CODES) {
        deadband = CAL_PI_DEADBAND_MIN_DAC_CODES;
    }
    return deadband;
#endif
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
    return (measured >= CAL_ADC_SIGNED_MIN_COUNTS &&
            measured <= CAL_ADC_SIGNED_MAX_COUNTS) ? 1u : 0u;
}

static uint8 cal_stage_measure_current(uint8 stage_index, int16 dac, int32 *measured)
{
    const PsocCalStage *stage = &g_psoc_cal_stages[stage_index];

    ADC_Stop();
    CAL_AMUX_ADC_SELECT(stage->adc_channel);
    ADC_Start();
    stage->write(dac);
    CyDelay(CAL_DIAG_SWEEP_SETTLE_MS);
    if (measured != (int32 *)0) {
        *measured = cal_adc_read_direct_counts();
    }
    return 1u;
}

static uint8 cal_stage_value_in_tolerance(uint8 stage_index, int32 measured)
{
    const PsocCalStage *stage = &g_psoc_cal_stages[stage_index];
    int32 control_sample = cal_pi_compare_counts(measured);
    int32 error_counts = stage->target_counts - control_sample;
    int32 error_dac = cal_counts_error_to_dac_scale(error_counts);
    int32 deadband_dac = cal_pi_deadband_dac_codes(stage_index);

    return (abs_counts(error_dac) <= deadband_dac &&
            cal_pi_measurement_valid(measured)) ? 1u : 0u;
}

static uint8 cal_verify_seeded_values(void)
{
    uint8 i;
    uint8 all_ok = 1u;

    cal_diag(PSOC_EVT_BOOT, PSOC_HW_CLASS);
    g_psoc_cal_result_count = PSOC_CAL_STAGE_COUNT;

    for (i = 0u; i < PSOC_CAL_STAGE_COUNT; i++) {
        const PsocCalStage *stage = &g_psoc_cal_stages[i];
        PsocCalResult *result = &g_psoc_cal_results[i];
        uint8 dac = cal_stage_current_dac(i);
        int32 measured = 0L;
        uint8 ok;

        cal_diag(PSOC_EVT_CAL_STAGE_BEGIN, i);
        cal_diag_i32(PSOC_EVT_CAL_STAGE_TARGET32, stage->target_counts);

        (void)cal_stage_measure_current(i, dac, &measured);
        ok = cal_stage_value_in_tolerance(i, measured);

        result->final_dac = dac;
        result->final_measured = measured;
        result->ok = ok;

        cal_diag(PSOC_EVT_CAL_STAGE_DAC, result->final_dac);
        cal_diag_i16(PSOC_EVT_CAL_STAGE_MEAS, result->final_measured);
        cal_diag_i32(PSOC_EVT_CAL_STAGE_MEAS32, result->final_measured);
        cal_diag(PSOC_EVT_CAL_STAGE_OK, result->ok);

        if (!ok) {
            all_ok = 0u;
        }
    }

    psoc_calibration_restore_capture_path();
    return all_ok;
}

static int32 cal_pi_abs_error_counts(const PsocCalStage *stage, int32 measured)
{
    int32 control_sample = cal_pi_compare_counts(measured);
    return abs_counts(stage->target_counts - control_sample);
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

static uint8 cal_pi_finalize_stage(uint8 ok, int16 final_dac, int32 final_measured)
{
    const PsocCalStage *stage = &g_psoc_cal_stages[g_cal_async.stage_index];
    PsocCalResult *result = &g_psoc_cal_results[g_cal_async.stage_index];
    uint8 final_pass;

    final_dac = cal_stage_clamp_dac(stage, final_dac);
    stage->write(final_dac);
    result->final_dac = final_dac;
    result->final_measured = final_measured;
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

/* Cierra la etapa con lo que el lazo ya tiene y agrega un unico refinamiento
 * de cuantizacion: probar el LSB que deberia reducir el error fisico; si no
 * mejora el error absoluto medido por el FIR, vuelve al DAC anterior. */
static uint8 cal_pi_finish_stage(uint8 ok)
{
    const PsocCalStage *stage = &g_psoc_cal_stages[g_cal_async.stage_index];
    const PsocCalPiCfg *cfg = &g_cal_pi_cfg[g_cal_async.stage_index];
    int32 stage_gain_x1000;
    int32 control_sample;
    int32 error_counts;
    int8 step_sign;
    uint8 dac_lo;
    uint8 dac_hi;
    uint8 trial_dac;

    if (!cfg->refine_enable || !ok || !cal_pi_measurement_valid(g_cal_pi.last_fir_output)) {
        return cal_pi_finalize_stage(ok, g_cal_pi.dac_current, g_cal_pi.last_fir_output);
    }

    stage_gain_x1000 = cal_pi_stage_gain_x1000(g_cal_async.stage_index);
    if (stage_gain_x1000 == 0L) {
        return cal_pi_finalize_stage(ok, g_cal_pi.dac_current, g_cal_pi.last_fir_output);
    }

    control_sample = cal_pi_compare_counts(g_cal_pi.last_fir_output);
    error_counts = stage->target_counts - control_sample;
    step_sign = cal_pi_effort_delta_sign(error_counts, stage->direction, stage_gain_x1000);
    if (step_sign == 0) {
        return cal_pi_finalize_stage(ok, g_cal_pi.dac_current, g_cal_pi.last_fir_output);
    }

    dac_lo = cal_stage_min_dac(stage);
    dac_hi = cal_stage_max_dac(stage);
    if (step_sign > 0) {
        if (g_cal_pi.dac_current >= dac_hi) {
            return cal_pi_finalize_stage(ok, g_cal_pi.dac_current, g_cal_pi.last_fir_output);
        }
        trial_dac = (uint8)(g_cal_pi.dac_current + 1u);
    } else {
        if (g_cal_pi.dac_current <= dac_lo) {
            return cal_pi_finalize_stage(ok, g_cal_pi.dac_current, g_cal_pi.last_fir_output);
        }
        trial_dac = (uint8)(g_cal_pi.dac_current - 1u);
    }

    g_cal_pi.refine_ok = ok;
    g_cal_pi.refine_base_dac = g_cal_pi.dac_current;
    g_cal_pi.refine_trial_dac = trial_dac;
    g_cal_pi.refine_base_measured = g_cal_pi.last_fir_output;
    g_cal_pi.refine_base_abs_error = cal_pi_abs_error_counts(stage, g_cal_pi.last_fir_output);
    g_cal_pi.settle_remaining = cfg->refine_settle_samples;
    g_cal_pi.empty_polls = 0UL;
    g_cal_pi.dac_current = trial_dac;
    stage->write(trial_dac);
    g_cal_pi.state = CAL_PI_REFINE_SETTLE;
    return 0u;
}

static void cal_pi_stage_begin(void)
{
    const PsocCalStage *stage = &g_psoc_cal_stages[g_cal_async.stage_index];
    const PsocCalPiCfg *cfg = &g_cal_pi_cfg[g_cal_async.stage_index];
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
    g_cal_pi.settle_remaining = cfg->settle_samples;
    g_cal_pi.refine_base_measured = 0L;
    g_cal_pi.refine_base_abs_error = 0L;
    g_cal_pi.refine_base_dac = 0u;
    g_cal_pi.refine_trial_dac = 0u;
    g_cal_pi.refine_ok = 0u;
    g_cal_pi.base_dac = cal_stage_current_dac(g_cal_async.stage_index);
    g_cal_pi.dac_current = g_cal_pi.base_dac;
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
#ifdef CY_ISR_isr_DMA_Filter_RAM_H
    isr_DMA_Filter_RAM_ClearPending();
#endif
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
    g_cal_pi.last_fir_output = sample;

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

static uint8 cal_pi_refine_service(void)
{
    const PsocCalStage *stage = &g_psoc_cal_stages[g_cal_async.stage_index];
    int32 sample;
    int32 trial_abs_error;

    if (!psoc_adc_take_isr_filtered_sample(&sample)) {
        g_cal_pi.empty_polls++;
        if (g_cal_pi.empty_polls >= CAL_ASYNC_EMPTY_POLL_LIMIT) {
            stage->write(g_cal_pi.refine_base_dac);
            g_cal_pi.dac_current = g_cal_pi.refine_base_dac;
            g_cal_pi.last_fir_output = g_cal_pi.refine_base_measured;
            return cal_pi_finalize_stage(g_cal_pi.refine_ok,
                                         g_cal_pi.refine_base_dac,
                                         g_cal_pi.refine_base_measured);
        }
        return 0u;
    }

    g_cal_pi.empty_polls = 0UL;
    g_cal_pi.last_fir_output = sample;

    if (g_cal_pi.settle_remaining > 0u) {
        g_cal_pi.settle_remaining--;
    }
    if (g_cal_pi.settle_remaining != 0u) {
        return 0u;
    }

    trial_abs_error = cal_pi_abs_error_counts(stage, sample);
    if (trial_abs_error < g_cal_pi.refine_base_abs_error &&
        cal_pi_measurement_valid(sample)) {
        g_cal_pi.dac_current = g_cal_pi.refine_trial_dac;
        return cal_pi_finalize_stage(g_cal_pi.refine_ok,
                                     g_cal_pi.refine_trial_dac,
                                     sample);
    }

    stage->write(g_cal_pi.refine_base_dac);
    g_cal_pi.dac_current = g_cal_pi.refine_base_dac;
    g_cal_pi.last_fir_output = g_cal_pi.refine_base_measured;
    return cal_pi_finalize_stage(g_cal_pi.refine_ok,
                                 g_cal_pi.refine_base_dac,
                                 g_cal_pi.refine_base_measured);
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
    int16 dac_sample;
    int16 dac_lo;
    int16 dac_hi;
    int16 dac_target;
    int16 dac_step;
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
        /* Una etapa configurada con ganancia dinamica puede carecer de
         * autoridad en algun ajuste; no se persigue el error hasta saturar. */
        effort = (int32)dac_sample;
    } else if (control_error == 0L) {
        g_cal_pi.integral = 0L;
        effort = (int32)dac_sample;
    } else {
        /* PI posicional en escala DAC: error_counts -> error_dac antes de
         * entrar al PI; P/I se dividen por la ganancia fisica VDAC->medida. */
        p_term = cal_pi_gain_scaled_term(control_error, cfg->kp_num, cfg->kp_div, stage_gain_x1000);
        i_term = cal_pi_gain_scaled_term(g_cal_pi.integral, cfg->ki_num, cfg->ki_div, stage_gain_x1000);
        effort = (int32)g_cal_pi.base_dac + (int32)stage->direction * (p_term + i_term);
    }

    effort_delta_sign = cal_pi_effort_delta_sign(control_error, stage->direction, stage_gain_x1000);

    if (effort < (int32)dac_lo) {
        dac_target = dac_lo;
        can_integrate = (effort_delta_sign > 0) ? 1u : 0u;
    } else if (effort > (int32)dac_hi) {
        dac_target = dac_hi;
        can_integrate = (effort_delta_sign < 0) ? 1u : 0u;
    } else {
        dac_target = (int16)effort;
        can_integrate = 1u;
    }

    /* Limitador de pendiente: con la macro en 0 no se limita nada y manda la
     * ley del PI, que es lo correcto. Estaba fijo en 1 codigo por muestra y eso
     * no protegia de nada: solo impedia que el PI aplicara su propio esfuerzo,
     * y una etapa que necesitaba 84 codigos tardaba 84 muestras como piso. */
    dac_step = (int16)CAL_PI_MAX_DAC_STEP_PER_SAMPLE;
    if (dac_step > 0) {
        dac_max_step_up = (int32)dac_sample + (int32)dac_step;
        dac_max_step_down = (int32)dac_sample - (int32)dac_step;
        if ((int32)dac_target > dac_max_step_up) {
            dac_target = (int16)dac_max_step_up;
        } else if ((int32)dac_target < dac_max_step_down) {
            dac_target = (int16)dac_max_step_down;
        }
        dac_target = cal_stage_clamp_dac(stage, dac_target);
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
        cal_diag_i16(PSOC_EVT_CAL_STAGE_DAC, (int32)dac_sample);
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

    if (g_cal_pi.samples_taken >= cfg->timeout_samples) {
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
        case CAL_PI_REFINE_SETTLE:
            return cal_pi_refine_service();
        default:
            return 0u;
    }
}

uint8 psoc_calibration_start_async(void)
{
    uint8 i;
    uint8 verify_ok;
    uint8 seed_dac[PSOC_CAL_MAX_STAGES];

    if (g_cal_async.busy) {
        return 0u;
    }

#ifdef CY_ISR_isr_SyncIn_H
    isr_SyncIn_Disable();
#endif
    ADC_Stop();
    psoc_adc_select_capture_config();
    ADC_Stop();

    for (i = 0u; i < PSOC_CAL_STAGE_COUNT; i++) {
        seed_dac[i] = cal_stage_current_dac(i);
    }

    verify_ok = cal_verify_seeded_values();
    if (verify_ok) {
        g_cal_async.busy = 0u;
        g_cal_async.done = 1u;
        g_cal_async.ok = 1u;
        return 2u;
    }

    g_psoc_cal_result_count = PSOC_CAL_STAGE_COUNT;
    g_cal_async.busy = 1u;
    g_cal_async.done = 0u;
    g_cal_async.ok = 1u;
    g_cal_async.stage_index = 0u;
    g_cal_async.pass_index = 0u;
    psoc_cal_timer_start(CAL_PROGRESS_PERIOD_MS, CAL_WATCHDOG_MS);
    for (i = 0u; i < PSOC_CAL_STAGE_COUNT; i++) {
        g_psoc_cal_stages[i].write(seed_dac[i]);
        g_psoc_cal_results[i].final_dac = seed_dac[i];
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

uint8 psoc_calibration_service_async(void)
{
    if (!g_cal_async.busy) {
        return 0u;
    }

    if (psoc_cal_timer_take_watchdog_due()) {
        cal_async_abort_watchdog();
        return 1u;
    }
    if (psoc_cal_timer_take_progress_due()) {
        cal_diag(PSOC_EVT_CAL_PROGRESS, g_cal_async.stage_index);
    }

    return cal_pi_service();   /* unico camino de calibracion */
}

/* ==========================================================================
 * PRIMITIVAS DE AUTOTEST  (proyecto AcondicionamientoAnalogicoTest)
 * --------------------------------------------------------------------------
 * Este bloque existe SOLO en el proyecto de test. Expone hacia main.c
 * (via psoc_selftest.h) las primitivas de medicion que ya usaba la
 * calibracion, porque el AMux y el lector de ADC son static en este archivo
 * y el autotest los necesita para barrer IDAC contra cada tap del AMux.
 *
 * No se agrega ningun .c nuevo al proyecto: el .cyprj no se toca a mano.
 * ========================================================================== */


/* Sin libm: agregar -lm al link obliga a editar el .cyprj, y ese archivo no
 * se toca a mano. Estas tres funciones cubren lo unico que hacia falta.
 * Solo usan +, -, * y /, que resuelve libgcc. */

/* Raiz cuadrada entera por Newton sobre uint64. Alcanza de sobra: todo lo que
 * se reporta termina redondeado a microvolts enteros. */
static uint32 st_isqrt64(uint64 x)
{
    uint64 r, prev;
    if (x == 0u) { return 0u; }
    /* Semilla: 2^(ceil(bits/2)) */
    r = x;
    prev = 0u;
    {
        uint32 bits = 0u;
        uint64 t = x;
        while (t != 0u) { t >>= 1; bits++; }
        r = (uint64)1u << ((bits + 1u) / 2u);
    }
    /* Newton: r <- (r + x/r)/2, converge en pocas vueltas y es monotona. */
    while (r != prev) {
        prev = r;
        r = (r + x / r) / 2u;
        if (r == 0u) { return 0u; }
        /* corta oscilacion entre dos valores adyacentes */
        if (prev > r && (prev - r) == 1u) { break; }
    }
    while (r * r > x) { r--; }
    return (uint32)r;
}

/* Coseno y seno por Taylor con reduccion de rango a [-pi/4, pi/4].
 * El Goertzel del autotest usa w = 2*pi*f0/fs; con f0 = 50 Hz y fs = 2604 Hz
 * queda w = 0.1207 rad, comodamente dentro del rango donde 5 terminos dan
 * mas precision de la que necesita una medida de ruido. La reduccion esta
 * igual para que siga siendo correcto si se cambia la frecuencia. */
#define ST_PI      3.14159265358979323846
#define ST_TWO_PI  6.28318530717958647692
#define ST_HALF_PI 1.57079632679489661923

static double st_cos_core(double x)   /* |x| <= pi/4 */
{
    double x2 = x * x;
    return 1.0 - x2 * (1.0 / 2.0
         - x2 * (1.0 / 24.0
         - x2 * (1.0 / 720.0
         - x2 * (1.0 / 40320.0
         - x2 * (1.0 / 3628800.0)))));
}

static double st_sin_core(double x)   /* |x| <= pi/4 */
{
    double x2 = x * x;
    return x * (1.0 - x2 * (1.0 / 6.0
              - x2 * (1.0 / 120.0
              - x2 * (1.0 / 5040.0
              - x2 * (1.0 / 362880.0)))));
}

/* Reduce a [0, 2pi) y despacha por octante usando las identidades
 * sin(x+pi/2)=cos(x) y cos(x+pi/2)=-sin(x). */
static void st_sincos(double x, double *out_sin, double *out_cos)
{
    int quad;
    double r;

    while (x < 0.0)        { x += ST_TWO_PI; }
    while (x >= ST_TWO_PI) { x -= ST_TWO_PI; }

    quad = (int)(x / ST_HALF_PI);   /* 0..3 */
    r = x - (double)quad * ST_HALF_PI;

    /* Dentro del cuadrante, si r > pi/4 se usa el complemento para que el
     * argumento del Taylor nunca pase de pi/4. */
    if (r > (ST_HALF_PI / 2.0)) {
        double c = ST_HALF_PI - r;
        double sc = st_sin_core(c);
        double cc = st_cos_core(c);
        /* Con c = pi/2 - r, o sea r = pi/2 - c, y x = quad*pi/2 + r:
         *   quad 0: sin(r)        =  cos(c)   cos(r)        =  sin(c)
         *   quad 1: sin(pi/2+r)   =  sin(c)   cos(pi/2+r)   = -cos(c)
         *   quad 2: sin(pi+r)     = -cos(c)   cos(pi+r)     = -sin(c)
         *   quad 3: sin(3pi/2+r)  = -sin(c)   cos(3pi/2+r)  =  cos(c)
         * Los cuadrantes 1 y 3 estaban intercambiados: devolvia el seno y el
         * coseno con los dos signos dados vuelta. No afectaba a la medicion de
         * 50 Hz (w = 0,12 rad cae en el cuadrante 0 y por la otra rama), pero
         * corrompia la magnitud del Goertzel para cualquier tono por encima de
         * ~325 Hz. */
        switch (quad) {
            case 0:  *out_sin =  cc; *out_cos =  sc; break;
            case 1:  *out_sin =  sc; *out_cos = -cc; break;
            case 2:  *out_sin = -cc; *out_cos = -sc; break;
            default: *out_sin = -sc; *out_cos =  cc; break;
        }
    } else {
        double sr = st_sin_core(r);
        double cr = st_cos_core(r);
        switch (quad) {
            case 0:  *out_sin =  sr; *out_cos =  cr; break;
            case 1:  *out_sin =  cr; *out_cos = -sr; break;
            case 2:  *out_sin = -sr; *out_cos = -cr; break;
            default: *out_sin = -cr; *out_cos =  sr; break;
        }
    }
}

/* Rango de entrada a fondo de escala de cada config del ADC, en microvolts.
 * 18 bits con signo -> +-131072 counts a fondo de escala. */
#define SELFTEST_ADC_FS_COUNTS 131072L

/* Muestras que se usan para estimar la continua antes del Goertzel. */
#define ST_DC_PRE_SAMPLES 64u

static int32 selftest_config_range_uv(void)
{
    switch (psoc_adc_get_config())
    {
        case 2u:  return 512000L;    /* ADC_CF_0V512 */
        case 3u:  return 1024000L;   /* ADC_CF_1V024 */
        case 4u:  return 625000L;    /* ADC_CF_0V625 */
        case 1u:
        default:  return 2500000L;   /* ADC_CF_2V5 */
    }
}

/* Lectura del ADC que SI distingue "no convirtio" de "convirtio cero".
 * cal_adc_read_direct_counts() devuelve 0 al vencer el guard, y eso es
 * indistinguible de una medicion legitima de 0 V. Para el autotest esa
 * ambiguedad es inaceptable: convierte un ADC muerto en un PASS. */
static uint8 st_adc_read_checked(int32 *out)
{
    uint16 guard;
    uint8 ready = 0u;

    if (out == (int32 *)0) { return 0u; }

    ADC_StopConvert();
    ADC_StartConvert();
    for (guard = 0u; guard < (uint16)CAL_ADC_DIRECT_WAIT_POLLS; guard++) {
        if (ADC_IsEndConversion(ADC_RETURN_STATUS)) { ready = 1u; break; }
    }
    if (!ready) {
        ADC_StopConvert();
        return 0u;
    }
    *out = psoc_adc_counts_right_aligned(ADC_GetResult32());
    ADC_StopConvert();
    return 1u;
}

int32 psoc_selftest_counts_to_uv(int32 counts)
{
    int64 num = (int64)counts * (int64)selftest_config_range_uv();
    return (int32)(num / (int64)SELFTEST_ADC_FS_COUNTS);
}

uint8 psoc_selftest_stage_count(void)
{
    return PSOC_CAL_STAGE_COUNT;
}

uint8 psoc_selftest_stage_channel(uint8 stage, uint8 *out_channel)
{
    if (stage >= PSOC_CAL_STAGE_COUNT || out_channel == (uint8 *)0) { return 0u; }
    *out_channel = g_psoc_cal_stages[stage].adc_channel;
    return 1u;
}

uint8 psoc_selftest_amux_channel_count(void)
{
    return (uint8)AMux_ADC_CHANNELS;
}

/* Escribe el IDAC de la etapa SIN clamp: el barrido del autotest necesita
 * poder recorrer 0..255 completo, no el rango acotado que usa el PI. */
uint8 psoc_selftest_write_stage_dac(uint8 stage, uint8 code)
{
    if (stage >= PSOC_CAL_STAGE_COUNT) { return 0u; }
    g_psoc_cal_stages[stage].write(code);
    return 1u;
}

uint8 psoc_selftest_current_stage_dac(uint8 stage, uint8 *out_code)
{
    if (stage >= PSOC_CAL_STAGE_COUNT || out_code == (uint8 *)0) { return 0u; }
    *out_code = cal_stage_current_dac(stage);
    return 1u;
}

uint8 psoc_selftest_stage_result(uint8 stage, uint8 *out_dac, int32 *out_meas, uint8 *out_ok)
{
    if (stage >= PSOC_CAL_STAGE_COUNT || stage >= g_psoc_cal_result_count) { return 0u; }
    if (out_dac  != (uint8 *)0) { *out_dac  = g_psoc_cal_results[stage].final_dac; }
    if (out_meas != (int32 *)0) { *out_meas = g_psoc_cal_results[stage].final_measured; }
    if (out_ok   != (uint8 *)0) { *out_ok   = g_psoc_cal_results[stage].ok; }
    return 1u;
}

/* Selecciona un canal del AMux de forma exclusiva y deja el ADC corriendo.
 * with_cap conecta ademas el canal del capacitor (mismo criterio que usa la
 * calibracion para medir una etapa). */
void psoc_selftest_select_channel(uint8 channel, uint8 with_cap)
{
    /* Pedir el capacitor EN PARALELO al propio canal del capacitor no tiene
     * sentido y ademas confunde a los dos trackers, que apuntarian al mismo
     * canal fisico con dos significados distintos. */
#if CAL_AMUX_HAS_CAP_CHANNEL
    if (channel == CAL_AMUX_CAP_CHANNEL) { with_cap = 0u; }
#endif
    ADC_Stop();
    psoc_amux_select_exclusive(channel, with_cap);
    ADC_Start();
}

void psoc_selftest_restore(void)
{
    psoc_calibration_restore_capture_path();
}

/* Medicion DC de un canal: promedio de n conversiones completas y
 * reiniciadas (cada una asienta el decimador del DelSig por si sola, que es
 * lo correcto despues de conmutar el AMux). Devuelve media y pico-pico en
 * counts right-aligned. */
uint8 psoc_selftest_measure_dc(uint8 channel, uint16 settle_ms, uint16 n,
                               uint8 with_cap, int32 *out_mean, int32 *out_pp)
{
    int64 sum = 0;
    int32 vmin = 0, vmax = 0;
    uint16 i;

    if (n == 0u || out_mean == (int32 *)0) { return 0u; }

    psoc_selftest_select_channel(channel, with_cap);
    if (settle_ms > 0u) { CyDelay(settle_ms); }

    for (i = 0u; i < n; i++) {
        int32 s;
        if (!st_adc_read_checked(&s)) {
            /* El ADC no convirtio. Abortar: devolver ceros seria decir "0 V,
             * perfectamente estable" sobre una placa rota. */
            psoc_selftest_restore();
            return 0u;
        }
        sum += (int64)s;
        if (i == 0u) { vmin = s; vmax = s; }
        else {
            if (s < vmin) { vmin = s; }
            if (s > vmax) { vmax = s; }
        }
    }

    *out_mean = (int32)(sum / (int64)n);
    if (out_pp != (int32 *)0) { *out_pp = vmax - vmin; }
    return 1u;
}

/* Serie continua para el piso de ruido: no reinicia la conversion entre
 * muestras, asi que respeta la Fs nativa y sirve para estimar amplitud a una
 * frecuencia dada. Calcula todo en streaming (sin buffer): media, RMS,
 * pico-pico y la amplitud a tone_hz por Goertzel.
 *
 * Salidas en counts; el RMS y la amplitud son magnitudes de AC (media ya
 * restada en el caso del RMS). */
uint8 psoc_selftest_measure_series(uint8 channel, uint16 settle_ms, uint16 n,
                                   uint8 with_cap, uint16 tone_hz,
                                   int32 *out_mean, int32 *out_rms,
                                   int32 *out_pp, int32 *out_tone)
{
    int64 sum = 0;
    int64 sumsq = 0;
    int32 vmin = 0, vmax = 0;
    uint16 i;
    uint16 got = 0u;
    double w, cw, sw, coeff;
    double s0 = 0.0, s1 = 0.0, s2 = 0.0;
    double dc_est = 0.0;

    if (n == 0u || out_mean == (int32 *)0) { return 0u; }

    psoc_selftest_select_channel(channel, with_cap);
    if (settle_ms > 0u) { CyDelay(settle_ms); }

    /* Goertzel: k = n*f0/fs, w = 2*pi*k/n = 2*pi*f0/fs */
    w = ST_TWO_PI * (double)tone_hz / (double)PSOC_ADC_NATIVE_FS_HZ;
    st_sincos(w, &sw, &cw);
    coeff = 2.0 * cw;

    /* Descarta las primeras conversiones: el decimador del DelSig arrastra
     * historia del canal anterior. */
    (void)cal_adc_read_direct_counts();

    /* Estimacion de continua para restarsela al Goertzel.
     *
     * Con N multiplo de la Fs el bin cae exacto y la continua se rechaza sola,
     * pero esto cubre el caso de que alguien pida un N cualquiera: sin restarla,
     * la fuga de un offset de decenas de miles de counts (lo normal en un tap
     * sin calibrar) se mide como si fuera senal a 50 Hz. Se estima con las
     * primeras muestras, que es suficiente porque la continua no se mueve en
     * el segundo que dura la medicion. */
    {
        int32 dcs;
        uint16 g2;
        uint16 got_dc = 0u;
        int64 dcsum = 0;
        ADC_StopConvert();
        ADC_StartConvert();
        for (dcs = 0; dcs < (int32)ST_DC_PRE_SAMPLES; dcs++) {
            uint8 rdy = 0u;
            for (g2 = 0u; g2 < (uint16)CAL_ADC_DIRECT_WAIT_POLLS; g2++) {
                if (ADC_IsEndConversion(ADC_RETURN_STATUS)) { rdy = 1u; break; }
            }
            if (!rdy) { break; }
            dcsum += (int64)psoc_adc_counts_right_aligned(ADC_GetResult32());
            got_dc++;
        }
        ADC_StopConvert();
        if (got_dc > 0u) { dc_est = (double)dcsum / (double)got_dc; }
    }

    ADC_StopConvert();
    ADC_StartConvert();
    for (i = 0u; i < n; i++) {
        int32 s;
        uint16 guard;
        uint8 ready = 0u;

        for (guard = 0u; guard < (uint16)CAL_ADC_DIRECT_WAIT_POLLS; guard++) {
            if (ADC_IsEndConversion(ADC_RETURN_STATUS)) { ready = 1u; break; }
        }
        if (!ready) {
            /* Corte a mitad de serie: la ventana ya no es la pedida, asi que
             * ni la normalizacion del Goertzel ni el RMS son comparables con
             * nada. Se aborta en vez de devolver numeros que parecen validos. */
            ADC_StopConvert();
            psoc_selftest_restore();
            return 0u;
        }

        s = psoc_adc_counts_right_aligned(ADC_GetResult32());
        sum += (int64)s;
        sumsq += (int64)s * (int64)s;
        if (got == 0u) { vmin = s; vmax = s; }
        else {
            if (s < vmin) { vmin = s; }
            if (s > vmax) { vmax = s; }
        }

        s0 = coeff * s1 - s2 + ((double)s - dc_est);
        s2 = s1;
        s1 = s0;
        got++;
    }
    ADC_StopConvert();

    /* Se exige la ventana COMPLETA: ver el comentario del corte de arriba. */
    if (got != n) { psoc_selftest_restore(); return 0u; }

    *out_mean = (int32)((double)sum / (double)got);

    if (out_pp != (int32 *)0) { *out_pp = vmax - vmin; }

    if (out_rms != (int32 *)0) {
        /* var = E[x^2] - E[x]^2, en double para no perder la resta. */
        double ex2 = (double)sumsq / (double)got;
        double ex  = (double)sum / (double)got;
        double var = ex2 - ex * ex;
        if (var < 0.0) { var = 0.0; }
        *out_rms = (int32)st_isqrt64((uint64)var);
    }

    if (out_tone != (int32 *)0) {
        /* Magnitud del bin, normalizada a amplitud de pico. El termino DC no
         * molesta porque Goertzel a f0 != 0 lo rechaza solo salvo fuga; con n
         * grande alcanza. */
        double re = s1 - s2 * cw;
        double im = s2 * sw;
        double p  = re * re + im * im;
        if (p < 0.0) { p = 0.0; }
        *out_tone = (int32)(2.0 * (double)st_isqrt64((uint64)p) / (double)got);
    }

    return 1u;
}
