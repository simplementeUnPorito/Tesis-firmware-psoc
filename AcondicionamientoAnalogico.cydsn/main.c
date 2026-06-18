/*******************************************************************************
* main.c — Firmware PSoC unificado para geófono (GEO) y martillo (HAMMER).
*
* El hardware del TopDesign determina automáticamente el modo de compilación:
*   Si el proyecto tiene PGAgain  → PSOC_HW_CLASS = GEO
*   Si el proyecto tiene PGA      → PSOC_HW_CLASS = HAMMER
* No hay que cambiar nada en este archivo al cambiar de proyecto.
*
* ─────────────────────────────────────────────────────────────────────────────
*  GEO — componentes TopDesign:
*    UART, ADC, AMux_ADC, VDAC_ref_*, PGAgain, OPAref, PGAp,
*    PGAn, LPF_1, LPF_2, OPAbp, OPAadder, OPAlp, Timer, LED, isr_*.
*
*  HAMMER — componentes TopDesign:
*    UART, ADC, AMux_ADC, VDAC_ref_IN, VDAC_PGA, VDAC_LP,
*    PGA, Opa_ref_IN, Opa_ref_PGA, Opa_LP, LPF_ADC, LPF_ref, Timer, LED.
* ─────────────────────────────────────────────────────────────────────────────
*
* Cadena: MATLAB → maestro → esclavo (ESP-NOW) → PSoC (UART).
* El PSoC muestrea N lotes de 30 muestras RAW, los guarda en RAM
* sin UART durante la ventana crítica, y recién después los envía al ESP.
* El arranque del muestreo es por el pin digital SYNC_IN (lo maneja el ESP).
*
* TX UART — frame de datos RAW (95 bytes):
*   [0xAB][n=30][seq_lo][seq_hi] + 30×3 bytes (raw LE: b0,b1,b2) + [crc XOR]
*
* RX UART — comandos con checksum XOR:
*   1 param : [0xAB][cmd][param][cmd^param]
*       0xA5 status/probe   0xA6 set PGA (0-8)       0xA9 legacy ACK
*       0xAA legacy ACK     0xB1 arm (espera SYNC)   0xB3 debug rampa (0/1)
*       0xB4 start-now      0xB5 calibrar refs       0xC1 pong
*   2 param : [0xAB][0xA3][n_lo][n_hi][0xA3^n_lo^n_hi]   (set N lotes)
*
* Detección PSoC ↔ ESP:
*   PSoC → ESP : [0xAB][0xC0][0x00][0xC0]             (ping)
*   ESP  → PSoC: [0xAB][0xC1][0x00][0xC1]             (pong)
*   PSoC → ESP : [0xAB][0xC2][cmd][val][0xC2^cmd^val]  (config ACK)
*******************************************************************************/

#include "project.h"
#include "psoc_hw.h"
#include "psoc_adc.h"
#include "calibration.h"
#include "psoc_nv.h"
#include "LED.h"
#include "DMA_DelSig_RAM_Single_dma.h"
#include "DMA_DelSig_RAM_Lote_dma.h"
#include "DMA_Filter_RAM_Single_dma.h"
#include "DMA_Filter_RAM_Lote_dma.h"
#include "DMA_DelSig_Filter_dma.h"
#include "Filter.h"
#include "Filter_PVT.h"
#include "Reg_Select.h"
/* -------------------------------------------------------------------------- */
#define LEGACY_VDAC_SHADOW_INIT 0x9Cu
#define FILTER_FIR_NTAPS   128u
#define FILTER_GROUP_DELAY ((FILTER_FIR_NTAPS - 1u) / 2u)   /* 63 muestras */
#define TIMER_TICK_MS      10u
#define TIMEOUT_COUNTS     240000u    /* 10 ms @ 24 MHz — tick de sistema */

#define BATCH_SAMPLES      30u
#define FRAME_BYTES        (4u + BATCH_SAMPLES * 3u + 1u)   /* 95 */
#define SAMPLE_BYTES       3u
#define CAPTURE_BATCH_BYTES (BATCH_SAMPLES * SAMPLE_BYTES)

#ifndef PSOC_CAPTURE_MAX_BATCHES
#define PSOC_CAPTURE_MAX_BATCHES 512u
#endif

/* Tamaño del lote HW (canales DMA_*_RAM_Lote) para promediado por DMA.
 * Hoy ningún consumidor lo usa todavía (la ruta queda en Single); el canal
 * ya está armado e inicializado para cuando se conecte el promediado. */
#ifndef DMA_LOTE_SAMPLES
#define DMA_LOTE_SAMPLES   30u
#endif

#define PING_PERIOD_MS     700u
#define PING_OFF_MS        100u
#define IDLE_PING_MS       1000u
#define COMM_WINDOW_MS     500u
#define COMM_BLINK_MS       50u

#define MS_TO_TICKS(ms)       (((ms) + TIMER_TICK_MS - 1u) / TIMER_TICK_MS)
#define PING_PERIOD_TICKS     MS_TO_TICKS(PING_PERIOD_MS)
#define PING_OFF_TICKS        MS_TO_TICKS(PING_OFF_MS)
#define IDLE_PING_TICKS       MS_TO_TICKS(IDLE_PING_MS)
#define COMM_WINDOW_TICKS     MS_TO_TICKS(COMM_WINDOW_MS)
#define COMM_BLINK_TICKS      MS_TO_TICKS(COMM_BLINK_MS)
#define CONNECT_BLINK_TICKS   MS_TO_TICKS(80u)

#ifndef PSOC_DIAG_ENABLE
#define PSOC_DIAG_ENABLE      1
#endif

