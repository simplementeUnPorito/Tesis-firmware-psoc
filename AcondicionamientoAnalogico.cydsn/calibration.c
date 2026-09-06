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

#ifndef CAL_ADC_DIRECT_CONVERSION_MS
/* El ADC DelSig entrega a 2604 Sa/s en la configuracion de calibracion. La
 * ISR de la ruta filtrada tambien lee el status de fin de conversion, por lo
 * que hacer polling de ADC_IsEndConversion(RETURN_STATUS) desde foreground
 * tiene una carrera: la ISR puede consumir todos los flags y el viejo helper
 * terminaba devolviendo un cero inventado. El Delta-Sigma conserva memoria
 * de su propio filtro decimador al cambiar AMux: 2 ms alcanzaban para obtener
 * un dato nuevo, pero no uno asentado (el error era especialmente visible al
 * pasar de SUM a LP). Treinta milisegundos dejan unas 78 conversiones reales
 * y solo agregan ~120 ms al precheck completo de cuatro etapas. */
#define CAL_ADC_DIRECT_CONVERSION_MS 30u
#endif

/* El servo de offset necesita el valor DC del tap, no la ruta DMA/DFB que se
 * reconfigura al entrar y salir de adquisicion. En banco el ADC directo dio
 * medidas repetibles, mientras el ultimo valor del DFB podia corresponder al
 * transitorio de la ruta anterior y cerraba falsamente cerca del target. */
#ifndef CAL_PI_USE_DIRECT_ADC
#define CAL_PI_USE_DIRECT_ADC 1u
#endif

static int32 cal_adc_read_direct_counts(void)
{
    int32 sample;

    ADC_StopConvert();
    ADC_StartConvert();
    CyDelay(CAL_ADC_DIRECT_CONVERSION_MS);
    sample = ADC_GetResult32();
    ADC_StopConvert();

    return psoc_adc_counts_right_aligned(sample);
}

static uint8 cal_pi_take_control_sample(int32 *sample)
{
#if CAL_PI_USE_DIRECT_ADC
    if (sample == (int32 *)0) {
        return 0u;
    }
    *sample = cal_adc_read_direct_counts();
    return 1u;
#else
    return psoc_adc_take_isr_filtered_sample(sample);
#endif
}

static int32 cal_pi_compare_counts(int32 measured)
{
#if PSOC_HW_CLASS == PSOC_HW_HAMMER
    return abs_counts(measured);
#else
    /* Los taps GEO descansan fisicamente sobre Vref ~= 1 V; el cero que se
     * calibra es la componente diferencial alrededor de ese modo comun. El
     * ADC entrega el nivel absoluto (unas 52429 cuentas/V en CF_2V5), por lo
     * que centrar aqui mantiene los targets de las tablas expresados en el
     * dominio correcto: 0 counts == tap exactamente en Vref. La telemetria
     * conserva `measured` crudo y expone `cmp` ya centrado. */
    return measured - CAL_TARGET_1V_COUNTS;
#endif
}

#if CAL_DIAG_SWEEP_ENABLE
static void cal_diag_sweep_stage(const PsocCalStage *stage)
{
    /* Los tres puntos del barrido de diagnostico ahora recorren el rango con
     * signo: un extremo, el centro (que es Vref) y el otro extremo. */
    static const int16 sweep_dac[3] = { -PSOC_IDAC_SIGNED_MAX, 0, PSOC_IDAC_SIGNED_MAX };
    uint8 i;

    for (i = 0u; i < 3u; i++) {
        stage->write(sweep_dac[i]);
        CyDelay(CAL_DIAG_SWEEP_SETTLE_MS);
        cal_diag_i16(PSOC_EVT_CAL_SWEEP_DAC, (int32)sweep_dac[i]);
        cal_diag_i32(PSOC_EVT_CAL_SWEEP_MEAS32, cal_adc_read_direct_counts());
    }
}
#endif

/* Rango de la etapa, EN CODIGOS CON SIGNO. El piso puede ser negativo: con
 * dac_center = 0 y dac_max_change = 255 el rango es -255..+255, que es todo el
 * punto de haber cableado polarity_reg. Con la version sin signo el piso daba 0
 * y la mitad negativa quedaba muerta sin que nada lo dijera. */