#ifndef PSOC_DEBUG_LED
#define PSOC_DEBUG_LED        1
#endif

#ifndef PSOC_EARLY_UART_TEST
#define PSOC_EARLY_UART_TEST  0
#endif

#ifndef PSOC_TX1_BITBANG_TEST
#define PSOC_TX1_BITBANG_TEST  0
#endif

#ifndef PSOC_TX1_GPIO_TEST
#define PSOC_TX1_GPIO_TEST     0
#endif

/* -------------------------------------------------------------------------- */
static volatile int32  g_adc_raw          = 0;

/* Buffers destino DMA — 3 bytes LE (24 bits signed) por muestra */
static volatile uint8  g_dma_raw_buf[3]   = {0u, 0u, 0u};
static volatile uint8  g_dma_filt_buf[3]  = {0u, 0u, 0u};

/* Buffers del canal Lote (mux HW) — reservados para promediado por DMA;
 * la ruta hoy siempre selecciona Single, así que quedan sin tocar. */
static volatile uint8  g_dma_raw_lote_buf[DMA_LOTE_SAMPLES * 3u];
static volatile uint8  g_dma_filt_lote_buf[DMA_LOTE_SAMPLES * 3u];

/* 0 = enviar crudo (default); 1 = enviar filtrado por FIR hardware */
static volatile uint8  g_stream_mode      = 0u;
/* Muestras filtradas a descartar tras SyncIn (compensa retardo de grupo FIR) */
static volatile uint16 g_fir_discard      = 0u;

static volatile uint8  g_capture_raw[PSOC_CAPTURE_MAX_BATCHES][CAPTURE_BATCH_BYTES];
static volatile uint16 g_batch_fill       = 0u;
static volatile uint8  g_batch_ready      = 0u;
static          uint16 g_seq              = 0u;
static volatile uint16 g_batches_captured = 0u;
static volatile uint8  g_capture_done     = 0u;

static uint8  g_pga_code     = PSOC_PGA_DEFAULT_CODE;
static uint8  g_pgavdac_code = PSOC_PGAVDAC_DEFAULT_CODE;

static uint8  g_vdac_val     = LEGACY_VDAC_SHADOW_INIT;

static volatile uint8  g_state        = PSOC_IDLE;
static          uint16 g_n_batches    = 0u;
static volatile uint16 g_batches_sent = 0u;
static volatile uint8  g_debug_psoc   = 0u;
static          uint32 g_dbg_cnt      = 0u;
static volatile uint8  g_cal_ack_pending = 0u;
static volatile uint8  g_cal_button_pressed = 0u;

static volatile uint8  rx_state       = 0u;
static volatile uint8  rx_cmd         = 0u;
static volatile uint8  rx_p1          = 0u;
static volatile uint8  rx_p2          = 0u;
static volatile uint8  watchdog_rx    = 0u;
static volatile uint32 g_timer_ticks  = 0u;

static          uint8  g_esp_connected  = 0u;
static          uint32 g_comm_countdown = 0u;

static volatile uint8 g_rx_watch_ticks = 0u;
#define RX_WATCHDOG_TICKS  1u

/* -------------------------------------------------------------------------- */

static const uint8 g_ping_frame[4] = {
    0xABu, PSOC_CMD_PING, 0x00u, PSOC_CMD_PING
};

static void uart_send_ping(void)
{
    UART_PutArray(g_ping_frame, (uint8)sizeof(g_ping_frame));
}

static void uart_send_cfg_ack(uint8 cmd, uint8 val)
{
    uint8 frame[5u];
    frame[0] = 0xABu;
    frame[1] = PSOC_CMD_CFG_ACK;
    frame[2] = cmd;
    frame[3] = val;
    frame[4] = (uint8)(PSOC_CMD_CFG_ACK ^ cmd ^ val);
    UART_PutArray(frame, (uint8)sizeof(frame));
}

static void uart_send_fs_report(void)
{
    uint8 fs_lo = (uint8)((uint16)ADC_DEFAULT_SRATE & 0xFFu);
    uint8 fs_hi = (uint8)(((uint16)ADC_DEFAULT_SRATE >> 8u) & 0xFFu);
    uint8 frame[5u];
    frame[0] = 0xABu;
    frame[1] = PSOC_CMD_FS_REPORT;
    frame[2] = fs_lo;
    frame[3] = fs_hi;
    frame[4] = (uint8)(PSOC_CMD_FS_REPORT ^ fs_lo ^ fs_hi);
    UART_PutArray(frame, (uint8)sizeof(frame));
}

static void uart_send_diag(uint8 event, uint8 value)
{
#if PSOC_DIAG_ENABLE
    uint8 state = g_state;
    uint8 frame[6u];
    frame[0] = 0xABu;
    frame[1] = PSOC_CMD_DIAG_EVT;
    frame[2] = event;
    frame[3] = value;
    frame[4] = state;
    frame[5] = (uint8)(PSOC_CMD_DIAG_EVT ^ event ^ value ^ state);
    UART_PutArray(frame, (uint8)sizeof(frame));
#else
    (void)event;
    (void)value;
#endif
}

static uint8 diag_u16_sat(uint16 value)
{
    return (value > 255u) ? 255u : (uint8)value;
}

static uint16 capture_target_batches(void)
{
    uint16 n = g_n_batches;
    if (n == 0u || n > PSOC_CAPTURE_MAX_BATCHES) {
        n = PSOC_CAPTURE_MAX_BATCHES;
    }
    return n;
}

static uint8 capture_dump_pending(void)
{
    return (g_batches_sent < g_batches_captured) ? 1u : 0u;
}

static void psoc_prepare_capture_path(void)
{
    psoc_calibration_servo_abort();
    ADC_StopConvert();
    psoc_adc_select_capture_config();
    psoc_calibration_restore_capture_path();
}

static void timer_start_runtime(void)
{
    Timer_Stop();
    Timer_WritePeriod(TIMEOUT_COUNTS);
    Timer_Start();
}

static void timer_stop_quiet(void)
{
    Timer_Stop();
    Timer_ReadStatusRegister();
    isr_Timer_ClearPending();
}

static void capture_reset_locked(void)
{
    g_batch_fill       = 0u;
    g_batch_ready      = 0u;
    g_batches_sent     = 0u;
    g_batches_captured = 0u;
    g_capture_done     = 0u;
    g_dbg_cnt          = 0u;
    g_seq              = 0u;
}

static void psoc_enter_sampling(uint8 debugMode)
{
    uint8 saved;
    uint16 target = capture_target_batches();
    ADC_StopConvert();
    timer_stop_quiet();
    saved = CyEnterCriticalSection();
    g_debug_psoc = debugMode ? 1u : 0u;
    capture_reset_locked();
    g_state = PSOC_SAMPLING;
    CyExitCriticalSection(saved);
    uart_send_diag(PSOC_EVT_SAMPLING_START, diag_u16_sat(target));
    ADC_StartConvert();
}

/* Reconstruye int32 signed a partir de 3 bytes LE (24 bits, extensión de signo) */
static int32 dma_buf_to_i24(const volatile uint8 *buf)
{
    uint32 u = (uint32)buf[0] | ((uint32)buf[1] << 8u) | ((uint32)buf[2] << 16u);
    return (u & 0x00800000UL) ? (int32)(u | 0xFF000000UL) : (int32)u;
}

/* ISR: muestra RAW del ADC (DMA_DelSig_RAM_Single depositó 3 bytes en g_dma_raw_buf) */
CY_ISR(isr_DMA_DelSig_RAM_Handler)
{
    g_adc_raw = psoc_adc_counts_right_aligned(dma_buf_to_i24(g_dma_raw_buf));
    psoc_adc_note_isr_sample(g_adc_raw);   /* mantiene ruta calibración activa */

    if (g_stream_mode != 0u) { return; }   /* modo FIR: el filtro captura */

    if (g_state == PSOC_SAMPLING && g_capture_done == 0u &&
        g_batches_captured < capture_target_batches() &&
        g_batch_fill < BATCH_SAMPLES)
    {
        uint16 pos = (uint16)(g_batch_fill * SAMPLE_BYTES);
        int32 val = g_debug_psoc ? (int32)(g_dbg_cnt++ & 0x00FFFFFFu) : g_adc_raw;
        g_capture_raw[g_batches_captured][pos]      = (uint8)( val        & 0xFFu);
        g_capture_raw[g_batches_captured][pos + 1u] = (uint8)((val >>  8u) & 0xFFu);
        g_capture_raw[g_batches_captured][pos + 2u] = (uint8)((val >> 16u) & 0xFFu);
        g_batch_fill++;
        if (g_batch_fill >= BATCH_SAMPLES)
        {
            g_batch_fill = 0u;
            g_batches_captured++;
            if (g_batches_captured >= capture_target_batches()) {
                g_capture_done = 1u;
            }
        }
    }
}

/* ISR: muestra FILTRADA (DMA_Filter_RAM_Single depositó 3 bytes en g_dma_filt_buf) */
CY_ISR(isr_DMA_Filter_RAM_Handler)
{
    int32 filt;

    if (g_fir_discard > 0u) { g_fir_discard--; return; }  /* descarte retardo grupo FIR */
    if (g_stream_mode == 0u) { return; }                   /* modo RAW: no capturar filtrado */

    filt = dma_buf_to_i24(g_dma_filt_buf);

    if (g_state == PSOC_SAMPLING && g_capture_done == 0u &&
        g_batches_captured < capture_target_batches() &&
        g_batch_fill < BATCH_SAMPLES)
    {
        uint16 pos = (uint16)(g_batch_fill * SAMPLE_BYTES);
        g_capture_raw[g_batches_captured][pos]      = (uint8)( filt        & 0xFFu);
        g_capture_raw[g_batches_captured][pos + 1u] = (uint8)((filt >>  8u) & 0xFFu);
        g_capture_raw[g_batches_captured][pos + 2u] = (uint8)((filt >> 16u) & 0xFFu);
        g_batch_fill++;
        if (g_batch_fill >= BATCH_SAMPLES)
        {
            g_batch_fill = 0u;
            g_batches_captured++;
            if (g_batches_captured >= capture_target_batches()) {
                g_capture_done = 1u;
            }
        }
    }
}

CY_ISR(isr_Timer)
{
    Timer_ReadStatusRegister();
    g_timer_ticks++;
    if (rx_state != 0u) {
        g_rx_watch_ticks++;
        if (g_rx_watch_ticks >= RX_WATCHDOG_TICKS) {
            g_rx_watch_ticks = 0u;
            watchdog_rx = 1u;
        }
    } else {
        g_rx_watch_ticks = 0u;
    }
    Timer_Stop();
    Timer_WritePeriod(TIMEOUT_COUNTS);
    Timer_Start();
}