/* --------------------------------------------------------------------------
 * RANGO UTIL DE LA ETAPA 0, QUE NO ES SU RANGO DE CODIGOS
 *
 * Medido el 2026-09-05 (EXP1): la pendiente del IDAC de la etapa 0 sobre su
 * propio tap escala con la ganancia del PGA, porque inyecta ANTES de amplificar.
 *
 *     PGA        x1      x8     x16     x32     x50
 *     uV/codigo  62,0   460,5   927,5  1774    2688
 *
 * Eso es una buena noticia -el offset de entrada y la autoridad para corregirlo
 * crecen juntos, asi que la razon es constante y x50 se puede calibrar- pero
 * tiene una consecuencia que el firmware no contemplaba: la EXCURSION del tap no
 * crece. Los rieles medidos son 759,4 y 1114,4 mV, o sea 355 mV, y a x50 eso son
 *
 *     355 mV / 2688 uV por codigo = 132 codigos, +-66 alrededor del centro.
 *
 * Las otras dos terceras partes del rango del IDAC no existen: son riel. Y
 * contra el riel la pendiente es CERO, asi que el lazo empuja sin efecto, la
 * integral se enrolla y el DAC termina en el extremo. Es el modo de falla que
 * mas veces se observo.
 *
 * Por eso el clamp de la etapa 0 escala con 1/ganancia. El numero sale del
 * cociente entre la excursion medida y la pendiente medida, no de un margen
 * elegido a ojo:
 *
 *     codigos utiles = excursion_uV / (uV_por_codigo_a_x1 * ganancia)
 *
 * Se deja un 20 % de margen porque la excursion depende de donde este parado el
 * offset, que es justo lo que la calibracion todavia no sabe cuando arranca.
 * -------------------------------------------------------------------------- */

/* Excursion medida del tap de la etapa 0, en uV: 1114,4 - 759,4 mV. */
#ifndef CAL_STAGE0_EXCURSION_UV
#define CAL_STAGE0_EXCURSION_UV 355000L
#endif
/* Pendiente del IDAC de la etapa 0 sobre su tap a PGA x1, en uV por codigo. */
#ifndef CAL_STAGE0_UV_POR_CODIGO_X1
#define CAL_STAGE0_UV_POR_CODIGO_X1 62L
#endif

static int16 cal_stage0_max_change(void)
{
    int32 pga_x1000 = (int32)psoc_hw_pga_gain_x1000();
    int32 utiles;

    if (pga_x1000 <= 0L) {
        return (int16)PSOC_IDAC_SIGNED_MAX;
    }
    /* (excursion / 2) / (uV_por_codigo * ganancia), con el 20 % de margen.
     * Todo en int64 para que no desborde con ganancias chicas. */
    utiles = (int32)(((int64)CAL_STAGE0_EXCURSION_UV * 1000LL * 4LL) /
                     ((int64)CAL_STAGE0_UV_POR_CODIGO_X1 * (int64)pga_x1000 * 2LL * 5LL));
    if (utiles > (int32)PSOC_IDAC_SIGNED_MAX) {
        utiles = (int32)PSOC_IDAC_SIGNED_MAX;
    }
    /* Nunca menos de 24 codigos: por debajo de eso la etapa no podria corregir
     * ni su propio offset y el lazo abortaria por una cota que puso el firmware,
     * que es peor que abortar por el hardware. A x50 la cuenta da 105, asi que
     * este piso no se activa en ningun caso real; esta como red. */
    if (utiles < 24L) {
        utiles = 24L;
    }
    return (int16)utiles;
}

/* Cuanto puede moverse esta etapa respecto de su centro. Para la etapa 0 sale
 * de la cuenta de arriba; para las demas, de su tabla. */
static int16 cal_stage_max_change(const PsocCalStage *stage)
{
    if (stage->adc_channel == 0u) {
        int16 medido = cal_stage0_max_change();
        return (medido < stage->dac_max_change) ? medido : stage->dac_max_change;
    }
    return stage->dac_max_change;
}

static int16 cal_stage_min_dac(const PsocCalStage *stage)
{
    int32 lo = (int32)stage->dac_center - (int32)stage->dac_max_change;

    if (lo < -(int32)PSOC_IDAC_SIGNED_MAX) {
        lo = -(int32)PSOC_IDAC_SIGNED_MAX;
    }
    return (int16)lo;
}