CY_ISR(isr_SyncIn)
{
    uint8 saved;
#if defined(SYNC_IN_INTSTAT)
    (void)SYNC_IN_ClearInterrupt();
#endif
    if (SYNC_IN_Read())
    {
        uart_send_diag(PSOC_EVT_SYNC_RISE, g_state);
        if (g_state == PSOC_ARMED)
        {
            if (g_stream_mode != 0u) {
                g_fir_discard = FILTER_GROUP_DELAY;  /* compensar retardo grupo FIR */
            }
            psoc_enter_sampling(g_debug_psoc);
        }
    }
    else
    {
        uart_send_diag(PSOC_EVT_SYNC_FALL, g_state);
        ADC_StopConvert();
        timer_start_runtime();
        saved = CyEnterCriticalSection();
        g_batch_fill       = 0u;
        g_batch_ready      = 0u;
        g_batches_sent     = 0u;
        g_batches_captured = 0u;
        g_capture_done     = 0u;
        g_state            = PSOC_IDLE;
        CyExitCriticalSection(saved);
    }
}

CY_ISR(isr_Button_Handler)
{
    isr_Button_ClearPending();
    g_cal_button_pressed = 1u;
}

/* -------------------------------------------------------------------------- */

static void rx_watchdog_start(void) { g_rx_watch_ticks = 0u; watchdog_rx = 0u; }
static void rx_watchdog_stop(void)  { g_rx_watch_ticks = 0u; watchdog_rx = 0u; }

static void led_write(uint8 value)
{
#ifdef CY_PINS_LED_H
    LED_Write(value);
#else
    (void)value;
#endif
}

static void led_toggle(void)
{
#ifdef CY_PINS_LED_H
    if (g_state != PSOC_SAMPLING) {
        LED_Write(LED_Read() ^ 0x01u);
    }
#endif
}

static uint32 timer_now_ticks(void)
{
    uint32 ticks;
    uint8 saved = CyEnterCriticalSection();
    ticks = g_timer_ticks;
    CyExitCriticalSection(saved);
    return ticks;
}

uint32 psoc_now_ticks(void)
{
    return timer_now_ticks();
}

static uint8 ticks_due(uint32 now, uint32 due)
{
    return ((int32)(now - due) >= 0) ? 1u : 0u;
}

/* Selecciona, vía Reg_Select, qué resultado final usan los DMA *_RAM y si cada
 * camino corre en Single (1 muestra/disparo) o Lote (DMA_LOTE_SAMPLES/disparo).
 * La exclusión Single/Lote y raw/Filter la imponen las compuertas del TopDesign;
 * acá solo se escribe el bit pattern que las gobierna (control_0/1/2). */
static void dma_route_select(uint8 use_filter, uint8 raw_lote, uint8 filt_lote)
{
    uint8 reg = 0u;
    if (use_filter) { reg |= 0x02u; }   /* control_1 */
    if (filt_lote)  { reg |= 0x01u; }   /* control_0 */
    if (raw_lote)   { reg |= 0x04u; }   /* control_2 */
    Reg_Select_Write(reg);
}

static void PGAgain_Set(uint8 code)
{
    if (code <= 8u) {
        g_pga_code = code;
        psoc_hw_set_pga(code);
    }
}

static void PGAvdac_Set(uint8 code)
{
    if (code <= 8u) {
        g_pgavdac_code = code;
        psoc_hw_set_pgavdac(code);
    }
}

static void psoc_arm(void)
{
    uint8 saved;
    psoc_prepare_capture_path();
    saved = CyEnterCriticalSection();
    capture_reset_locked();
    g_state = PSOC_ARMED;
    CyExitCriticalSection(saved);
    uart_send_diag(PSOC_EVT_ARMED, diag_u16_sat(g_n_batches));
}

static void psoc_start_now(void)
{
    psoc_prepare_capture_path();
    psoc_enter_sampling(0u);
}

static uint8 psoc_start_calibration_if_idle(uint8 send_ack)
{
    if (g_state != PSOC_IDLE) {
        uart_send_diag(PSOC_EVT_CAL_BUSY, g_state);
        if (send_ack) {
            uart_send_cfg_ack(PSOC_CMD_CALIBRATE, 0u);
        }
        return 0u;
    }
    g_batches_sent = 0u;
    g_batches_captured = 0u;
    g_capture_done = 0u;
    psoc_calibration_servo_abort();
    dma_route_select(0u, 0u, 0u);  /* fuerza raw+Single: calibración necesita ADC crudo */
    uart_send_diag(PSOC_EVT_CAL_START, 0u);
    if (!psoc_calibration_start_async()) {
        uart_send_diag(PSOC_EVT_CAL_DONE, 0u);
        if (send_ack) {
            uart_send_cfg_ack(PSOC_CMD_CALIBRATE, 0u);
        }
        return 0u;
    }
    g_cal_ack_pending = send_ack ? 1u : 0u;
    g_state = PSOC_CALIBRATING;
    return 1u;
}

static uint8 service_button_calibration(void)
{
    if (!g_cal_button_pressed) {
        return 0u;
    }

    g_cal_button_pressed = 0u;
    uart_send_diag(PSOC_EVT_BUTTON, g_state);
    if (g_state == PSOC_IDLE) {
        (void)psoc_start_calibration_if_idle(0u);
    } else {
        uart_send_diag(PSOC_EVT_BUTTON_IGNORED, g_state);
    }
    return 1u;
}

static void uart_send_capture_batch(uint16 batchIndex)
{
    uint8  frame[FRAME_BYTES];
    uint8 *p;
    uint16 i;
    uint16 pos;
    uint8  crc = 0u;

    frame[0] = 0xABu;
    frame[1] = (uint8)BATCH_SAMPLES;
    frame[2] = (uint8)( g_seq        & 0xFFu);
    frame[3] = (uint8)((g_seq >> 8u) & 0xFFu);

    p = &frame[4];
    for (i = 0u; i < BATCH_SAMPLES; i++, p += 3u)
    {
        pos  = (uint16)(i * SAMPLE_BYTES);
        p[0] = g_capture_raw[batchIndex][pos];
        p[1] = g_capture_raw[batchIndex][pos + 1u];
        p[2] = g_capture_raw[batchIndex][pos + 2u];
    }

    for (i = 0u; i < (FRAME_BYTES - 1u); i++) { crc ^= frame[i]; }
    frame[FRAME_BYTES - 1u] = crc;

    g_seq++;
    UART_PutArray(frame, FRAME_BYTES);
    g_comm_countdown = COMM_WINDOW_TICKS;
}

static void uart_service(void)
{
    uint8 rx;
    while (UART_GetRxBufferSize() > 0u)
    {
        rx = UART_ReadRxData();
        switch (rx_state)
        {
            case 0u:
                if (rx == 0xABu) { rx_state = 1u; rx_watchdog_start(); }
                break;

            case 1u:
                rx_watchdog_start();
                switch (rx)
                {
                    case 0xA5u: case 0xA6u: case 0xA9u: case 0xAAu:
                    case 0xB1u: case 0xB3u: case 0xB4u: case PSOC_CMD_CALIBRATE:
                    case PSOC_CMD_SAVE_EEPROM: case PSOC_CMD_SELECT_STREAM:
                    case PSOC_CMD_PONG:
                        rx_cmd = rx; rx_state = 2u; break;
                    case 0xA3u:
                        rx_cmd = rx; rx_state = 4u; break;
                    default:
                        rx_watchdog_stop(); rx_state = 0u; break;
                }
                break;

            case 2u:
                rx_p1 = rx; rx_p2 = 0u; rx_state = 3u; rx_watchdog_start();
                break;

            case 4u:
                rx_p1 = rx; rx_state = 5u; rx_watchdog_start();
                break;

            case 5u:
                rx_p2 = rx; rx_state = 3u; rx_watchdog_start();
                break;

            case 3u:
                rx_watchdog_stop(); rx_state = 0u;
                if (rx != (uint8)(rx_cmd ^ rx_p1 ^ rx_p2)) { break; }

                if (!g_esp_connected) {
                    /* Durante reset/upload del ESP, el TX hacia el PSoC puede
                     * emitir bytes que por casualidad formen un frame valido.
                     * No aceptar comandos de configuracion/captura como
                     * handshake: solo PONG/STATUS abren la comunicacion real. */
                    if (rx_cmd != PSOC_CMD_PONG && rx_cmd != 0xA5u) {
                        break;
                    }
                    g_esp_connected = 1u;
                    uart_send_diag(PSOC_EVT_ESP_SEEN, rx_cmd);
                }
                if (rx_cmd != PSOC_CMD_PONG) {
                    uart_send_diag(PSOC_EVT_RX_CMD, rx_cmd);
                }
                g_comm_countdown = COMM_WINDOW_TICKS;

                if (g_state == PSOC_CALIBRATING) {
                    switch (rx_cmd)
                    {
                        case PSOC_CMD_PONG:
                            break;
                        case 0xA5u:
                            uart_send_diag(PSOC_EVT_STATUS_REQ, 0u);
                            uart_send_ping();
                            uart_send_fs_report();
                            break;
                        case PSOC_CMD_CALIBRATE:
                            uart_send_diag(PSOC_EVT_CAL_BUSY, g_state);
                            uart_send_cfg_ack(PSOC_CMD_CALIBRATE, 0u);
                            break;
                        case 0xA6u: case 0xA9u: case 0xAAu:
                        case 0xB1u: case 0xB3u: case 0xB4u:
                            uart_send_diag(PSOC_EVT_CAL_BUSY, rx_cmd);
                            uart_send_cfg_ack(rx_cmd, 0u);
                            break;
                        default:
                            uart_send_diag(PSOC_EVT_CAL_BUSY, rx_cmd);
                            break;
                    }
                    break;
                }

                switch (rx_cmd)
                {
                    case PSOC_CMD_PONG: break;
                    case 0xA5u:
                        uart_send_diag(PSOC_EVT_STATUS_REQ, 0u);
                        uart_send_ping();
                        uart_send_fs_report();
                        break;
                    case 0xA6u:
                        PGAgain_Set(rx_p1);
                        uart_send_cfg_ack(0xA6u, g_pga_code);
                        led_toggle(); break;
                    case 0xA9u:
                        PGAvdac_Set(rx_p1);
                        uart_send_cfg_ack(0xA9u, g_pgavdac_code);
                        led_toggle(); break;
                    case 0xAAu:
                        /* Legacy command: hardware now uses the four VDAC_ref_* refs. */
                        g_vdac_val = rx_p1;
                        uart_send_cfg_ack(0xAAu, g_vdac_val);
                        led_toggle(); break;
                    case 0xA3u:
                        g_n_batches = (uint16)rx_p1 | ((uint16)rx_p2 << 8u);
                        uart_send_diag(PSOC_EVT_SETN, diag_u16_sat(g_n_batches));
                        break;
                    case 0xB1u:
                        psoc_arm();
                        uart_send_cfg_ack(0xB1u, g_state);
                        led_toggle();
                        break;
                    case 0xB4u:
                        uart_send_diag(PSOC_EVT_START_NOW, 0u);
                        uart_send_cfg_ack(0xB4u, 1u);
                        psoc_start_now();
                        led_toggle();
                        break;
                    case 0xB3u:
                        g_debug_psoc = (rx_p1 == 0u) ? 0u : 1u;
                        uart_send_diag(PSOC_EVT_DEBUG_MODE, g_debug_psoc);
                        if (g_debug_psoc) {
                            psoc_prepare_capture_path();
                            psoc_enter_sampling(1u);
                        } else {
                            ADC_StopConvert();
                            psoc_prepare_capture_path();
                            timer_start_runtime();
                            g_batch_fill   = 0u;
                            g_batch_ready  = 0u;
                            g_capture_done = 0u;
                            g_state        = PSOC_IDLE;
                        }
                        break;
                    case PSOC_CMD_CALIBRATE:
                        (void)psoc_start_calibration_if_idle(1u);
                        led_toggle();
                        break;
                    case PSOC_CMD_SAVE_EEPROM:
                        /* Guardar los DAC de calibración actuales en EEPROM */
                        {
                            uint8 dac_snap[PSOC_NV_CAL_STAGES];
                            uint8 ok_snap = 0u;
                            uint8 k;
                            for (k = 0u; k < PSOC_NV_CAL_STAGES && k < g_psoc_cal_result_count; k++) {
                                dac_snap[k] = g_psoc_cal_results[k].final_dac;
                            }
                            if (g_psoc_cal_result_count >= PSOC_NV_CAL_STAGES) {
                                ok_snap = psoc_nv_save(dac_snap, PSOC_NV_CAL_STAGES);
                            }
                            uart_send_cfg_ack(PSOC_CMD_SAVE_EEPROM, ok_snap);
                        }
                        led_toggle();
                        break;
                    case PSOC_CMD_SELECT_STREAM:
                        /* rx_p1: 0=crudo, 1=filtrado FIR */
                        g_stream_mode = (rx_p1 != 0u) ? 1u : 0u;
                        dma_route_select(g_stream_mode, 0u, 0u);
                        if (g_stream_mode != 0u) {
                            g_fir_discard = 0u;  /* reset — se recarga en próximo SyncIn */
                        }
                        uart_send_cfg_ack(PSOC_CMD_SELECT_STREAM, g_stream_mode);
                        led_toggle();
                        break;
                    default: break;
                }
                break;

            default:
                rx_state = 0u; break;
        }
    }
}