static int16 cal_stage_max_dac(const PsocCalStage *stage)
{
    int32 hi = (int32)stage->dac_center + (int32)cal_stage_max_change(stage);

    if (hi > (int32)PSOC_IDAC_SIGNED_MAX) {
        hi = (int32)PSOC_IDAC_SIGNED_MAX;
    }
    return (int16)hi;
}

static int16 cal_stage_clamp_dac(const PsocCalStage *stage, int16 dac)
{
    int16 lo = cal_stage_min_dac(stage);
    int16 hi = cal_stage_max_dac(stage);

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
        int16 clamped = cal_stage_clamp_dac(&g_psoc_cal_stages[stage_index], dac);
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

uint8 psoc_calibration_set_stage_dac(uint8 stage_index, int16 dac)
{
    if (stage_index >= PSOC_CAL_STAGE_COUNT) {
        return 0u;
    }

    cal_stage_write_result(stage_index, dac);
    /* El valor medido anterior ya no describe el codigo que acaba de
     * aplicarse. No permitir que un ajuste manual parezca calibrado. */
    g_psoc_cal_results[stage_index].final_measured = 0L;
    g_psoc_cal_results[stage_index].ok = 0u;
    return 1u;
}

void psoc_calibration_report_adc_snapshot(void)
{
    uint8 i;

    cal_diag(PSOC_EVT_BOOT, PSOC_HW_CLASS);

    for (i = 0u; i < PSOC_CAL_STAGE_COUNT; i++) {
        const PsocCalStage *stage = &g_psoc_cal_stages[i];
        int16 saved_dac = cal_stage_current_dac(i);

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
        int16 center = cal_stage_center_dac(&g_psoc_cal_stages[i]);
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
        int16 center = cal_stage_center_dac(&g_psoc_cal_stages[i]);
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
        int16 center = cal_stage_center_dac(&g_psoc_cal_stages[i]);
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
        int16 center = cal_stage_center_dac(&g_psoc_cal_stages[i]);
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
    int32 deadband_counts;  /* banda fisica del tap, en counts del ADC */
    uint16 lock_samples;    /* M muestras en la misma celda de error */
    /* POR QUE ESTOS TRES SON uint32 Y NO uint16
     * La planta tarda tau ~ 31 s en asentarse (R4*C1 = 43k x 680 uF = 29,2 s
     * del esquematico, 31,3 s medidos el 2026-09-03). A 2604 Hz, 5 tau son
     * 407.526 muestras. En uint16 el techo es 65.535 = 25,2 s: no entra ni UN
     * tau. Y el modo de falla es el peor posible, porque no avisa: 407.526
     * truncado a 16 bits da 14.310 muestras = 5,5 s. Compila limpio, corre, y
     * espera veintiocho veces menos de lo pedido. */
    uint32 settle_samples;  /* espera del FIR al cambiar AMux/VDAC: 128 muestras */
    uint32 plant_settle_samples; /* espera de la PLANTA, ver abajo */
    uint32 timeout_samples; /* techo de muestras de PI para esta etapa */
    uint8 refine_enable;    /* prueba final de +/-1 codigo VDAC */
    uint32 refine_settle_samples;
} PsocCalPiCfg;

/* SETTLE vs PLANT_SETTLE: son dos esperas distintas que estaban colapsadas en
 * una sola constante, y esa confusion es la causa de fondo del problema.
 *
 *   settle_samples       vacia el FIR. Son exactamente FILTER_FIR_NTAPS = 128
 *                        muestras (49 ms), porque el filtro tiene ganancia DC
 *                        uno y ese es el tiempo que tarda su ventana en
 *                        contener solo muestras posteriores al cambio. Este
 *                        numero SIEMPRE estuvo bien.
 *
 *   plant_settle_samples espera a que la CADENA ANALOGICA se asiente despues de
 *                        que una etapa de aguas arriba movio su referencia. Es
 *                        un multiplo de tau, del orden de 10^5 muestras. Este
 *                        numero NO EXISTIA: no es que estuviera mal calculado,
 *                        es que el concepto faltaba.
 *
 * Sin el segundo, cada etapa medía su tap mientras todavía se movía por lo que
 * acababa de hacer la anterior, y el lazo anulaba perfectamente un valor
 * equivocado. De ahi los tres sintomas: oscila, el watchdog aborta, y lo que
 * consigue no se sostiene. */

typedef struct {
    PsocCalPiState state;
    int32 integral;
    int32 last_fir_output;
    int32 last_error_dac;
    int32 last_error_bucket;
    /* uint32 por el mismo motivo que en PsocCalPiCfg: con la espera de planta
     * estos contadores pasan de 10^5, y en uint16 daban la vuelta en silencio. */
    uint32 samples_taken;
    uint16 stable_count;
    uint32 settle_remaining;
    uint32 control_hold_remaining; /* memoria FIR pendiente tras mover el DAC */
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
    /* GEO_SUM_LP: el ADDER visto desde ch3. La ganancia NO es la de su propio
     * tap (411) sino la que tiene sobre ch3: 3823 uV/codigo medidos, sobre un
     * escalon de 1875 uV en la referencia, dan 2039. El signo es NEGATIVO
     * porque subir la referencia del ADDER BAJA la salida del LP.
     *
     * El 3823 tambien es una correccion: la matriz del 2026-09-04 decia 2120,9,
     * medido con un escalon de +120 codigos que metia al LP contra el riel, asi
     * que la exponencial se ajusto sobre una respuesta recortada. */
    { CAL_PI_KP_NUM_GEO_SUM, CAL_PI_KP_DIV_GEO_SUM, CAL_PI_KI_NUM_GEO_SUM, CAL_PI_KI_DIV_GEO_SUM, -2039L, CAL_PI_DEADBAND_GEO_SUM_COUNTS, CAL_PI_LOCK_SAMPLES_GEO_SUM, CAL_PI_SETTLE_SAMPLES_GEO_SUM, CAL_PI_PLANT_SETTLE_SAMPLES_GEO_SUM, CAL_PI_TIMEOUT_SAMPLES_GEO_SUM, CAL_PI_REFINE_ENABLE_GEO_SUM, CAL_PI_REFINE_SETTLE_SAMPLES_GEO_SUM },
    /* GEO_LP: 525 uV/codigo sobre 1875 dan 280. */
    { CAL_PI_KP_NUM_GEO_LP, CAL_PI_KP_DIV_GEO_LP, CAL_PI_KI_NUM_GEO_LP, CAL_PI_KI_DIV_GEO_LP, 280L, CAL_PI_DEADBAND_GEO_LP_COUNTS, CAL_PI_LOCK_SAMPLES_GEO_LP, CAL_PI_SETTLE_SAMPLES_GEO_LP, CAL_PI_PLANT_SETTLE_SAMPLES_GEO_LP, CAL_PI_TIMEOUT_SAMPLES_GEO_LP, CAL_PI_REFINE_ENABLE_GEO_LP, CAL_PI_REFINE_SETTLE_SAMPLES_GEO_LP },
};
#else
static const PsocCalPiCfg g_cal_pi_cfg[PSOC_CAL_STAGE_COUNT] = {
    { CAL_PI_KP_NUM_HAMMER_PGA, CAL_PI_KP_DIV_HAMMER_PGA, CAL_PI_KI_NUM_HAMMER_PGA, CAL_PI_KI_DIV_HAMMER_PGA, CAL_PI_GAIN_HAMMER_PGA_X1000, CAL_PI_DEADBAND_HAMMER_PGA_COUNTS, CAL_PI_LOCK_SAMPLES_HAMMER_PGA, CAL_PI_SETTLE_SAMPLES_HAMMER_PGA, CAL_PI_PLANT_SETTLE_SAMPLES_HAMMER_PGA, CAL_PI_TIMEOUT_SAMPLES_HAMMER_PGA, CAL_PI_REFINE_ENABLE_HAMMER_PGA, CAL_PI_REFINE_SETTLE_SAMPLES_HAMMER_PGA },
    { CAL_PI_KP_NUM_HAMMER_LP, CAL_PI_KP_DIV_HAMMER_LP, CAL_PI_KI_NUM_HAMMER_LP, CAL_PI_KI_DIV_HAMMER_LP, CAL_PI_GAIN_HAMMER_LP_X1000, CAL_PI_DEADBAND_HAMMER_LP_COUNTS, CAL_PI_LOCK_SAMPLES_HAMMER_LP, CAL_PI_SETTLE_SAMPLES_HAMMER_LP, CAL_PI_PLANT_SETTLE_SAMPLES_HAMMER_LP, CAL_PI_TIMEOUT_SAMPLES_HAMMER_LP, CAL_PI_REFINE_ENABLE_HAMMER_LP, CAL_PI_REFINE_SETTLE_SAMPLES_HAMMER_LP },
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
/* --------------------------------------------------------------------------
 * TAU DE LA PLANTA EN TIEMPO DE EJECUCION
 *
 * Estos dos son variables y no constantes por una razon medida, no por gusto:
 * tau depende de la temperatura -C1 es un electrolitico- y en la ubicacion del
 * nodo la temperatura va de 8 a 32 C en la misma semana. Una constante de banco
 * tomada a 19 C no cubre eso.
 *
 * La decision de Elias fue NO corregir por temperatura sino MEDIR tau en el
 * propio nodo y dimensionar la espera con lo medido. Estas variables son el
 * lugar donde aterriza esa medicion.
 *
 * Son uint16 por pedido explicito, y por eso tau esta en ms y no en muestras.
 * -------------------------------------------------------------------------- */
static uint16 g_cal_tau_ms          = CAL_PI_TAU_MS;
static uint16 g_cal_plant_tau_x10   = CAL_PI_PLANT_SETTLE_TAU_X10;

/* POR QUE NO HAY UNA ESPERA POR ETAPA. Habia un campo plant_settle_samples por
 * etapa, y sobra: la matriz de acople del 2026-09-04 midio tau en los SEIS
 * pares (etapa, tap) con senal util y dio 26,4 a 33,4 s, media 29,5. Es UN SOLO
 * polo -C1 contra R4- visto desde distintos lugares, no uno por etapa. Cuatro
 * constantes para un solo polo son cuatro oportunidades de que se
 * desincronicen. */
uint32 psoc_cal_plant_settle_samples(void)
{
    uint32 tau_muestras = CAL_PI_TAU_SAMPLES_FROM_MS(g_cal_tau_ms);
    /* Dividir antes de multiplicar: asi sigue entrando en uint32 aunque alguien
     * pida 10 tau. */
    return (tau_muestras / 10UL) * (uint32)g_cal_plant_tau_x10;
}

uint16 psoc_cal_get_tau_ms(void)
{
    return g_cal_tau_ms;
}

/* Devuelve 0 si el valor pedido es absurdo, para que un tau mal medido no pueda
 * dejar la calibracion esperando un tiempo ridiculo. Los limites son anchos a
 * proposito: 1 s a 60 s cubre el rango de temperatura de campo con margen
 * grande, y lo que se quiere atajar es un error de medicion, no afinar. */
uint8 psoc_cal_set_tau_ms(uint16 tau_ms)
{
    if (tau_ms < 1000u || tau_ms > 60000u) {
        return 0u;
    }
    g_cal_tau_ms = tau_ms;
    return 1u;
}

uint16 psoc_cal_get_plant_tau_x10(void)
{
    return g_cal_plant_tau_x10;
}

/* 0 = sin espera de planta (comportamiento viejo, util para comparar).
 * El techo de 100 son 10 tau, muy por encima de los 2 tau que fijo Elias. */
uint8 psoc_cal_set_plant_tau_x10(uint16 x10)
{
    if (x10 > 100u) {
        return 0u;
    }
    g_cal_plant_tau_x10 = x10;
    return 1u;
}

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

static int32 cal_pi_deadband_counts(uint8 stage_index)
{
#ifdef CAL_PI_FORCE_MIN_DEADBAND
    (void)stage_index;
    return CAL_PI_DEADBAND_MIN_COUNTS;
#else
    int32 deadband = g_cal_pi_cfg[stage_index].deadband_counts;

#if PSOC_HW_CLASS == PSOC_HW_GEO
    if (stage_index == 0u) {
        uint16 gain_x1000 = psoc_hw_pga_gain_x1000();
        if (gain_x1000 >= 6000u) {
            deadband = CAL_PI_DEADBAND_GEO_PGA_8X_COUNTS;
        } else if (gain_x1000 >= 3000u) {
            deadband = CAL_PI_DEADBAND_GEO_PGA_4X_COUNTS;
        } else if (gain_x1000 >= 1500u) {
            deadband = CAL_PI_DEADBAND_GEO_PGA_2X_COUNTS;
        } else {
            deadband = CAL_PI_DEADBAND_GEO_PGA_1X_COUNTS;
        }
    }
#endif
    if (deadband < CAL_PI_DEADBAND_MIN_COUNTS) {
        deadband = CAL_PI_DEADBAND_MIN_COUNTS;
    }
    return deadband;
#endif
}

static int32 cal_pi_error_bucket(int32 error_counts, int32 deadband_counts)
{
    int32 abs_error;
    int32 span;
    int32 bucket;

    span = (deadband_counts <= 0L) ? 1L : deadband_counts;
    abs_error = abs_counts(error_counts);
    if (abs_error <= span) {
        return 0L;
    }
    bucket = 1L + ((abs_error - span - 1L) / span);
    return (error_counts < 0L) ? -bucket : bucket;
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
    int32 deadband_counts = cal_pi_deadband_counts(stage_index);

    return (abs_counts(error_counts) <= deadband_counts &&
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
        int16 dac = cal_stage_current_dac(i);
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
    /* value permanece en counts hasta esta division. La version anterior lo
     * redondeaba primero a un codigo IDAC entero y recien despues dividia por
     * la ganancia: en GEO_PGA eso descartaba correcciones reales de varios
     * LSB del actuador. Esta expresion encadena counts->uV->IDAC->etapa en un
     * unico cociente int64 y conserva toda la resolucion. */
    return sign * cal_round_div_i64(
        (int64)value * (int64)num * (int64)CAL_ADC_SPAN_UV * 1000LL,
        (int64)div * (int64)CAL_ADC_LEVELS *
        (int64)CAL_IDAC_UV_PER_LSB * (int64)gain_x1000);
}

#if PSOC_HW_CLASS == PSOC_HW_GEO
/* Transferencia firmada Vref_LP -> tap, medida en la placa el 2026-09-03.
 * Los valores son delta-counts respecto del codigo cero. Se usa solamente la
 * pendiente de cuerda entre el seed y el esfuerzo candidato: el PI sigue
 * cerrando el lazo con la medida, pero deja de suponer que la ganancia es
 * constante cuando en la mitad negativa cambia por un factor 2,6. */
static const int16 g_cal_geo_lp_codes[] = {
    -255, -224, -192, -160, -128, -96, -64, 0,
      32,   64,   96,  128,  160, 192, 224, 255
};
static const int16 g_cal_geo_lp_delta_counts[] = {
    -13367, -12582, -10319, -7995, -5761, -3649, -1870, 0,
       930,   1813,   2706,  3578,  4424,  5279,  6132, 6367
};
#define CAL_GEO_LP_CURVE_POINTS \
    ((uint8)(sizeof(g_cal_geo_lp_codes) / sizeof(g_cal_geo_lp_codes[0])))

static int32 cal_pi_geo_lp_response_counts(int16 code)
{
    uint8 i;

    if (code <= g_cal_geo_lp_codes[0]) {
        return (int32)g_cal_geo_lp_delta_counts[0];
    }
    for (i = 1u; i < CAL_GEO_LP_CURVE_POINTS; i++) {
        int16 x1 = g_cal_geo_lp_codes[i];
        if (code <= x1) {
            int16 x0 = g_cal_geo_lp_codes[i - 1u];
            int32 y0 = (int32)g_cal_geo_lp_delta_counts[i - 1u];
            int32 y1 = (int32)g_cal_geo_lp_delta_counts[i];
            return y0 + cal_round_div_i64((int64)(code - x0) * (int64)(y1 - y0),
                                          (int64)(x1 - x0));
        }
    }
    return (int32)g_cal_geo_lp_delta_counts[CAL_GEO_LP_CURVE_POINTS - 1u];
}

static int32 cal_pi_geo_lp_chord_gain_x1000(int16 base_dac, int16 target_dac)
{
    int32 delta_counts;
    int32 delta_code;
    int32 gain;

    if (target_dac == base_dac) {
        if (target_dac < PSOC_IDAC_SIGNED_MAX) {
            target_dac++;
        } else {
            target_dac--;
        }
    }
    delta_counts = cal_pi_geo_lp_response_counts(target_dac) -
                   cal_pi_geo_lp_response_counts(base_dac);
    delta_code = (int32)target_dac - (int32)base_dac;
    gain = abs_counts(cal_round_div_i64(
        (int64)delta_counts * (int64)CAL_ADC_SPAN_UV * 1000LL,
        (int64)delta_code * (int64)CAL_ADC_LEVELS *
        (int64)CAL_IDAC_UV_PER_LSB));
    if (gain < 200L) { gain = 200L; }
    if (gain > 850L) { gain = 850L; }
    return gain;
}
#endif

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
    int16 dac_lo;
    int16 dac_hi;
    int16 trial_dac;

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
        trial_dac = (int16)(g_cal_pi.dac_current + 1);
    } else {
        if (g_cal_pi.dac_current <= dac_lo) {
            return cal_pi_finalize_stage(ok, g_cal_pi.dac_current, g_cal_pi.last_fir_output);
        }
        trial_dac = (int16)(g_cal_pi.dac_current - 1);
    }

    g_cal_pi.refine_ok = ok;
    g_cal_pi.refine_base_dac = g_cal_pi.dac_current;
    g_cal_pi.refine_trial_dac = trial_dac;
    g_cal_pi.refine_base_measured = g_cal_pi.last_fir_output;
    g_cal_pi.refine_base_abs_error = cal_pi_abs_error_counts(stage, g_cal_pi.last_fir_output);
#if CAL_PI_USE_DIRECT_ADC
    g_cal_pi.settle_remaining = 1u;
#else
    g_cal_pi.settle_remaining = cfg->refine_settle_samples;
#endif
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
    int32 deadband_counts;

#if CAL_PI_USE_DIRECT_ADC
    (void)cfg;
#endif

    cal_diag(PSOC_EVT_CAL_STAGE_BEGIN, g_cal_async.stage_index);
    cal_diag_i32(PSOC_EVT_CAL_STAGE_TARGET32, stage->target_counts);
    stage_gain_x1000 = cal_pi_stage_gain_x1000(g_cal_async.stage_index);
    deadband_counts = cal_pi_deadband_counts(g_cal_async.stage_index);
    cal_diag_i32(PSOC_EVT_CAL_PI_GAIN32, stage_gain_x1000);
    cal_diag(PSOC_EVT_CAL_PI_DEADBAND,
             (deadband_counts > 255L) ? 255u : (uint8)deadband_counts);

    g_cal_pi.integral = 0L;
    g_cal_pi.samples_taken = 0u;
    g_cal_pi.stable_count = 0u;
    g_cal_pi.have_last_error = 0u;
    g_cal_pi.last_error_dac = 0L;
    g_cal_pi.last_error_bucket = 0L;
    g_cal_pi.last_dac_target = 0u;
    g_cal_pi.last_fir_output = 0L;
    g_cal_pi.empty_polls = 0UL;
#if CAL_PI_USE_DIRECT_ADC
    /* Una lectura descartada despues de seleccionar AMux. Cada lectura
     * directa ya mantiene conversiones durante CAL_ADC_DIRECT_CONVERSION_MS. */
    g_cal_pi.settle_remaining = 1u;
#else
    /* La espera de planta sale de la variable de ejecucion, no de la tabla:
     * asi la automedicion de tau tiene efecto sin recompilar. */
    g_cal_pi.settle_remaining = cfg->settle_samples + psoc_cal_plant_settle_samples();
#endif
    g_cal_pi.control_hold_remaining = 0u;
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

    /* Al cambiar de AMux, la linea de retardo contiene otra etapa. Borrarla
     * permite que el settle sea exactamente la longitud del FIR, no un margen
     * empirico de 512 muestras. */
    psoc_filter_reset_history();

    /* El PI no promedia/EMA en software: extrae el DC con el FIR de hardware
     * (Canal A del Filter, coeficientes de FIR_calibration.h cargados en
     * cal_pi_start) -- ADC -> Filter_STAGEA -> DMA_Filter_RAM, leido muestra
     * a muestra en cal_pi_run_service. */
#if CAL_PI_USE_DIRECT_ADC
    dma_route_select(0u);
#else
    dma_route_select(1u);
#endif
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

    if (!cal_pi_take_control_sample(&sample)) {
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

    if (!cal_pi_take_control_sample(&sample)) {
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
    int32 deadband_counts;
    int32 error_bucket;
    int32 dac_max_step_up;
    int32 dac_max_step_down;
    /* uint32 igual que samples_taken: si se queda en uint16 vuelve a
     * truncar justo lo que se acaba de ensanchar. Hoy no se nota porque
     * el contador se resetea al terminar el settle y el timeout es 45000,
     * pero es la misma trampa esperando a que alguien suba el techo. */
    uint32 sample_index;
    uint16 lock_n;
    int16 dac_sample;
    int16 dac_lo;
    int16 dac_hi;
    int16 dac_target;
    int16 dac_step;
    uint8 can_integrate;
    uint8 dac_changed;
    int8 effort_delta_sign;
#if PSOC_HW_CLASS == PSOC_HW_GEO
    uint8 gain_iteration;
    int16 gain_probe_dac;
#endif

    if (!cal_pi_take_control_sample(&sample)) {
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
    sample_index = g_cal_pi.samples_taken + 1UL;
    g_cal_pi.samples_taken = sample_index;

    /* Cada salida del FIR mezcla 128 entradas. Despues de mover el DAC se
     * retiene el siguiente ajuste hasta que todas correspondan al codigo
     * nuevo; sin esto el PI persigue una medida vieja y llega a los rieles. */
    if (g_cal_pi.control_hold_remaining > 0u) {
        g_cal_pi.control_hold_remaining--;
        if (g_cal_pi.control_hold_remaining > 0u) {
            if (g_cal_pi.samples_taken >= cfg->timeout_samples) {
                return cal_pi_finish_stage(0u);
            }
            return 0u;
        }
    }

    /* error_dac se conserva solo para telemetria compatible. El control y la
     * banda trabajan en counts para no perder la parte fraccionaria. */
    error_dac = cal_counts_error_to_dac_scale(error_counts);
    stage_gain_x1000 = cal_pi_stage_gain_x1000(g_cal_async.stage_index);
    deadband_counts = cal_pi_deadband_counts(g_cal_async.stage_index);
    control_error = (abs_counts(error_counts) <= deadband_counts) ? 0L : error_counts;
    error_bucket = cal_pi_error_bucket(error_counts, deadband_counts);
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
#if CAL_PI_USE_DIRECT_ADC
        /* Con una observacion DC ya asentada, aplicar la correccion sobre el
         * codigo actual es un paso de Newton. El PI posicional basado siempre
         * en base_dac necesitaba cargar integral durante cientos de lecturas y
         * terminaba siguiendo el polo de BP en vez del error local. */
        effort = (int32)dac_sample + (int32)stage->direction * p_term;
#endif
#if PSOC_HW_CLASS == PSOC_HW_GEO
        if (stage->adc_channel == 3u) {
            for (gain_iteration = 0u; gain_iteration < 2u; gain_iteration++) {
                if (effort < (int32)cal_stage_min_dac(stage)) {
                    gain_probe_dac = cal_stage_min_dac(stage);
                } else if (effort > (int32)cal_stage_max_dac(stage)) {
                    gain_probe_dac = cal_stage_max_dac(stage);
                } else {
                    gain_probe_dac = (int16)effort;
                }
                stage_gain_x1000 = cal_pi_geo_lp_chord_gain_x1000(
#if CAL_PI_USE_DIRECT_ADC
                    dac_sample, gain_probe_dac);
#else
                    g_cal_pi.base_dac, gain_probe_dac);
#endif
                p_term = cal_pi_gain_scaled_term(control_error, cfg->kp_num,
                                                  cfg->kp_div, stage_gain_x1000);
                i_term = cal_pi_gain_scaled_term(g_cal_pi.integral, cfg->ki_num,
                                                  cfg->ki_div, stage_gain_x1000);
                effort = (int32)g_cal_pi.base_dac +
                         (int32)stage->direction * (p_term + i_term);
#if CAL_PI_USE_DIRECT_ADC
                effort = (int32)dac_sample +
                         (int32)stage->direction * p_term;
#endif
            }
        }
#endif
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
    if (dac_changed) {
#if CAL_PI_USE_DIRECT_ADC
        /* La siguiente lectura directa ya integra 30 ms de conversiones. */
        g_cal_pi.control_hold_remaining = 0u;
#else
        g_cal_pi.control_hold_remaining = cfg->settle_samples;
#endif
    }

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
    int16 seed_dac[PSOC_CAL_MAX_STAGES];

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