static void service_runtime(void)
{
    if (g_state == PSOC_CALIBRATING) {
        if (psoc_calibration_service_async()) {
            uint8 ok = psoc_calibration_async_result_ok();
            g_state = PSOC_IDLE;
            timer_start_runtime();
            dma_route_select(g_stream_mode, 0u, 0u);  /* restaura ruta previa a la calibración */
            uart_send_diag(PSOC_EVT_CAL_DONE, ok);
            if (g_cal_ack_pending) {
                g_cal_ack_pending = 0u;
                uart_send_cfg_ack(PSOC_CMD_CALIBRATE, ok);
            }
        }
        uart_service();
        return;
    }

    if (g_state == PSOC_SAMPLING) {
        if (g_capture_done == 0u) {
            return;   /* Silencio total: sin UART RX/TX, sin LED, sin pings. */
        }
        ADC_StopConvert();
        timer_start_runtime();
        uart_send_diag(PSOC_EVT_CAPTURE_DONE, diag_u16_sat(g_batches_captured));
        g_capture_done = 0u;
        g_state = PSOC_IDLE;
    }

    if (capture_dump_pending()) {
        if (g_batches_sent == 0u) {
            uart_send_diag(PSOC_EVT_DUMP_START, diag_u16_sat(g_batches_captured));
        }
        uart_send_capture_batch(g_batches_sent);
        g_batches_sent++;
        if (!capture_dump_pending()) {
            uart_send_diag(PSOC_EVT_DUMP_DONE, diag_u16_sat(g_batches_sent));
        }
        return;
    }

    if (watchdog_rx) { watchdog_rx = 0u; rx_state = 0u; }

    uart_service();
}

static void service_comm_led(uint32 now)
{
    static uint32 lastTick   = 0u;
    static uint32 blinkTicks = 0u;
    uint32 elapsed;

    if (g_state == PSOC_SAMPLING) {
        lastTick = now;
        return;
    }

    if (lastTick == 0u) { lastTick = now; return; }

    elapsed = now - lastTick;
    if (elapsed == 0u) { return; }
    lastTick = now;

    if (g_comm_countdown > elapsed) {
        g_comm_countdown -= elapsed;
    } else {
        g_comm_countdown = 0u;
    }

    if (g_comm_countdown > 0u) {
        blinkTicks += elapsed;
        if (blinkTicks >= COMM_BLINK_TICKS) {
            blinkTicks = 0u;
            led_toggle();
        }
    } else {
        led_write(1u);
        blinkTicks = 0u;
    }
}

static void wait_ticks(uint32 ticks)
{
    uint32 due = timer_now_ticks() + ticks;
    while (!ticks_due(timer_now_ticks(), due)) {
        (void)service_button_calibration();
        service_runtime();
    }
}

static void wait_for_esp(void)
{
    uint32 nextPing  = timer_now_ticks();
    uint32 pingStart = nextPing;

    led_write(1u);
    uart_send_diag(PSOC_EVT_WAIT_ESP, 0u);
    while (!g_esp_connected)
    {
        uint32 now;
        (void)service_button_calibration();
        service_runtime();
        now = timer_now_ticks();

        if (ticks_due(now, nextPing)) {
            uart_send_ping();
            pingStart = now;
            nextPing  = now + PING_PERIOD_TICKS;
            led_write(1u);
        }

        if ((now - pingStart) >= (PING_PERIOD_TICKS - PING_OFF_TICKS)) {
            led_write(0u);
        } else {
            led_write(1u);
        }
    }
}

/* -------------------------------------------------------------------------- */

#if PSOC_TX1_BITBANG_TEST
static void tx1_manual_write(uint8 value)
{
    reg8 *dr = (reg8 *)Tx_1__DR;
    uint8 cur = *dr;
    if (value) {
        cur |= Tx_1__MASK;
    } else {
        cur &= (uint8)~Tx_1__MASK;
    }
    *dr = cur;
}

static void tx1_bitbang_byte(uint8 value)
{
    uint8 i;

    tx1_manual_write(0u);
    CyDelayUs(9u);
    for (i = 0u; i < 8u; i++) {
        tx1_manual_write((value >> i) & 0x01u);
        CyDelayUs(9u);
    }
    tx1_manual_write(1u);
    CyDelayUs(9u);
}
#endif

#if PSOC_TX1_GPIO_TEST
static void tx1_gpio_detach_dsi(void)
{
    reg8 *outSel0 = (reg8 *)Tx_1__PRTDSI__OUT_SEL0;
    reg8 *outSel1 = (reg8 *)Tx_1__PRTDSI__OUT_SEL1;
    reg8 *oeSel0  = (reg8 *)Tx_1__PRTDSI__OE_SEL0;
    reg8 *oeSel1  = (reg8 *)Tx_1__PRTDSI__OE_SEL1;
    reg8 *byp     = (reg8 *)Tx_1__BYP;

    *outSel0 &= (uint8)~Tx_1__MASK;
    *outSel1 &= (uint8)~Tx_1__MASK;
    *oeSel0  &= (uint8)~Tx_1__MASK;
    *oeSel1  &= (uint8)~Tx_1__MASK;
    *byp     |= Tx_1__MASK;
    CyPins_SetPinDriveMode(Tx_1__0__PC, PIN_DM_STRONG);
}

static void tx1_gpio_write(uint8 value)
{
    reg8 *dr = (reg8 *)Tx_1__DR;
    uint8 cur = *dr;
    if (value) {
        cur |= Tx_1__MASK;
    } else {
        cur &= (uint8)~Tx_1__MASK;
    }
    *dr = cur;
}
#endif

/* Inicializa los cinco canales DMA del mux Single/Lote:
 *   DMA_DelSig_RAM_Single/Lote   ADC → RAM
 *   DMA_Filter_RAM_Single/Lote   Filter → RAM
 *   DMA_DelSig_Filter            ADC → Filter (siempre activo, sin ISR)
 * Todos los TD son circulares (nextTd == mismo TD): cada canal recicla solo.
 * Los cinco quedan habilitados aunque solo uno por par esté "vivo" — cuál
 * recibe el trigger real lo decide la lógica combinacional gobernada por
 * Reg_Select, no el software. */
static void dma_adc_init(void)
{
    uint8 td;

    /* ── Raw Single: ADC DelSig → g_dma_raw_buf (3 bytes/disparo) ────────── */
    DMA_DelSig_RAM_Single_DmaInitialize(3u, 1u,
        HI16((uint32)ADC_DEC_SAMP_PTR),
        HI16((uint32)(void *)g_dma_raw_buf));
    td = CyDmaTdAllocate();
    CyDmaTdSetConfiguration(td, 3u, td,
        DMA_DelSig_RAM_Single__TD_TERMOUT_EN | TD_INC_DST_ADR);
    CyDmaTdSetAddress(td,
        LO16((uint32)ADC_DEC_SAMP_PTR),
        LO16((uint32)(void *)g_dma_raw_buf));
    CyDmaChSetInitialTd(DMA_DelSig_RAM_Single_DmaHandle, td);
    CyDmaChEnable(DMA_DelSig_RAM_Single_DmaHandle, 1u);

    /* ── Raw Lote: ADC DelSig → g_dma_raw_lote_buf (lote de DMA_LOTE_SAMPLES) ── */
    DMA_DelSig_RAM_Lote_DmaInitialize(3u, 1u,
        HI16((uint32)ADC_DEC_SAMP_PTR),
        HI16((uint32)(void *)g_dma_raw_lote_buf));
    td = CyDmaTdAllocate();
    CyDmaTdSetConfiguration(td, (uint16)(3u * DMA_LOTE_SAMPLES), td,
        DMA_DelSig_RAM_Lote__TD_TERMOUT_EN | TD_INC_DST_ADR);
    CyDmaTdSetAddress(td,
        LO16((uint32)ADC_DEC_SAMP_PTR),
        LO16((uint32)(void *)g_dma_raw_lote_buf));
    CyDmaChSetInitialTd(DMA_DelSig_RAM_Lote_DmaHandle, td);
    CyDmaChEnable(DMA_DelSig_RAM_Lote_DmaHandle, 1u);

    /* ── Filter input: ADC DelSig → Filter_STAGEA (siempre activo, sin ISR) ── */
    DMA_DelSig_Filter_DmaInitialize(3u, 1u,
        HI16((uint32)ADC_DEC_SAMP_PTR),
        HI16((uint32)Filter_STAGEA_PTR));
    td = CyDmaTdAllocate();
    CyDmaTdSetConfiguration(td, 3u, td,
        DMA_DelSig_Filter__TD_TERMOUT_EN | TD_INC_DST_ADR);
    CyDmaTdSetAddress(td,
        LO16((uint32)ADC_DEC_SAMP_PTR),
        LO16((uint32)Filter_STAGEA_PTR));
    CyDmaChSetInitialTd(DMA_DelSig_Filter_DmaHandle, td);
    CyDmaChEnable(DMA_DelSig_Filter_DmaHandle, 1u);

    /* ── Filter Single: Filter_HOLDA → g_dma_filt_buf (3 bytes/disparo) ──── */
    DMA_Filter_RAM_Single_DmaInitialize(3u, 1u,
        HI16((uint32)Filter_HOLDA_PTR),
        HI16((uint32)(void *)g_dma_filt_buf));
    td = CyDmaTdAllocate();
    CyDmaTdSetConfiguration(td, 3u, td,
        DMA_Filter_RAM_Single__TD_TERMOUT_EN | TD_INC_SRC_ADR | TD_INC_DST_ADR);
    CyDmaTdSetAddress(td,
        LO16((uint32)Filter_HOLDA_PTR),
        LO16((uint32)(void *)g_dma_filt_buf));
    CyDmaChSetInitialTd(DMA_Filter_RAM_Single_DmaHandle, td);
    CyDmaChEnable(DMA_Filter_RAM_Single_DmaHandle, 1u);

    /* ── Filter Lote: Filter_HOLDA → g_dma_filt_lote_buf ─────────────────── */
    DMA_Filter_RAM_Lote_DmaInitialize(3u, 1u,
        HI16((uint32)Filter_HOLDA_PTR),
        HI16((uint32)(void *)g_dma_filt_lote_buf));
    td = CyDmaTdAllocate();
    CyDmaTdSetConfiguration(td, (uint16)(3u * DMA_LOTE_SAMPLES), td,
        DMA_Filter_RAM_Lote__TD_TERMOUT_EN | TD_INC_SRC_ADR | TD_INC_DST_ADR);
    CyDmaTdSetAddress(td,
        LO16((uint32)Filter_HOLDA_PTR),
        LO16((uint32)(void *)g_dma_filt_lote_buf));
    CyDmaChSetInitialTd(DMA_Filter_RAM_Lote_DmaHandle, td);
    CyDmaChEnable(DMA_Filter_RAM_Lote_DmaHandle, 1u);

    /* Ruta por defecto: raw + Single — igual al comportamiento previo al mux. */
    dma_route_select(0u, 0u, 0u);
}

int main(void)
{
    uint8 i;

    CyGlobalIntEnable;

#if PSOC_TX1_GPIO_TEST
    tx1_gpio_detach_dsi();
    tx1_gpio_write(1u);
    for (;;)
    {
        led_toggle();
        tx1_gpio_write(0u);
        CyDelay(50u);
        tx1_gpio_write(1u);
        CyDelay(50u);
    }
#endif

#if PSOC_TX1_BITBANG_TEST
    CyPins_SetPinDriveMode(Tx_1__0__PC, PIN_DM_STRONG);
    tx1_manual_write(1u);
    for (;;)
    {
        led_toggle();
        tx1_bitbang_byte(0xABu);
        tx1_bitbang_byte(0xC0u);
        tx1_bitbang_byte(0x00u);
        tx1_bitbang_byte(0xC0u);
        CyDelay(100u);
    }
#endif

    UART_Start();
    psoc_calibration_set_diag_hook(uart_send_diag);

#if PSOC_EARLY_UART_TEST
    for (;;)
    {
        led_toggle();
        uart_send_diag(PSOC_EVT_BOOT, PSOC_HW_CLASS);
        uart_send_ping();
        CyDelay(100u);
    }
#endif

    uart_send_diag(PSOC_EVT_BOOT, PSOC_HW_CLASS);

    EEPROM_Start();

    psoc_hw_start_analog(g_pga_code, g_pgavdac_code);
    psoc_calibration_start_references();

    /* Cargar config guardada en EEPROM (si CRC válido) para converger más rápido */
    {
        uint8 nv_dac[PSOC_NV_CAL_STAGES];
        if (psoc_nv_load(nv_dac, PSOC_NV_CAL_STAGES)) {
            psoc_calibration_seed_dac(nv_dac, PSOC_NV_CAL_STAGES);
        }
    }

    Filter_Start();
    dma_adc_init();

    psoc_prepare_capture_path();
    uart_send_diag(PSOC_EVT_ANALOG_READY, 0u);

    isr_DMA_DelSig_RAM_StartEx(isr_DMA_DelSig_RAM_Handler);
    isr_DMA_Filter_RAM_StartEx(isr_DMA_Filter_RAM_Handler);
    isr_Timer_StartEx(isr_Timer);
    isr_SyncIn_StartEx(isr_SyncIn);
    Clock_1_Start();
    isr_Button_ClearPending();
    isr_Button_StartEx(isr_Button_Handler);

    Timer_Stop();
    Timer_WritePeriod(TIMEOUT_COUNTS);
    Timer_Start();

    /* ── Loop de arranque: busca el ESP sin bloquear UART/ADC ───────────── */
    wait_for_esp();
    psoc_calibration_servo_enable(0u);

    /* ── 5 parpadeos rápidos al conectar ─────────────────────────────────── */
    for (i = 0u; i < 5u; i++)
    {
        led_write(0u); wait_ticks(CONNECT_BLINK_TICKS);
        led_write(1u); wait_ticks(CONNECT_BLINK_TICKS);
    }

    /* ── Loop principal ─────────────────────────────────────────────────── */
    for (;;)
    {
        static uint32 idlePingDue = 0u;
        uint32 now;

        if (service_button_calibration()) {
            continue;
        }
        service_runtime();

        if (g_state == PSOC_SAMPLING || capture_dump_pending()) {
            continue;   /* Sin LED ni pings durante captura y volcado */
        }
        now = timer_now_ticks();

        if (g_state == PSOC_IDLE && !capture_dump_pending()) {
            (void)psoc_calibration_servo_service();
        }

        if (g_state == PSOC_IDLE && !capture_dump_pending())
        {
            if (idlePingDue == 0u) {
                idlePingDue = now + IDLE_PING_TICKS;
            }
            if (ticks_due(now, idlePingDue))
            {
                idlePingDue = now + IDLE_PING_TICKS;
                uart_send_ping();
            }
        }
        else
        {
            idlePingDue = 0u;
        }

        service_comm_led(now);
    }
}

/* [] END OF FILE */
