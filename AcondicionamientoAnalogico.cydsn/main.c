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
*  HAMMER — componentes TopDesign (simplificado en "Precambios", ya sin
*  etapa de referencia de entrada separada):
*    UART, ADC, AMux_ADC, VDAC_PGA, VDAC_LP,
*    PGA, Opa_ref_PGA, Opa_LP, LPF_ADC, Timer, LED.
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
#include "DMA_DelSig_RAM_dma.h"
#include "DMA_Filter_RAM_dma.h"
#include "DMA_DelSig_Filter_dma.h"
#include "Filter.h"
#include "Filter_PVT.h"
#include "ctrl.h"
#include "cfg.h"
#include "reset.h"
#include "status.h"
#include "error.h"
#include "state.h"
#include "isr_SuperMaquina.h"
#include "isr_Timer.h"
#include "isr_Timer_1.h"
#include "isr_Timer_2.h"
#include "isr_Timer_3.h"
#include "tmr_event.h"
#include "filter_coeffs.h"
#include "FIR_adquisition.h"
#include "psoc_debug.h"
/* -------------------------------------------------------------------------- */
#define LEGACY_VDAC_SHADOW_INIT 0x9Cu

#if defined(CY_STATUS_REG_tmr_event_H)
#define timer_event_Read tmr_event_Read
#endif

#if defined(isr_Timer__INTC_MASK)
#define isr_Timer_SetPriority(priority)   (*(isr_Timer_INTC_PRIOR) = ((uint8)(priority) << 5))
#define isr_Timer_Disable()               (*(isr_Timer_INTC_CLR_EN) = isr_Timer__INTC_MASK)
#define isr_Timer_ClearPending()          (*(isr_Timer_INTC_CLR_PD) = isr_Timer__INTC_MASK)
#else
#define isr_Timer_SetPriority(priority)   ((void)(priority))
#define isr_Timer_Disable()               ((void)0)
#define isr_Timer_ClearPending()          ((void)0)
#endif

#if defined(isr_Timer_1__INTC_MASK)
#define isr_Timer_1_SetPriority(priority) (*(isr_Timer_1_INTC_PRIOR) = ((uint8)(priority) << 5))
#define isr_Timer_1_Disable()             (*(isr_Timer_1_INTC_CLR_EN) = isr_Timer_1__INTC_MASK)
#define isr_Timer_1_ClearPending()        (*(isr_Timer_1_INTC_CLR_PD) = isr_Timer_1__INTC_MASK)
#else
#define isr_Timer_1_SetPriority(priority) ((void)(priority))
#define isr_Timer_1_Disable()             ((void)0)
#define isr_Timer_1_ClearPending()        ((void)0)
#endif

#if defined(isr_Timer_2__INTC_MASK)
#define isr_Timer_2_SetPriority(priority) (*(isr_Timer_2_INTC_PRIOR) = ((uint8)(priority) << 5))
#define isr_Timer_2_Disable()             (*(isr_Timer_2_INTC_CLR_EN) = isr_Timer_2__INTC_MASK)
#define isr_Timer_2_ClearPending()        (*(isr_Timer_2_INTC_CLR_PD) = isr_Timer_2__INTC_MASK)
#else
#define isr_Timer_2_SetPriority(priority) ((void)(priority))
#define isr_Timer_2_Disable()             ((void)0)
#define isr_Timer_2_ClearPending()        ((void)0)
#endif

#if defined(isr_Timer_3__INTC_MASK)
#define isr_Timer_3_SetPriority(priority) (*(isr_Timer_3_INTC_PRIOR) = ((uint8)(priority) << 5))
#define isr_Timer_3_Disable()             (*(isr_Timer_3_INTC_CLR_EN) = isr_Timer_3__INTC_MASK)
#define isr_Timer_3_ClearPending()        (*(isr_Timer_3_INTC_CLR_PD) = isr_Timer_3__INTC_MASK)
#else
#define isr_Timer_3_SetPriority(priority) ((void)(priority))
#define isr_Timer_3_Disable()             ((void)0)
#define isr_Timer_3_ClearPending()        ((void)0)
#endif

#if defined(Tmr_UartRxWatchdog_STATUS_TC) && !defined(Timer_STATUS_TC)
#define Timer_Init               Tmr_UartRxWatchdog_Init
#define Timer_Stop               Tmr_UartRxWatchdog_Stop
#define Timer_Start              Tmr_UartRxWatchdog_Start
#define Timer_Enable             Tmr_UartRxWatchdog_Enable
#define Timer_WritePeriod        Tmr_UartRxWatchdog_WritePeriod
#define Timer_WriteCounter       Tmr_UartRxWatchdog_WriteCounter
#define Timer_ReadStatusRegister Tmr_UartRxWatchdog_ReadStatusRegister
#define Timer_STATUS_TC          Tmr_UartRxWatchdog_STATUS_TC
#define Timer_initVar            Tmr_UartRxWatchdog_initVar
#endif

#if defined(Tmr_PingCalTick_STATUS_TC) && !defined(Timer_1_STATUS_TC)
#define Timer_1_Init               Tmr_PingCalTick_Init
#define Timer_1_Stop               Tmr_PingCalTick_Stop
#define Timer_1_Start              Tmr_PingCalTick_Start
#define Timer_1_Enable             Tmr_PingCalTick_Enable
#define Timer_1_WritePeriod        Tmr_PingCalTick_WritePeriod
#define Timer_1_WriteCounter       Tmr_PingCalTick_WriteCounter
#define Timer_1_ReadStatusRegister Tmr_PingCalTick_ReadStatusRegister
#define Timer_1_STATUS_TC          Tmr_PingCalTick_STATUS_TC
#define Timer_1_initVar            Tmr_PingCalTick_initVar
#endif

#if defined(Tmr_LedWait_STATUS_TC) && !defined(Timer_2_STATUS_TC)
#define Timer_2_Init               Tmr_LedWait_Init
#define Timer_2_Stop               Tmr_LedWait_Stop
#define Timer_2_Start              Tmr_LedWait_Start
#define Timer_2_Enable             Tmr_LedWait_Enable
#define Timer_2_WritePeriod        Tmr_LedWait_WritePeriod
#define Timer_2_WriteCounter       Tmr_LedWait_WriteCounter
#define Timer_2_ReadStatusRegister Tmr_LedWait_ReadStatusRegister
#define Timer_2_STATUS_TC          Tmr_LedWait_STATUS_TC
#define Timer_2_initVar            Tmr_LedWait_initVar
#endif

#if defined(Tmr_CaptureWatchdog_STATUS_TC) && !defined(Timer_3_STATUS_TC)
#define Timer_3_Init               Tmr_CaptureWatchdog_Init
#define Timer_3_Stop               Tmr_CaptureWatchdog_Stop
#define Timer_3_Start              Tmr_CaptureWatchdog_Start
#define Timer_3_Enable             Tmr_CaptureWatchdog_Enable
#define Timer_3_WritePeriod        Tmr_CaptureWatchdog_WritePeriod
#define Timer_3_WriteCounter       Tmr_CaptureWatchdog_WriteCounter
#define Timer_3_ReadStatusRegister Tmr_CaptureWatchdog_ReadStatusRegister
#define Timer_3_STATUS_TC          Tmr_CaptureWatchdog_STATUS_TC
#define Timer_3_initVar            Tmr_CaptureWatchdog_initVar
#endif

/* FILTER_FIR_NTAPS vive en filter_coeffs.h (unica fuente de verdad,
 * compartida con calibration.c). El descarte del retardo de grupo (63
 * muestras) lo hace el ARM con g_fir_discard. superMaquina mantiene el FIR
 * alimentado en ARMED y el objetivo de hardware se extiende para no perder
 * lotes utiles al final. */
#define FILTER_GROUP_DELAY ((FILTER_FIR_NTAPS - 1u) / 2u)   /* 63 muestras */

#ifndef PSOC_LOAD_NV_CAL_ON_BOOT
#define PSOC_LOAD_NV_CAL_ON_BOOT 1u
#endif
#define TIMER_ILO_COUNTS_PER_MS 100u     /* Timers fixed @ 100 kHz. */
#define TIMER_MAX_COUNTS        60000UL  /* margen bajo 0xFFFF para rearmar FF one-shot */
#define UART_RX_TIMEOUT_MS      10u
#define TIMEOUT_COUNTS          (UART_RX_TIMEOUT_MS * TIMER_ILO_COUNTS_PER_MS)

#define BATCH_SAMPLES      30u
#define FRAME_BYTES        (4u + BATCH_SAMPLES * 3u + 1u)   /* 95 */
#define SAMPLE_BYTES       3u
#define CAPTURE_BATCH_BYTES (BATCH_SAMPLES * SAMPLE_BYTES)

#ifndef PSOC_CAPTURE_MAX_BATCHES
#define PSOC_CAPTURE_MAX_BATCHES 512u
#endif

#define PING_PERIOD_MS     700u
#define PING_OFF_MS        100u
#define IDLE_PING_MS       1000u
#define COMM_BLINK_MS      200u  /* medio-período del parpadeo del LED (~2.5 Hz, visible) */
#define CONNECT_BLINK_MS   80u

/* Titilar LED (identificación de nodo): parpadeo NO bloqueante servido por
 * Timer_2. Al recibir PSOC_CMD_BLINK_LED se arma una cadena de one-shots; el
 * LED titila y luego vuelve a su reposo (encendido fijo). */
#define IDENTIFY_BLINK_WINDOW_MS  8000u
#define IDENTIFY_BLINK_TOGGLES    ((IDENTIFY_BLINK_WINDOW_MS + COMM_BLINK_MS - 1u) / COMM_BLINK_MS)

#ifndef PSOC_DIAG_ENABLE
#define PSOC_DIAG_ENABLE      1
#endif

#ifndef PSOC_RAMP_DEBUG_ENABLE
#define PSOC_RAMP_DEBUG_ENABLE 1
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

#ifndef PSOC_BUTTON_CAL_ENABLE
#define PSOC_BUTTON_CAL_ENABLE 1
#endif

/* -------------------------------------------------------------------------- */
/* Buffers destino DMA — 3 bytes LE (24 bits signed) por muestra */
static volatile uint8  g_dma_raw_buf[3]   = {0u, 0u, 0u};
static volatile uint8  g_dma_filt_buf[3]  = {0u, 0u, 0u};

/* 0 = enviar crudo (default); 1 = enviar filtrado por FIR hardware */
static volatile uint8  g_stream_mode      = 0u;

/* Buffer plano de captura: N lotes x 30 muestras x 3 bytes. El hot path de la
 * ISR solo copia 3 bytes al puntero de escritura; el alineado de counts se
 * difiere al volcado UART (uart_send_capture_batch). */
static volatile uint8  g_capture_raw[(uint32)PSOC_CAPTURE_MAX_BATCHES * CAPTURE_BATCH_BYTES];
static volatile uint8 * volatile g_capture_wr  = g_capture_raw;
static volatile uint8 * volatile g_capture_end = g_capture_raw;
/* 1 = las muestras guardadas necesitan alineado (division DEC_DIV) al volcar;
 * 0 = rampa debug u otros datos sinteticos que van tal cual. */
static volatile uint8  g_capture_align    = 1u;
static volatile uint8  g_capture_adc_raw  = 1u;
static volatile uint16 g_fir_discard      = 0u;
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
#if PSOC_BUTTON_CAL_ENABLE
static volatile uint8  g_cal_button_pressed = 0u;
#endif

static volatile uint8  rx_state       = 0u;
static volatile uint8  rx_cmd         = 0u;
static volatile uint8  rx_p1          = 0u;
static volatile uint8  rx_p2          = 0u;
static volatile uint8  watchdog_rx    = 0u;

static          uint8  g_esp_connected  = 0u;
static          uint8  g_nv_ready       = 0u;
static          uint8  g_last_calibration_ok = 0u;

/* Mapa de timers fixed (los 4 con rol propio; ver
 * docs/psoc_supermaquina_handoff.md):
 *   Timer   : watchdog del parser UART RX (concurrente con todo lo demas).
 *   Timer_1 : ping boot/idle + tick de calibracion (nunca concurrentes:
 *             la calibracion detiene los pings antes de arrancar).
 *   Timer_2 : LED/esperas cortas, sin multiplexar con nada mas.
 *   Timer_3 : watchdog de captura dedicado. Antes compartia Timer_2 por modo;
 *             dedicarlo elimina el riesgo de que un blink/espera pise el
 *             watchdog (o al reves) si algun dia coinciden. Si se quisiera
 *             reducir a 3 timers, este es el que se pliega de vuelta sobre
 *             Timer_2 (modo CAPTURE_WD) y se elimina del TopDesign. */
#define TIMER1_MODE_NONE       0u
#define TIMER1_MODE_BOOT_PING  1u
#define TIMER1_MODE_IDLE_PING  2u
#define TIMER1_MODE_CAL_TICK   3u

#define TIMER2_MODE_NONE          0u
#define TIMER2_MODE_BOOT_LED_OFF  1u
#define TIMER2_MODE_WAIT          2u
#define TIMER2_MODE_COMM_BLINK    3u

static volatile uint32 g_timer1_remaining_counts = 0u;
static volatile uint8  g_timer1_mode             = TIMER1_MODE_NONE;
static volatile uint8  g_boot_ping_due           = 0u;
static volatile uint8  g_idle_ping_due           = 0u;
static volatile uint8  g_idle_ping_armed         = 0u;

static volatile uint32 g_timer2_remaining_counts = 0u;
static volatile uint8  g_timer2_mode             = TIMER2_MODE_NONE;
static volatile uint8  g_boot_led_off_due        = 0u;
static volatile uint8  g_timer2_wait_done        = 0u;
static volatile uint8  g_comm_blink_due          = 0u;
static volatile uint16 g_comm_blinks_remaining   = 0u;
static volatile uint8  g_comm_led_active         = 0u;

/* Timer_3 es monoproposito (watchdog de captura): no necesita variable de
 * modo, solo el resto de cuenta para el troceado a 60000 counts. */
static volatile uint32 g_timer3_remaining_counts = 0u;
static volatile uint8  g_capture_wd_due          = 0u;
static volatile uint8  g_timer_event_pending     = 0u;

/* ── Circuit-breaker de tormenta de IRQ de timers fixed ──────────────────────
 * Visto en hardware (pyocd): la IRQ de un timer FF puede quedar re-disparando
 * para siempre (VECTACTIVE fijo, ISPR=0, SR0=0, timer parado) y mata el
 * sistema entero; leer el status no la corta, solo deshabilitar la IRQ en el
 * NVIC. Cada ISR cuenta entradas espurias (status==0) consecutivas y al
 * llegar a TIMER_STORM_TRIP se auto-deshabilita y marca su bit; el thread
 * (timer_storm_recover) re-inicializa el bloque, emite diagnostico y restaura
 * el estado del modo que estaba en vuelo. */
#define TIMER_STORM_TRIP 4u
static volatile uint8  g_timer_storm             = 0u;  /* bit0..3 = Timer..Timer_3 */
static volatile uint8  g_t0_spurious             = 0u;
static volatile uint8  g_t1_spurious             = 0u;
static volatile uint8  g_t2_spurious             = 0u;
static volatile uint8  g_t3_spurious             = 0u;

static volatile uint8  g_cal_timer_active        = 0u;
static volatile uint8  g_cal_progress_due        = 0u;
static volatile uint8  g_cal_watchdog_due        = 0u;
static volatile uint32 g_cal_progress_period_ms  = 0u;
static volatile uint32 g_cal_watchdog_remaining_ms = 0u;

/* Margen extra del watchdog de captura sobre la duracion nominal esperada. */
#define CAPTURE_WD_MARGIN_MS  1500u

/* superMaquina control/status map. */
#define CE_CTRL_ARM          0x01u
#define CE_CTRL_START_NOW    0x02u
#define CE_CTRL_STOP         0x04u
#define CE_CTRL_CLEAR_FLAGS  0x08u
#define CE_CTRL_USE_SYNC     0x10u
#define CE_CTRL_LOAD_BATCH_LO 0x20u
#define CE_CTRL_ENGINE_EN    0x40u
#define CE_CTRL_LOAD_BATCH_HI 0x80u

#define CE_CFG_SRC_MASK      0x03u
#define CE_CFG_SRC_RAW       0x00u
#define CE_CFG_SRC_FILTER    0x01u
#define CE_CFG_SRC_COMBINED  0x02u
#define CE_CFG_SRC_DEBUG     0x03u
#define CE_CFG_IRQ_BATCH_EN  0x04u
#define CE_CFG_IRQ_SYNC_EN   0x08u
#define CE_CFG_IRQ_ERROR_EN  0x10u
/* cfg[5..7] libres. El descarte FIR queda en C: ponerlo en PLD/count7
 * compila, pero en placa dejo el camino filtrado por sync retenido. */

#define CE_STATUS_IDLE       0x01u
#define CE_STATUS_ARMED      0x02u
#define CE_STATUS_SAMPLING   0x04u
#define CE_STATUS_DONE       0x08u
#define CE_STATUS_ERROR      0x10u
#define CE_STATUS_BUTTON     0x80u

#define CE_TIMER_EVT_UART       0x01u
#define CE_TIMER_EVT_PING       0x02u
#define CE_TIMER_EVT_LED        0x04u
#define CE_TIMER_EVT_CAPTURE_WD 0x08u
#define CE_TIMER_EVT_MASK       0x0Fu

#ifndef PSOC_SUPERMAQUINA_OWNS_DMA_IRQ
#define PSOC_SUPERMAQUINA_OWNS_DMA_IRQ 1u
#endif

#ifndef PSOC_SUPERMAQUINA_OWNS_SYNC_IRQ
#define PSOC_SUPERMAQUINA_OWNS_SYNC_IRQ 1u
#endif

#define CE_STATE_IDLE        0u
#define CE_STATE_ARMED       1u
#define CE_STATE_SAMPLING    2u
#define CE_STATE_DONE        3u
#define CE_STATE_ERROR       4u

static          uint8 g_ce_ctrl_level = 0u;
static          uint8 g_ce_cfg_level  = (CE_CFG_SRC_RAW | CE_CFG_IRQ_SYNC_EN | CE_CFG_IRQ_ERROR_EN);
static volatile uint8 g_ce_status     = CE_STATUS_IDLE;
static volatile uint8 g_ce_error      = 0u;
static volatile uint8 g_ce_state      = CE_STATE_IDLE;

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

static void uart_wait_tx_complete_quiet(void)
{
    uint16 guard = 60000u;
    while (guard-- != 0u) {
#if (UART_TX_INTERRUPT_ENABLED && UART_TX_ENABLED)
        if (UART_GetTxBufferSize() != 0u) {
            continue;
        }
#endif
        if ((UART_ReadTxStatus() & UART_TX_STS_COMPLETE) != 0u) {
            break;
        }
    }
}

static void uart_send_fs_report(void)
{
    uint16 fs = psoc_adc_effective_fs_hz();
    uint8 fs_lo = (uint8)(fs & 0xFFu);
    uint8 fs_hi = (uint8)((fs >> 8u) & 0xFFu);
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
    psoc_debug_print_evt(event, value, state);
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

static void capture_engine_refresh_status(void)
{
    g_ce_status = status_Read();
    g_ce_error  = error_Read();
    g_ce_state  = state_Read();
}

static void capture_engine_write_ctrl(void)
{
    ctrl_Write(g_ce_ctrl_level);
}

static void capture_engine_pulse(uint8 pulse)
{
    ctrl_Write((uint8)(g_ce_ctrl_level | pulse));
    ctrl_Write(g_ce_ctrl_level);
}

static void capture_engine_set_source(uint8 source)
{
    g_ce_cfg_level = (uint8)((g_ce_cfg_level &
                              (uint8)~CE_CFG_SRC_MASK) |
                             (source & CE_CFG_SRC_MASK) |
                             CE_CFG_IRQ_SYNC_EN |
                             CE_CFG_IRQ_ERROR_EN);
    cfg_Write(g_ce_cfg_level);
}

static void capture_engine_set_enabled(uint8 enabled, uint8 use_sync)
{
    if (enabled) {
        g_ce_ctrl_level |= CE_CTRL_ENGINE_EN;
    } else {
        g_ce_ctrl_level &= (uint8)~CE_CTRL_ENGINE_EN;
    }

    if (use_sync) {
        g_ce_ctrl_level |= CE_CTRL_USE_SYNC;
    } else {
        g_ce_ctrl_level &= (uint8)~CE_CTRL_USE_SYNC;
    }

    capture_engine_write_ctrl();
}

static void capture_engine_reset_hw(void)
{
    reset_Write(1u);
    reset_Write(0u);
    g_timer_event_pending = 0u;
    capture_engine_refresh_status();
}

static void capture_engine_clear_flags(void)
{
    capture_engine_pulse(CE_CTRL_CLEAR_FLAGS);
    capture_engine_refresh_status();
}

static void timer_events_accumulate(uint8 events)
{
    uint8 saved;
    events &= CE_TIMER_EVT_MASK;
    if (events == 0u) {
        return;
    }
    saved = CyEnterCriticalSection();
    g_timer_event_pending |= events;
    CyExitCriticalSection(saved);
}

/* Toma solo los eventos incluidos en mask; el resto queda holdeado en
 * g_timer_event_pending hasta que un estado tranquilo los consuma. */
static uint8 timer_events_take_pending(uint8 mask)
{
    uint8 events;
    uint8 saved = CyEnterCriticalSection();
    events = (uint8)(g_timer_event_pending & mask & CE_TIMER_EVT_MASK);
    g_timer_event_pending &= (uint8)~events;
    CyExitCriticalSection(saved);
    return events;
}

/* Los TC ya no generan IRQ (política determinismo-primero): cada TC queda
 * retenido en el STATUS_TC de su timer fixed hasta que el lazo principal lo
 * lea acá. Durante ARMED/SAMPLING solo se sondea el watchdog de captura. */
static uint8 capture_engine_poll_capture_wd(void)
{
    uint8 events = 0u;

    if ((Timer_3_ReadStatusRegister() & Timer_3_STATUS_TC) != 0u) {
        Timer_3_Stop();
        events |= CE_TIMER_EVT_CAPTURE_WD;
    }
    if (events != 0u) {
        timer_events_accumulate(events);
    }
    return events;
}

static uint8 capture_engine_poll_timer_events(void)
{
    uint8 events = 0u;

    if ((Timer_ReadStatusRegister() & Timer_STATUS_TC) != 0u) {
        Timer_Stop();
        events |= CE_TIMER_EVT_UART;
    }
    if ((Timer_1_ReadStatusRegister() & Timer_1_STATUS_TC) != 0u) {
        Timer_1_Stop();
        events |= CE_TIMER_EVT_PING;
    }
    if ((Timer_2_ReadStatusRegister() & Timer_2_STATUS_TC) != 0u) {
        Timer_2_Stop();
        events |= CE_TIMER_EVT_LED;
    }
    if (events != 0u) {
        timer_events_accumulate(events);
    }
    events |= capture_engine_poll_capture_wd();
    return events;
}

/* Carga el objetivo de lotes en superMaquina. Para FIR armado el caller pasa
 * un objetivo extendido: el ARM descarta FILTER_GROUP_DELAY muestras y aun asi
 * quedan los lotes pedidos en el buffer final. */
static void capture_engine_configure_target(uint16 hw_target)
{
    uint16 limit = (uint16)(hw_target - 1u);
    uint8 saved_cfg = g_ce_cfg_level;

    cfg_Write((uint8)(limit & 0xFFu));
    capture_engine_pulse(CE_CTRL_LOAD_BATCH_LO);
    cfg_Write((uint8)((limit >> 8u) & 0x01u));
    capture_engine_pulse(CE_CTRL_LOAD_BATCH_HI);
    cfg_Write(saved_cfg);
}

static uint8 capture_engine_source_from_stream(uint8 use_filter)
{
#if PSOC_RAMP_DEBUG_ENABLE
    if (g_debug_psoc) {
        return CE_CFG_SRC_DEBUG;
    }
#endif
    return (use_filter != 0u) ? CE_CFG_SRC_FILTER : CE_CFG_SRC_RAW;
}

static void psoc_prepare_capture_path(void)
{
    psoc_calibration_servo_abort();
    ADC_StopConvert();
    psoc_adc_select_capture_config();
    psoc_calibration_restore_capture_path();
}

static uint32 timer_ms_to_counts(uint32 ms)
{
    uint32 counts = ms * (uint32)TIMER_ILO_COUNTS_PER_MS;
    return (counts == 0u) ? 1u : counts;
}

static uint16 timer_take_chunk(volatile uint32 *remaining_counts)
{
    uint32 remaining = *remaining_counts;
    uint16 chunk;

    if (remaining == 0u) {
        return 0u;
    }
    if (remaining > TIMER_MAX_COUNTS) {
        chunk = (uint16)TIMER_MAX_COUNTS;
        *remaining_counts = remaining - TIMER_MAX_COUNTS;
    } else {
        chunk = (uint16)remaining;
        *remaining_counts = 0u;
    }
    return chunk;
}

static void timer1_start_next_chunk(void)
{
    uint16 chunk = timer_take_chunk(&g_timer1_remaining_counts);
    if (chunk == 0u) { return; }
    Timer_1_Stop();
    Timer_1_WritePeriod(chunk);
    Timer_1_WriteCounter(chunk);
    (void)Timer_1_ReadStatusRegister();
    isr_Timer_1_ClearPending();
    Timer_1_Start();
}

static void timer1_arm_ms(uint32 ms, uint8 mode)
{
    uint8 saved = CyEnterCriticalSection();
    Timer_1_Stop();
    g_timer1_mode = mode;
    g_timer1_remaining_counts = timer_ms_to_counts(ms);
    timer1_start_next_chunk();
    CyExitCriticalSection(saved);
}

static void timer1_stop_quiet(void)
{
    uint8 saved = CyEnterCriticalSection();
    Timer_1_Stop();
    (void)Timer_1_ReadStatusRegister();
    isr_Timer_1_ClearPending();
    g_timer1_remaining_counts = 0u;
    g_timer1_mode = TIMER1_MODE_NONE;
    CyExitCriticalSection(saved);
}

static void timer2_start_next_chunk(void)
{
    uint16 chunk = timer_take_chunk(&g_timer2_remaining_counts);
    if (chunk == 0u) { return; }
    Timer_2_Stop();
    Timer_2_WritePeriod(chunk);
    Timer_2_WriteCounter(chunk);
    (void)Timer_2_ReadStatusRegister();
    isr_Timer_2_ClearPending();
    Timer_2_Start();
}

static void timer2_arm_ms(uint32 ms, uint8 mode)
{
    uint8 saved = CyEnterCriticalSection();
    Timer_2_Stop();
    g_timer2_mode = mode;
    g_timer2_remaining_counts = timer_ms_to_counts(ms);
    timer2_start_next_chunk();
    CyExitCriticalSection(saved);
}

static void timer2_stop_quiet(void)
{
    uint8 saved = CyEnterCriticalSection();
    Timer_2_Stop();
    (void)Timer_2_ReadStatusRegister();
    isr_Timer_2_ClearPending();
    g_timer2_remaining_counts = 0u;
    g_timer2_mode = TIMER2_MODE_NONE;
    CyExitCriticalSection(saved);
}

static void timer3_start_next_chunk(void)
{
    uint16 chunk = timer_take_chunk(&g_timer3_remaining_counts);
    if (chunk == 0u) { return; }
    Timer_3_Stop();
    Timer_3_WritePeriod(chunk);
    Timer_3_WriteCounter(chunk);
    (void)Timer_3_ReadStatusRegister();
    isr_Timer_3_ClearPending();
    Timer_3_Start();
}

static void timer3_arm_ms(uint32 ms)
{
    uint8 saved = CyEnterCriticalSection();
    Timer_3_Stop();
    g_timer3_remaining_counts = timer_ms_to_counts(ms);
    timer3_start_next_chunk();
    CyExitCriticalSection(saved);
}

static void timer3_stop_quiet(void)
{
    uint8 saved = CyEnterCriticalSection();
    Timer_3_Stop();
    (void)Timer_3_ReadStatusRegister();
    isr_Timer_3_ClearPending();
    g_timer3_remaining_counts = 0u;
    CyExitCriticalSection(saved);
}

static void fixed_timers_init(void)
{
    Timer_Init();   Timer_initVar = 1u;   Timer_Stop();   (void)Timer_ReadStatusRegister();
    Timer_1_Init(); Timer_1_initVar = 1u; Timer_1_Stop(); (void)Timer_1_ReadStatusRegister();
    Timer_2_Init(); Timer_2_initVar = 1u; Timer_2_Stop(); (void)Timer_2_ReadStatusRegister();
    Timer_3_Init(); Timer_3_initVar = 1u; Timer_3_Stop(); (void)Timer_3_ReadStatusRegister();
    isr_Timer_SetPriority(7u);
    isr_Timer_1_SetPriority(7u);
    isr_Timer_2_SetPriority(7u);
    isr_Timer_3_SetPriority(7u);
    isr_Timer_Disable();
    isr_Timer_1_Disable();
    isr_Timer_2_Disable();
    isr_Timer_3_Disable();
    isr_Timer_ClearPending();
    isr_Timer_1_ClearPending();
    isr_Timer_2_ClearPending();
    isr_Timer_3_ClearPending();
}

static void rx_watchdog_start(void)
{
    uint8 saved = CyEnterCriticalSection();
    watchdog_rx = 0u;
    Timer_Stop();
    Timer_WritePeriod((uint16)TIMEOUT_COUNTS);
    Timer_WriteCounter((uint16)TIMEOUT_COUNTS);
    (void)Timer_ReadStatusRegister();
    isr_Timer_ClearPending();
    Timer_Start();
    CyExitCriticalSection(saved);
}

static void rx_watchdog_stop(void)
{
    uint8 saved = CyEnterCriticalSection();
    Timer_Stop();
    (void)Timer_ReadStatusRegister();
    isr_Timer_ClearPending();
    watchdog_rx = 0u;
    CyExitCriticalSection(saved);
}

static void idle_ping_schedule(void)
{
    g_idle_ping_due = 0u;
    g_idle_ping_armed = 1u;
    timer1_arm_ms(IDLE_PING_MS, TIMER1_MODE_IDLE_PING);
}

static void idle_ping_stop(void)
{
    if (g_timer1_mode == TIMER1_MODE_IDLE_PING) {
        timer1_stop_quiet();
    }
    g_idle_ping_due = 0u;
    g_idle_ping_armed = 0u;
}

static void comm_led_stop(void)
{
    if (g_timer2_mode == TIMER2_MODE_COMM_BLINK) {
        timer2_stop_quiet();
    }
    g_comm_led_active = 0u;
    g_comm_blink_due = 0u;
    g_comm_blinks_remaining = 0u;
}

/* ── Watchdog de captura (Timer_3, dedicado) ──
 * Si superMaquina nunca llega a DONE (p.ej. ADC detenido, DMA sin disparos),
 * el equipo antes quedaba mudo para siempre. Ahora el ARM se despierta al
 * vencer la duracion nominal + margen y aborta con diagnostico. Al tener
 * timer propio no compite con LED/esperas de Timer_2. */
static uint32 capture_expected_ms(uint16 batches)
{
    /* 30 muestras/lote a 1020 S/s ≈ 29.4 ms por lote (512 lotes ≈ 15.06 s). */
    return ((uint32)batches * (uint32)BATCH_SAMPLES * 1000u) /
           (uint32)psoc_adc_effective_fs_hz();
}

static void capture_watchdog_arm(void)
{
    uint32 ms = capture_expected_ms(capture_target_batches());
    g_capture_wd_due = 0u;
    timer3_arm_ms(ms + (ms >> 1u) + CAPTURE_WD_MARGIN_MS);
}

static void capture_watchdog_stop(void)
{
    timer3_stop_quiet();
    g_capture_wd_due = 0u;
}

static void runtime_timers_stop_for_quiet_window(void)
{
    rx_watchdog_stop();
    idle_ping_stop();
    comm_led_stop();
}

/* Tick de calibracion sobre Timer_1: progreso periodico + watchdog global.
 * Nunca concurre con los pings (los callers detienen idle/boot ping antes). */
void psoc_cal_timer_start(uint32 progress_ms, uint32 watchdog_ms)
{
    uint8 saved = CyEnterCriticalSection();
    if (progress_ms == 0u) { progress_ms = 1u; }
    if (watchdog_ms == 0u) { watchdog_ms = progress_ms; }
    g_cal_progress_due = 0u;
    g_cal_watchdog_due = 0u;
    g_cal_progress_period_ms = progress_ms;
    g_cal_watchdog_remaining_ms = watchdog_ms;
    g_cal_timer_active = 1u;
    g_idle_ping_due = 0u;
    g_idle_ping_armed = 0u;
    g_boot_ping_due = 0u;
    timer1_arm_ms(progress_ms, TIMER1_MODE_CAL_TICK);
    CyExitCriticalSection(saved);
}

void psoc_cal_timer_stop(void)
{
    uint8 saved = CyEnterCriticalSection();
    g_cal_timer_active = 0u;
    g_cal_progress_due = 0u;
    g_cal_watchdog_due = 0u;
    g_cal_progress_period_ms = 0u;
    g_cal_watchdog_remaining_ms = 0u;
    if (g_timer1_mode == TIMER1_MODE_CAL_TICK) {
        timer1_stop_quiet();
    }
    CyExitCriticalSection(saved);
}

uint8 psoc_cal_timer_take_progress_due(void)
{
    uint8 due;
    uint8 saved = CyEnterCriticalSection();
    due = g_cal_progress_due;
    g_cal_progress_due = 0u;
    CyExitCriticalSection(saved);
    return due;
}

uint8 psoc_cal_timer_take_watchdog_due(void)
{
    uint8 due;
    uint8 saved = CyEnterCriticalSection();
    due = g_cal_watchdog_due;
    g_cal_watchdog_due = 0u;
    CyExitCriticalSection(saved);
    return due;
}

static void capture_reset_locked(uint16 stored_batches)
{
    g_batches_sent     = 0u;
    g_batches_captured = 0u;
    g_capture_done     = 0u;
    g_capture_wd_due   = 0u;
    g_dbg_cnt          = 0u;
    g_seq              = 0u;
    g_fir_discard      = 0u;
    g_capture_wr       = g_capture_raw;
    g_capture_end      = g_capture_raw + ((uint32)stored_batches * CAPTURE_BATCH_BYTES);
}

static int32 sign_extend_bits(uint32 value, uint8 bits)
{
    uint32 sign;
    uint32 mask;

    if (bits == 0u || bits >= 32u) {
        return (int32)value;
    }
    sign = (uint32)1u << (bits - 1u);
    mask = (sign << 1u) - 1u;
    value &= mask;
    return (value & sign) ? (int32)(value | ~mask) : (int32)value;
}

/* Reconstruye int32 signed a partir de una palabra DMA LE de 24 bits. */
static int32 dma_buf_to_i24(const volatile uint8 *buf)
{
    uint32 u = (uint32)buf[0] | ((uint32)buf[1] << 8u) | ((uint32)buf[2] << 16u);
    return sign_extend_bits(u, 24u);
}

/* El ADC raw puede venir right-aligned (configs 2/3/4: CF_0V512, CF_1V024,
 * CF_0V625), donde el bit de signo real es resolution-1. Solo CF_2V5 es
 * left-aligned (Bit-23 OVF Protected) y ya viene sign-extendido en 24 bits.
 * La salida del DFB sigue usando dma_buf_to_i24(). */
static int32 dma_buf_to_adc_counts(const volatile uint8 *buf)
{
    uint32 u = (uint32)buf[0] | ((uint32)buf[1] << 8u) | ((uint32)buf[2] << 16u);

#if (ADC_DEFAULT_NUM_CONFIGS > 1)
#if (ADC_CF_0V512_ALIGNMENT == 0u)
    if (ADC_Config != ADC_CF_2V5) {
        return sign_extend_bits(u, (uint8)ADC_CF_0V512_RESOLUTION);
    }
#endif
#endif

    return sign_extend_bits(u, 24u);
}

/* ── Handlers de muestra de superMaquina ─────────────────────────────────────
 * El IRQ único de superMaquina llega una vez por muestra aceptada. Para no
 * decidir el modo en cada interrupción, el trabajo por muestra se resuelve
 * con un puntero a función que se fija al cambiar de modo:
 *   captura raw/filtrada : copiar 3 bytes crudos al puntero de escritura
 *                          (el alineado se difiere al volcado UART).
 *   captura debug        : rampa sintética paceada por el ADC real.
 *   bypass calibración   : alinear y publicar la muestra para calibration.c.
 *   armed-wait           : primer IRQ tras el sync — bookkeeping de la
 *                          transición ARMED→SAMPLING y cambio al handler de
 *                          captura elegido al armar.
 */
typedef void (*sm_sample_handler_t)(void);

static void sm_sample_noop(void)
{
}

static void sm_sample_capture_raw(void)
{
    volatile uint8 *dst = g_capture_wr;
    if (dst >= g_capture_end) { return; }
    dst[0] = g_dma_raw_buf[0];
    dst[1] = g_dma_raw_buf[1];
    dst[2] = g_dma_raw_buf[2];
    dst += SAMPLE_BYTES;
    g_capture_wr = dst;
    if (dst >= g_capture_end) { g_capture_done = 1u; }
}

static void sm_sample_capture_filt(void)
{
    volatile uint8 *dst = g_capture_wr;
    if (g_fir_discard != 0u) {
        g_fir_discard--;
        return;
    }
    if (dst >= g_capture_end) { return; }
    dst[0] = g_dma_filt_buf[0];
    dst[1] = g_dma_filt_buf[1];
    dst[2] = g_dma_filt_buf[2];
    dst += SAMPLE_BYTES;
    g_capture_wr = dst;
    if (dst >= g_capture_end) { g_capture_done = 1u; }
}

#if PSOC_RAMP_DEBUG_ENABLE
static void sm_sample_capture_debug(void)
{
    volatile uint8 *dst = g_capture_wr;
    uint32 val;
    if (dst >= g_capture_end) { return; }
    val = g_dbg_cnt++ & 0x00FFFFFFu;
    dst[0] = (uint8)( val         & 0xFFu);
    dst[1] = (uint8)((val >>  8u) & 0xFFu);
    dst[2] = (uint8)((val >> 16u) & 0xFFu);
    dst += SAMPLE_BYTES;
    g_capture_wr = dst;
    if (dst >= g_capture_end) { g_capture_done = 1u; }
}
#endif

static void sm_sample_cal_raw(void)
{
    psoc_adc_note_isr_sample(
        psoc_adc_counts_right_aligned(dma_buf_to_adc_counts(g_dma_raw_buf)));
}

static void sm_sample_cal_filt(void)
{
    psoc_adc_note_isr_filtered_sample(
        psoc_adc_counts_right_aligned(dma_buf_to_i24(g_dma_filt_buf)));
}

static volatile sm_sample_handler_t g_sm_sample_handler = sm_sample_noop;
/* Handler de captura elegido al armar; sm_sample_armed_wait lo instala al
 * detectar la transición ARMED→SAMPLING. */
static volatile sm_sample_handler_t g_sm_capture_handler = sm_sample_noop;

static sm_sample_handler_t capture_handler_for_source(uint8 source)
{
#if PSOC_RAMP_DEBUG_ENABLE
    if (source == CE_CFG_SRC_DEBUG) { return sm_sample_capture_debug; }
#endif
    if (source == CE_CFG_SRC_FILTER || source == CE_CFG_SRC_COMBINED) {
        return sm_sample_capture_filt;
    }
    return sm_sample_capture_raw;
}

static void sm_sample_armed_wait(void)
{
    /* Primer IRQ tras el sync (IRQ_SYNC_EN): superMaquina ya está muestreando.
     * No hay muestra que copiar todavía; solo bookkeeping de la ventana. */
    capture_watchdog_arm();
    g_state = PSOC_SAMPLING;
    g_sm_sample_handler = g_sm_capture_handler;
}

static void psoc_enter_sampling(uint8 debugMode)
{
    uint8 saved;
    uint8 source;
    uint16 target = capture_target_batches();
    ADC_StopConvert();
    runtime_timers_stop_for_quiet_window();
    saved = CyEnterCriticalSection();
#if PSOC_RAMP_DEBUG_ENABLE
    g_debug_psoc = debugMode ? 1u : 0u;
#else
    (void)debugMode;
    g_debug_psoc = 0u;
#endif
    capture_reset_locked(target);
    g_state = PSOC_SAMPLING;
    CyExitCriticalSection(saved);
    source = capture_engine_source_from_stream(g_stream_mode);
#if PSOC_RAMP_DEBUG_ENABLE
    g_capture_align = (source == CE_CFG_SRC_DEBUG) ? 0u : 1u;
#else
    g_capture_align = 1u;
#endif
    g_capture_adc_raw = (source == CE_CFG_SRC_RAW) ? 1u : 0u;
    capture_engine_configure_target(target);
    /* start-now: el FIR viene alimentado en bypass, historia caliente — sin
     * descarte de retardo de grupo. */
    capture_engine_set_source(source);
    g_sm_capture_handler = capture_handler_for_source(source);
    g_sm_sample_handler = g_sm_capture_handler;
    capture_engine_set_enabled(1u, 0u);
    capture_engine_clear_flags();
    capture_watchdog_arm();
    capture_engine_pulse(CE_CTRL_START_NOW);
    if (g_n_batches > PSOC_CAPTURE_MAX_BATCHES) {
        /* Pedido real recortado al cap físico — avisar en vez de truncar en
         * silencio (la web puede así informar la duración real soportada). */
        uart_send_diag(PSOC_EVT_CAPTURE_CLAMPED, diag_u16_sat(g_n_batches));
    }
    uart_send_diag(PSOC_EVT_SAMPLING_START, diag_u16_sat(target));
    ADC_StartConvert();
}

static void service_timer_uart_tc(void)
{
    if (rx_state != 0u) {
        watchdog_rx = 1u;
    }
}

static void service_timer_ping_tc(void)
{
    if (g_timer1_remaining_counts != 0u) {
        timer1_start_next_chunk();
        return;
    }

    if (g_timer1_mode == TIMER1_MODE_BOOT_PING) {
        g_boot_ping_due = 1u;
        g_timer1_mode = TIMER1_MODE_NONE;
    } else if (g_timer1_mode == TIMER1_MODE_IDLE_PING) {
        g_idle_ping_due = 1u;
        g_idle_ping_armed = 0u;
        g_timer1_mode = TIMER1_MODE_NONE;
    } else if (g_timer1_mode == TIMER1_MODE_CAL_TICK) {
        g_timer1_mode = TIMER1_MODE_NONE;
        if (g_cal_timer_active) {
            g_cal_progress_due = 1u;
            if (g_cal_watchdog_remaining_ms <= g_cal_progress_period_ms) {
                g_cal_watchdog_remaining_ms = 0u;
                g_cal_watchdog_due = 1u;
                g_cal_timer_active = 0u;
            } else {
                g_cal_watchdog_remaining_ms -= g_cal_progress_period_ms;
                g_timer1_mode = TIMER1_MODE_CAL_TICK;
                g_timer1_remaining_counts = timer_ms_to_counts(g_cal_progress_period_ms);
                timer1_start_next_chunk();
            }
        }
    } else {
        g_timer1_mode = TIMER1_MODE_NONE;
    }
}

static void service_timer_led_tc(void)
{
    if (g_timer2_remaining_counts != 0u) {
        timer2_start_next_chunk();
        return;
    }

    if (g_timer2_mode == TIMER2_MODE_BOOT_LED_OFF) {
        g_boot_led_off_due = 1u;
    } else if (g_timer2_mode == TIMER2_MODE_WAIT) {
        g_timer2_wait_done = 1u;
    } else if (g_timer2_mode == TIMER2_MODE_COMM_BLINK) {
        g_comm_blink_due = 1u;
    }
    g_timer2_mode = TIMER2_MODE_NONE;
}

static void service_timer_capture_watchdog_tc(void)
{
    if (g_timer3_remaining_counts != 0u) {
        timer3_start_next_chunk();
        return;
    }
    g_capture_wd_due = 1u;
}

/* Despacha los eventos de timer permitidos por mask. Ya no pulsa CLEAR_FLAGS:
 * los TC no tienen sticky en superMaquina (y un CLEAR_FLAGS durante SAMPLING
 * resetearía los contadores de muestra/lote en pleno vuelo). */
static void service_timer_events(uint8 mask)
{
    uint8 events = timer_events_take_pending(mask);

    if ((events & CE_TIMER_EVT_UART) != 0u) {
        Timer_Stop();
        (void)Timer_ReadStatusRegister();
        service_timer_uart_tc();
    }
    if ((events & CE_TIMER_EVT_PING) != 0u) {
        Timer_1_Stop();
        (void)Timer_1_ReadStatusRegister();
        service_timer_ping_tc();
    }
    if ((events & CE_TIMER_EVT_LED) != 0u) {
        Timer_2_Stop();
        (void)Timer_2_ReadStatusRegister();
        service_timer_led_tc();
    }
    if ((events & CE_TIMER_EVT_CAPTURE_WD) != 0u) {
        Timer_3_Stop();
        (void)Timer_3_ReadStatusRegister();
        service_timer_capture_watchdog_tc();
    }
}

/* Fallbacks legacy (solo compilados con PSOC_SUPERMAQUINA_OWNS_DMA_IRQ=0,
 * TopDesign viejo con ISRs de DMA separadas): despachan al mismo handler. */
CY_ISR(isr_DMA_DelSig_RAM_Handler)
{
    g_sm_sample_handler();
}

CY_ISR(isr_DMA_Filter_RAM_Handler)
{
    g_sm_sample_handler();
}

CY_ISR(isr_Timer)
{
    uint8 st = Timer_ReadStatusRegister();
    Timer_Stop();
    if (st == 0u) {
        if (++g_t0_spurious >= TIMER_STORM_TRIP) {
            isr_Timer_Disable();
            g_timer_storm |= 0x01u;
            g_t0_spurious = 0u;
        }
        return;
    }
    g_t0_spurious = 0u;
    service_timer_uart_tc();
}

CY_ISR(isr_Timer_1)
{
    uint8 st = Timer_1_ReadStatusRegister();
    Timer_1_Stop();
    if (st == 0u) {
        if (++g_t1_spurious >= TIMER_STORM_TRIP) {
            isr_Timer_1_Disable();
            g_timer_storm |= 0x02u;
            g_t1_spurious = 0u;
        }
        return;
    }
    g_t1_spurious = 0u;
    service_timer_ping_tc();
}

CY_ISR(isr_Timer_2)
{
    uint8 st = Timer_2_ReadStatusRegister();
    Timer_2_Stop();
    if (st == 0u) {
        if (++g_t2_spurious >= TIMER_STORM_TRIP) {
            isr_Timer_2_Disable();
            g_timer_storm |= 0x04u;
            g_t2_spurious = 0u;
        }
        return;
    }
    g_t2_spurious = 0u;
    service_timer_led_tc();
}

CY_ISR(isr_Timer_3)
{
    uint8 st = Timer_3_ReadStatusRegister();
    Timer_3_Stop();
    if (st == 0u) {
        if (++g_t3_spurious >= TIMER_STORM_TRIP) {
            isr_Timer_3_Disable();
            g_timer_storm |= 0x08u;
            g_t3_spurious = 0u;
        }
        return;
    }
    g_t3_spurious = 0u;
    service_timer_capture_watchdog_tc();
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
            psoc_enter_sampling(g_debug_psoc);
        }
    }
    else
    {
        uart_send_diag(PSOC_EVT_SYNC_FALL, g_state);
        ADC_StopConvert();
        capture_watchdog_stop();
        idle_ping_schedule();
        saved = CyEnterCriticalSection();
        capture_reset_locked(capture_target_batches());
        g_state = PSOC_IDLE;
        CyExitCriticalSection(saved);
    }
}

/* IRQ único de superMaquina. Fast path: una sola lectura de status y el
 * handler de muestra vigente; el resto (fin de captura, error, botón) va por
 * el camino lento, que ocurre a lo sumo unas pocas veces por captura.
 * Los TC de timers ya no llegan acá: quedan holdeados en el STATUS_TC de su
 * timer y los consume el polling de service_runtime(). Así cada IRQ sin
 * DONE/ERROR/BUTTON es una muestra, sin ambigüedad ni lecturas extra. */
CY_ISR(isr_SuperMaquina_Handler)
{
    uint8 st = status_Read();

    if ((st & (CE_STATUS_DONE | CE_STATUS_ERROR | CE_STATUS_BUTTON)) == 0u)
    {
        g_sm_sample_handler();
        return;
    }

    g_ce_status = st;

#if PSOC_BUTTON_CAL_ENABLE
    if ((st & CE_STATUS_BUTTON) != 0u)
    {
        g_cal_button_pressed = 1u;
        capture_engine_clear_flags();
        if ((st & (CE_STATUS_DONE | CE_STATUS_ERROR)) == 0u)
        {
            /* El botón puede coincidir con una muestra de bypass (calibración
             * en curso): no perderla. */
            g_sm_sample_handler();
            return;
        }
    }
#endif

    if ((st & CE_STATUS_ERROR) != 0u)
    {
        g_ce_error = error_Read();
        g_sm_sample_handler = sm_sample_noop;
        if (g_state != PSOC_SAMPLING) {
            g_state = PSOC_SAMPLING;   /* canaliza el cierre por service_runtime */
        }
        g_capture_done = 1u;
        return;
    }

    if ((st & CE_STATUS_DONE) != 0u)
    {
        /* La última muestra del último lote llega junto con DONE. */
        g_sm_sample_handler();
        g_sm_sample_handler = sm_sample_noop;
        if (g_state == PSOC_ARMED) {
            g_state = PSOC_SAMPLING;
        }
        g_capture_done = 1u;
    }
}

CY_ISR(isr_Button_Handler)
{
#ifdef CY_ISR_isr_Button_H
    isr_Button_ClearPending();
#endif
#if PSOC_BUTTON_CAL_ENABLE
    g_cal_button_pressed = 1u;
#endif
}

/* ── Trampas de robustez ─────────────────────────────────────────────────────
 * 1) isr_SyncIn: calibration.c la deshabilita/rehabilita alrededor de una
 *    corrida (cal_async_complete). Con PSOC_SUPERMAQUINA_OWNS_SYNC_IRQ el
 *    vector nunca se instalaba, así que ese re-enable dejaba la NVIC apuntando
 *    a IntDefaultHandler (while(1)): el siguiente flanco de SYNC_IN congelaba
 *    el equipo. Se instala SIEMPRE un vector seguro.
 * 2) Cualquier IRQ que siga apuntando a IntDefaultHandler se re-vector a un
 *    trap que reporta el número de IRQ por UART (EVT 0x7E) y la deshabilita,
 *    en vez de colgar el firmware.
 * 3) HardFault (EVT 0x7F) queda en loop parpadeando el LED rápido. */
static void led_write(uint8 value);

CY_ISR(isr_SyncIn_SafeVector)
{
#if defined(SYNC_IN_INTSTAT)
    (void)SYNC_IN_ClearInterrupt();
#endif
}

extern void IntDefaultHandler(void);

#ifndef CY_NUM_INTERRUPTS
#define CY_NUM_INTERRUPTS 32u
#endif

CY_ISR(unexpected_irq_trap)
{
    uint32 vecactive = CY_GET_REG32((reg32 *)0xE000ED04u) & 0x1FFu;
    uint8 irq_n = (uint8)((vecactive >= 16u) ? (vecactive - 16u) : 0xFFu);
    if (irq_n < CY_NUM_INTERRUPTS) {
        CyIntDisable(irq_n);
    }
    uart_send_diag(0x7Eu, irq_n);
}

CY_ISR(hardfault_trap)
{
    uart_send_diag(0x7Fu, 0u);
    for (;;)
    {
        led_write(0u);
        CyDelay(60u);
        led_write(1u);
        CyDelay(60u);
    }
}

static void install_irq_traps(void)
{
    uint8 i;
    for (i = 0u; i < CY_NUM_INTERRUPTS; i++) {
        if (CyIntGetVector(i) == (cyisraddress)IntDefaultHandler) {
            (void)CyIntSetVector(i, unexpected_irq_trap);
        }
    }
    (void)CyIntSetSysVector(3u, hardfault_trap); /* HardFault */
}

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
    if (g_state != PSOC_ARMED && g_state != PSOC_SAMPLING) {
        LED_Write(LED_Read() ^ 0x01u);
    }
#endif
}

static void comm_led_start_identify(void)
{
    g_comm_led_active = 1u;
    g_comm_blinks_remaining = (uint16)IDENTIFY_BLINK_TOGGLES;
    g_comm_blink_due = 0u;
    led_write(1u);
    timer2_arm_ms(COMM_BLINK_MS, TIMER2_MODE_COMM_BLINK);
}

static void idle_ping_service(void)
{
    if (!g_idle_ping_armed && !g_idle_ping_due) {
        idle_ping_schedule();
    }
    if (g_idle_ping_due) {
        g_idle_ping_due = 0u;
        uart_send_ping();
        idle_ping_schedule();
    }
}

uint32 psoc_now_ticks(void)
{
    return 0u; /* Compatibilidad para el servo legacy, hoy compilado apagado. */
}

/* Selecciona la ruta en superMaquina. Con ENGINE_ENABLE=0 actúa como bypass
 * para calibración/diagnóstico; con ENGINE_ENABLE=1 decide qué DRQ deja pasar
 * durante captura. Instala además el handler de muestra de bypass que publica
 * la muestra alineada para calibration.c. */
void dma_route_select(uint8 use_filter)
{
    capture_engine_set_source(capture_engine_source_from_stream(use_filter));
    g_sm_sample_handler = (use_filter != 0u) ? sm_sample_cal_filt
                                             : sm_sample_cal_raw;
}

static uint8 psoc_seed_calibration_from_nv(uint8 reset_defaults_if_missing)
{
    uint8 nv_dac[PSOC_NV_CAL_STAGES];
    uint8 stage_count = psoc_calibration_stage_count();

    if (!g_nv_ready || stage_count == 0u || stage_count > PSOC_NV_CAL_STAGES) {
        return 0u;
    }

    if (psoc_nv_load_for_gain(g_pga_code, nv_dac, stage_count)) {
        psoc_calibration_seed_dac(nv_dac, stage_count);
        return 1u;
    }

    if (reset_defaults_if_missing) {
        psoc_calibration_seed_default_dac();
    }
    return 0u;
}

static void PGAgain_Set(uint8 code)
{
    if (code <= 8u) {
        uint8 changed = (g_pga_code != code) ? 1u : 0u;
        g_pga_code = code;
        psoc_hw_set_pga(code);
        if (changed) {
            g_last_calibration_ok = 0u;
            (void)psoc_seed_calibration_from_nv(1u);
        }
    }
}

static void PGAvdac_Set(uint8 code)
{
    if (code <= 8u) {
        g_pgavdac_code = code;
        psoc_hw_set_pgavdac(code);
    }
}

#define CAPTURE_FILTER_EXTRA_BATCHES \
    ((uint16)((FILTER_GROUP_DELAY + BATCH_SAMPLES - 1u) / BATCH_SAMPLES))

static void psoc_arm(void)
{
    uint8 saved;
    uint8 source;
    uint16 stored_target = capture_target_batches();
    uint16 hw_target;
    uint16 discard = 0u;

    runtime_timers_stop_for_quiet_window();
    psoc_prepare_capture_path();
    source = capture_engine_source_from_stream(g_stream_mode);
    hw_target = stored_target;
    if (source == CE_CFG_SRC_FILTER || source == CE_CFG_SRC_COMBINED) {
        discard = FILTER_GROUP_DELAY;
        if (stored_target > (uint16)(PSOC_CAPTURE_MAX_BATCHES - CAPTURE_FILTER_EXTRA_BATCHES)) {
            stored_target = (uint16)(PSOC_CAPTURE_MAX_BATCHES - CAPTURE_FILTER_EXTRA_BATCHES);
            uart_send_diag(PSOC_EVT_CAPTURE_CLAMPED, diag_u16_sat(capture_target_batches()));
        }
        hw_target = (uint16)(stored_target + CAPTURE_FILTER_EXTRA_BATCHES);
    }
#if PSOC_RAMP_DEBUG_ENABLE
    g_capture_align = (source == CE_CFG_SRC_DEBUG) ? 0u : 1u;
#else
    g_capture_align = 1u;
#endif
    g_capture_adc_raw = (source == CE_CFG_SRC_RAW) ? 1u : 0u;
    saved = CyEnterCriticalSection();
    capture_reset_locked(stored_target);
    g_fir_discard = discard;
    g_state = PSOC_ARMED;
    CyExitCriticalSection(saved);
    capture_engine_configure_target(hw_target);
    capture_engine_set_source(source);
    g_sm_capture_handler = capture_handler_for_source(source);
    g_sm_sample_handler = sm_sample_armed_wait;
    capture_engine_set_enabled(1u, 1u);
    capture_engine_clear_flags();
    capture_engine_pulse(CE_CTRL_ARM);
    ADC_StartConvert();
}

static void psoc_start_now(void)
{
    psoc_prepare_capture_path();
    psoc_enter_sampling(0u);
}

static uint8 psoc_start_calibration_if_idle(uint8 send_ack)
{
    uint8 cal_start;

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
    g_last_calibration_ok = 0u;
    psoc_calibration_servo_abort();
    idle_ping_stop();
    comm_led_stop();
    capture_engine_set_enabled(0u, 0u);
    capture_engine_clear_flags();
    /* La calibracion usa targets definidos para el rango ±2.5 V. Si el host
     * dejo seleccionado ±0.512 V, se fuerza 2V5 solo durante esta corrida y
     * se restaura la seleccion al terminar. La ruta DMA/AMux la gobierna
     * cal_pi_stage_begin (calibration.c) por etapa. */
    psoc_adc_select_calibration_config();
    uart_send_diag(PSOC_EVT_BOOT, PSOC_HW_CLASS);
    uart_send_diag(PSOC_EVT_CAL_START, 0u);
    cal_start = psoc_calibration_start_async();
    if (cal_start == 2u) {
        g_last_calibration_ok = 1u;
        psoc_adc_cal_override(0u);
        psoc_prepare_capture_path();
        idle_ping_schedule();
        uart_send_diag(PSOC_EVT_CAL_DONE, 1u);
        if (send_ack) {
            uart_send_cfg_ack(PSOC_CMD_CALIBRATE, 1u);
        }
        return 1u;
    }
    if (!cal_start) {
        psoc_adc_cal_override(0u);
        psoc_prepare_capture_path();
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

static void psoc_report_adc_snapshot_if_idle(void)
{
    if (g_state != PSOC_IDLE) {
        uart_send_diag(PSOC_EVT_CAL_BUSY, g_state);
        uart_send_cfg_ack(PSOC_CMD_ADC_SNAPSHOT, 0u);
        return;
    }

    psoc_calibration_servo_abort();
    capture_engine_set_enabled(0u, 0u);
    capture_engine_clear_flags();
    uart_send_cfg_ack(PSOC_CMD_ADC_SNAPSHOT, 1u);
    psoc_calibration_report_adc_snapshot();
    dma_route_select(g_stream_mode);
}

static uint8 service_button_calibration(void)
{
#if !PSOC_BUTTON_CAL_ENABLE
    return 0u;
#else
    if (!g_cal_button_pressed) {
        return 0u;
    }

    g_cal_button_pressed = 0u;
    if ((g_state == PSOC_ARMED) || (g_state == PSOC_SAMPLING)) {
        /* Política: botón fuera de IDLE se descarta; en ventana crítica ni
         * siquiera se emite diagnóstico UART. */
        return 0u;
    }
    uart_send_diag(PSOC_EVT_BUTTON, g_state);
    if (g_state == PSOC_IDLE) {
        (void)psoc_start_calibration_if_idle(0u);
    } else {
        uart_send_diag(PSOC_EVT_BUTTON_IGNORED, g_state);
    }
    return 1u;
#endif
}

static void uart_send_capture_batch(uint16 batchIndex)
{
    uint8  frame[FRAME_BYTES];
    uint8 *p;
    const volatile uint8 *src;
    uint16 i;
    uint8  crc = 0u;

    frame[0] = 0xABu;
    frame[1] = (uint8)BATCH_SAMPLES;
    frame[2] = (uint8)( g_seq        & 0xFFu);
    frame[3] = (uint8)((g_seq >> 8u) & 0xFFu);

    /* La ISR guarda los 3 bytes crudos del DMA sin procesar; el alineado a
     * counts (division por DEC_DIV) se hace acá, fuera de la ventana crítica
     * de muestreo. La rampa debug va tal cual (g_capture_align=0). */
    src = &g_capture_raw[(uint32)batchIndex * CAPTURE_BATCH_BYTES];
    p = &frame[4];
    for (i = 0u; i < BATCH_SAMPLES; i++, p += 3u, src += 3u)
    {
        if (g_capture_align) {
            int32 val = psoc_adc_counts_right_aligned(
                g_capture_adc_raw ? dma_buf_to_adc_counts(src) : dma_buf_to_i24(src));
            p[0] = (uint8)( val         & 0xFFu);
            p[1] = (uint8)((val >>  8u) & 0xFFu);
            p[2] = (uint8)((val >> 16u) & 0xFFu);
        } else {
            p[0] = src[0];
            p[1] = src[1];
            p[2] = src[2];
        }
    }

    for (i = 0u; i < (FRAME_BYTES - 1u); i++) { crc ^= frame[i]; }
    frame[FRAME_BYTES - 1u] = crc;

    g_seq++;
    UART_PutArray(frame, FRAME_BYTES);
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
                    case PSOC_CMD_ADC_SNAPSHOT: case PSOC_CMD_ADC_CONFIG:
                    case PSOC_CMD_BLINK_LED:
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
                if (rx_cmd != PSOC_CMD_PONG && rx_cmd != 0xB1u) {
                    uart_send_diag(PSOC_EVT_RX_CMD, rx_cmd);
                }

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
                        case PSOC_CMD_ADC_SNAPSHOT:
                            uart_send_diag(PSOC_EVT_CAL_BUSY, g_state);
                            uart_send_cfg_ack(PSOC_CMD_ADC_SNAPSHOT, 0u);
                            break;
                        case 0xA6u: case 0xA9u: case 0xAAu:
                        case 0xB1u: case 0xB3u: case 0xB4u:
                        case PSOC_CMD_ADC_CONFIG:
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
                        uart_send_diag(PSOC_EVT_BOOT, PSOC_HW_CLASS);
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
                        uart_wait_tx_complete_quiet();
                        psoc_arm();
                        break;
                    case 0xB4u:
                        uart_send_diag(PSOC_EVT_START_NOW, 0u);
                        uart_send_cfg_ack(0xB4u, 1u);
                        psoc_start_now();
                        led_toggle();
                        break;
                    case 0xB3u:
#if PSOC_RAMP_DEBUG_ENABLE
                        g_debug_psoc = (rx_p1 == 0u) ? 0u : 1u;
                        uart_send_diag(PSOC_EVT_DEBUG_MODE, g_debug_psoc);
                        if (g_debug_psoc) {
                            psoc_prepare_capture_path();
                            psoc_enter_sampling(1u);
                        } else {
                            uint8 saved;
                            ADC_StopConvert();
                            capture_watchdog_stop();
                            g_sm_sample_handler = sm_sample_noop;
                            capture_engine_set_enabled(0u, 0u);
                            capture_engine_clear_flags();
                            psoc_prepare_capture_path();
                            idle_ping_schedule();
                            saved = CyEnterCriticalSection();
                            capture_reset_locked(capture_target_batches());
                            g_state = PSOC_IDLE;
                            CyExitCriticalSection(saved);
                        }
#else
                        (void)rx_p1;
                        uart_send_diag(PSOC_EVT_DEBUG_MODE, 0u);
#endif
                        break;
                    case PSOC_CMD_CALIBRATE:
                        (void)psoc_start_calibration_if_idle(1u);
                        led_toggle();
                        break;
                    case PSOC_CMD_SAVE_EEPROM:
                        /* Guardar los DAC de calibración actuales en EEPROM.
                         * El gate es vs PSOC_CAL_STAGE_COUNT (2 en HAMMER, no
                         * el tope fijo PSOC_NV_CAL_STAGES=4 del layout EEPROM
                         * — con ese tope fijo HAMMER nunca guardaba nada).
                         * No se pisa el slot si alguna etapa quedó ok=0. */
                        {
                            uint8 dac_snap[PSOC_NV_CAL_STAGES];
                            uint8 ok_snap = 0u;
                            uint8 stage_count = psoc_calibration_stage_count();
                            uint8 k;
                            for (k = 0u; k < PSOC_NV_CAL_STAGES; k++) {
                                dac_snap[k] = 0u;
                            }
                            for (k = 0u; k < PSOC_NV_CAL_STAGES && k < stage_count; k++) {
                                dac_snap[k] = g_psoc_cal_results[k].final_dac;
                            }
                            if (g_last_calibration_ok &&
                                g_psoc_cal_result_count >= stage_count) {
                                ok_snap = 1u;
                                for (k = 0u; k < stage_count; k++) {
                                    if (!g_psoc_cal_results[k].ok) {
                                        ok_snap = 0u;
                                        break;
                                    }
                                }
                                if (ok_snap) {
                                    ok_snap = psoc_nv_save_for_gain(g_pga_code, dac_snap, stage_count);
                                }
                            }
                            uart_send_cfg_ack(PSOC_CMD_SAVE_EEPROM, ok_snap);
                        }
                        led_toggle();
                        break;
                    case PSOC_CMD_SELECT_STREAM:
                        /* rx_p1: 0=crudo, 1=filtrado FIR */
                        g_stream_mode = (rx_p1 != 0u) ? 1u : 0u;
                        if (g_stream_mode != 0u) {
                            (void)psoc_filter_load_fir_coefficients(g_fir_adquisition_coeffs_q23,
                                                                     FILTER_FIR_NTAPS);
                            psoc_filter_reset_history();
                            psoc_adc_clear_isr_filtered_sample();
#ifdef CY_ISR_isr_DMA_Filter_RAM_H
                            isr_DMA_Filter_RAM_ClearPending();
#endif
                        }
                        dma_route_select(g_stream_mode);
                        uart_send_cfg_ack(PSOC_CMD_SELECT_STREAM, g_stream_mode);
                        led_toggle();
                        break;
                    case PSOC_CMD_ADC_CONFIG:
                        if ((g_state == PSOC_IDLE) && psoc_adc_set_config(rx_p1)) {
                            psoc_adc_cal_override(0u);
                            psoc_prepare_capture_path();
                            uart_send_cfg_ack(PSOC_CMD_ADC_CONFIG, psoc_adc_get_config());
                            uart_send_fs_report();
                        } else {
                            uart_send_cfg_ack(PSOC_CMD_ADC_CONFIG, 0u);
                        }
                        led_toggle();
                        break;
                    case PSOC_CMD_ADC_SNAPSHOT:
                        psoc_report_adc_snapshot_if_idle();
                        led_toggle();
                        break;
                    case PSOC_CMD_BLINK_LED:
                        /* Identificacion no bloqueante: Timer_2 encadena
                         * one-shots y service_comm_led solo consume flags. */
                        comm_led_start_identify();
                        break;
                    default: break;
                }
                break;

            default:
                rx_state = 0u; break;
        }
    }
}

/* Recuperación tras el breaker de tormenta de IRQ: re-inicializa el bloque
 * (Enable deja propagar cualquier clear latcheado, doble lectura de status,
 * pend limpio) y restaura el estado software del modo que estaba en vuelo
 * para que ninguna espera quede huérfana. */
static void timer_storm_recover(void)
{
    uint8 mask;
    uint8 saved;

    if (g_timer_storm == 0u) { return; }
    saved = CyEnterCriticalSection();
    mask = g_timer_storm;
    g_timer_storm = 0u;
    CyExitCriticalSection(saved);

    if (mask & 0x01u) {
        Timer_Enable();
        (void)Timer_ReadStatusRegister();
        Timer_Stop();
        (void)Timer_ReadStatusRegister();
        isr_Timer_ClearPending();
        if (rx_state != 0u) {
            watchdog_rx = 1u;   /* resincroniza el parser a mano */
        }
        uart_send_diag(PSOC_EVT_TIMER_STORM, 0u);
    }
    if (mask & 0x02u) {
        uint8 mode;
        Timer_1_Enable();
        (void)Timer_1_ReadStatusRegister();
        Timer_1_Stop();
        (void)Timer_1_ReadStatusRegister();
        isr_Timer_1_ClearPending();
        saved = CyEnterCriticalSection();
        mode = g_timer1_mode;
        g_timer1_mode = TIMER1_MODE_NONE;
        g_timer1_remaining_counts = 0u;
        if (mode == TIMER1_MODE_BOOT_PING) {
            g_boot_ping_due = 1u;
        } else if (mode == TIMER1_MODE_IDLE_PING) {
            g_idle_ping_due = 1u;
            g_idle_ping_armed = 0u;
        }
        CyExitCriticalSection(saved);
        if (mode == TIMER1_MODE_CAL_TICK && g_cal_timer_active) {
            /* Reproduce la cadena del tick de calibración. */
            g_cal_progress_due = 1u;
            timer1_arm_ms(g_cal_progress_period_ms, TIMER1_MODE_CAL_TICK);
        }
        uart_send_diag(PSOC_EVT_TIMER_STORM, 1u);
    }
    if (mask & 0x04u) {
        uint8 mode;
        Timer_2_Enable();
        (void)Timer_2_ReadStatusRegister();
        Timer_2_Stop();
        (void)Timer_2_ReadStatusRegister();
        isr_Timer_2_ClearPending();
        saved = CyEnterCriticalSection();
        mode = g_timer2_mode;
        g_timer2_mode = TIMER2_MODE_NONE;
        g_timer2_remaining_counts = 0u;
        if (mode == TIMER2_MODE_BOOT_LED_OFF) {
            g_boot_led_off_due = 1u;
        } else if (mode == TIMER2_MODE_WAIT) {
            g_timer2_wait_done = 1u;
        } else if (mode == TIMER2_MODE_COMM_BLINK) {
            g_comm_blink_due = 1u;
        }
        CyExitCriticalSection(saved);
        uart_send_diag(PSOC_EVT_TIMER_STORM, 2u);
    }
    if (mask & 0x08u) {
        Timer_3_Enable();
        (void)Timer_3_ReadStatusRegister();
        Timer_3_Stop();
        (void)Timer_3_ReadStatusRegister();
        isr_Timer_3_ClearPending();
        g_timer3_remaining_counts = 0u;
        if (g_state == PSOC_SAMPLING && g_capture_done == 0u) {
            capture_watchdog_arm();   /* re-arma la ventana completa */
        }
        uart_send_diag(PSOC_EVT_TIMER_STORM, 3u);
    }
}

static void service_runtime(void)
{
    if ((g_state == PSOC_ARMED) || (g_state == PSOC_SAMPLING)) {
        /* Ventana crítica: la única actividad secundaria permitida es el
         * watchdog de captura (troceado del Timer_3). uart/ping/led quedan
         * holdeados en su STATUS_TC hasta volver a un estado tranquilo. */
        (void)capture_engine_poll_capture_wd();
        service_timer_events(CE_TIMER_EVT_CAPTURE_WD);
    } else {
        (void)capture_engine_poll_timer_events();
        service_timer_events(CE_TIMER_EVT_MASK);
        timer_storm_recover();
    }

    if (g_state == PSOC_CALIBRATING) {
        if (psoc_calibration_service_async()) {
            uint8 ok = psoc_calibration_async_result_ok();
            g_state = PSOC_IDLE;
            g_last_calibration_ok = ok ? 1u : 0u;
            psoc_adc_cal_override(0u);
            psoc_prepare_capture_path();
            idle_ping_schedule();
            uart_send_diag(PSOC_EVT_CAL_DONE, ok);
            if (g_cal_ack_pending) {
                g_cal_ack_pending = 0u;
                uart_send_cfg_ack(PSOC_CMD_CALIBRATE, ok);
            }
        }
        uart_service();
        return;
    }

    if (g_state == PSOC_ARMED) {
        return;   /* HOT_WAIT silencioso: sin UART RX/TX, LED ni pings. */
    }

    if (g_state == PSOC_SAMPLING) {
        uint8 capture_error = g_ce_error;
        if (g_capture_done == 0u) {
            if (!g_capture_wd_due) {
                return;   /* Silencio total: sin UART RX/TX, sin LED, sin pings. */
            }
            /* Watchdog: la captura no llegó a DONE en el tiempo esperado
             * (ADC parado, DMA sin disparos, sync perdido...). Abortar con
             * diagnóstico en vez de quedar mudo para siempre. */
            g_capture_wd_due = 0u;
            g_sm_sample_handler = sm_sample_noop;
            capture_engine_pulse(CE_CTRL_STOP);
            uart_send_diag(PSOC_EVT_CAPTURE_WATCHDOG, state_Read());
            g_capture_done = 1u;
        }
        ADC_StopConvert();
        capture_watchdog_stop();
        g_sm_sample_handler = sm_sample_noop;
        capture_engine_set_enabled(0u, 0u);
        capture_engine_clear_flags();
        /* Lotes completos realmente escritos por la ISR (los parciales de un
         * aborto se descartan). */
        g_batches_captured =
            (uint16)(((uint32)(g_capture_wr - g_capture_raw)) / CAPTURE_BATCH_BYTES);
        idle_ping_schedule();
        uart_send_diag(PSOC_EVT_CAPTURE_DONE,
                       (capture_error != 0u) ? capture_error : diag_u16_sat(g_batches_captured));
        g_ce_error = 0u;
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

static void service_comm_led(void)
{
    if (!g_comm_led_active || !g_comm_blink_due) {
        return;
    }

    g_comm_blink_due = 0u;
    if (g_comm_blinks_remaining > 0u) {
        g_comm_blinks_remaining--;
        led_toggle();
    }

    if (g_comm_blinks_remaining > 0u) {
        timer2_arm_ms(COMM_BLINK_MS, TIMER2_MODE_COMM_BLINK);
    } else {
        g_comm_led_active = 0u;
        led_write(1u);
    }
}

static void wait_ms_timer2(uint32 ms)
{
    g_timer2_wait_done = 0u;
    timer2_arm_ms(ms, TIMER2_MODE_WAIT);
    while (!g_timer2_wait_done) {
        (void)service_button_calibration();
        service_runtime();
    }
    g_timer2_wait_done = 0u;
    timer2_stop_quiet();
}

static void wait_for_esp(void)
{
    g_boot_ping_due = 1u;
    g_boot_led_off_due = 0u;
    led_write(1u);
    uart_send_diag(PSOC_EVT_WAIT_ESP, 0u);
    while (!g_esp_connected)
    {
        (void)service_button_calibration();
        service_runtime();
        if (g_esp_connected) {
            break;
        }

        /* Self-heal: si una calibración por botón usó Timer_1 (tick de cal)
         * y terminó, el ping de arranque debe renacer solo. */
        if (g_state == PSOC_IDLE && g_timer1_mode == TIMER1_MODE_NONE &&
            !g_boot_ping_due && !g_idle_ping_armed && !g_idle_ping_due) {
            g_boot_ping_due = 1u;
        }

        if (g_boot_ping_due) {
            g_boot_ping_due = 0u;
            uart_send_ping();
            led_write(1u);
            timer1_arm_ms(PING_PERIOD_MS, TIMER1_MODE_BOOT_PING);
            timer2_arm_ms((uint32)(PING_PERIOD_MS - PING_OFF_MS), TIMER2_MODE_BOOT_LED_OFF);
        }

        if (g_boot_led_off_due) {
            g_boot_led_off_due = 0u;
            led_write(0u);
        }
    }
    timer1_stop_quiet();
    timer2_stop_quiet();
    g_boot_ping_due = 0u;
    g_boot_led_off_due = 0u;
    led_write(1u);
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

/* Inicializa los tres canales DMA (TopDesign consolidado, sin Single/Lote):
 *   DMA_DelSig_RAM    ADC → RAM (g_dma_raw_buf), activo en ruta raw
 *   DMA_Filter_RAM    Filter (HOLDA, Canal A) → RAM (g_dma_filt_buf)
 *   DMA_DelSig_Filter ADC → Filter_STAGEA (siempre activo, sin ISR), activo
 *                     en ruta filtrada
 * Todos los TD son circulares (nextTd == mismo TD): cada canal recicla solo.
 * Cual DRQ recibe el trigger real lo decide superMaquina desde cfg/ctrl; el
 * ARM solo configura la ruta con dma_route_select(). DMA_Filter_RAM solo
 * produce datos cuando el Filter esta siendo alimentado. */
static void dma_adc_init(void)
{
    uint8 td;

    /* ── Raw: ADC DelSig → g_dma_raw_buf (3 bytes/disparo) ───────────────── */
    DMA_DelSig_RAM_DmaInitialize(3u, 1u,
        HI16((uint32)ADC_DEC_SAMP_PTR),
        HI16((uint32)(void *)g_dma_raw_buf));
    td = CyDmaTdAllocate();
    CyDmaTdSetConfiguration(td, 3u, td,
        DMA_DelSig_RAM__TD_TERMOUT_EN | TD_INC_SRC_ADR | TD_INC_DST_ADR);
    CyDmaTdSetAddress(td,
        LO16((uint32)ADC_DEC_SAMP_PTR),
        LO16((uint32)(void *)g_dma_raw_buf));
    CyDmaChSetInitialTd(DMA_DelSig_RAM_DmaHandle, td);
    CyDmaChEnable(DMA_DelSig_RAM_DmaHandle, 1u);

    /* ── Filter input: ADC DelSig → Filter_STAGEA (siempre activo, sin ISR) ── */
    DMA_DelSig_Filter_DmaInitialize(3u, 1u,
        HI16((uint32)ADC_DEC_SAMP_PTR),
        HI16((uint32)Filter_STAGEA_PTR));
    td = CyDmaTdAllocate();
    CyDmaTdSetConfiguration(td, 3u, td,
        DMA_DelSig_Filter__TD_TERMOUT_EN | TD_INC_SRC_ADR | TD_INC_DST_ADR);
    CyDmaTdSetAddress(td,
        LO16((uint32)ADC_DEC_SAMP_PTR),
        LO16((uint32)Filter_STAGEA_PTR));
    CyDmaChSetInitialTd(DMA_DelSig_Filter_DmaHandle, td);
    CyDmaChEnable(DMA_DelSig_Filter_DmaHandle, 1u);

    /* ── Filter output: Filter_HOLDA → g_dma_filt_buf (3 bytes/disparo) ──── */
    DMA_Filter_RAM_DmaInitialize(3u, 1u,
        HI16((uint32)Filter_HOLDA_PTR),
        HI16((uint32)(void *)g_dma_filt_buf));
    td = CyDmaTdAllocate();
    CyDmaTdSetConfiguration(td, 3u, td,
        DMA_Filter_RAM__TD_TERMOUT_EN | TD_INC_SRC_ADR | TD_INC_DST_ADR);
    CyDmaTdSetAddress(td,
        LO16((uint32)Filter_HOLDA_PTR),
        LO16((uint32)(void *)g_dma_filt_buf));
    CyDmaChSetInitialTd(DMA_Filter_RAM_DmaHandle, td);
    CyDmaChEnable(DMA_Filter_RAM_DmaHandle, 1u);

    /* Ruta por defecto: raw, igual al comportamiento previo al mux. */
    dma_route_select(0u);
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
    psoc_debug_start();
    psoc_debug_print("\r\n[PC] boot\r\n");
    psoc_debug_print_u32("[PC] PSOC_HW_CLASS=", PSOC_HW_CLASS);
    psoc_debug_print_u32("[PC] PSOC_CAL_STAGE_COUNT=", psoc_calibration_stage_count());
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
    g_nv_ready = 1u;

    psoc_hw_start_analog(g_pga_code, g_pgavdac_code);
    /* Arranca todos los VDAC de calibracion en su adelanto/feedforward de
     * tabla. Si EEPROM tiene una calibracion valida, se pisa justo abajo con
     * esos valores guardados para ahorrar aun mas tiempo. */
    psoc_calibration_start_references();

    /* Siempre arranca con el ultimo slot EEPROM de la ganancia actual.
     * Si no hay slot valido, deja explicitamente los adelantos nominales. */
#if PSOC_LOAD_NV_CAL_ON_BOOT
    (void)psoc_seed_calibration_from_nv(1u);
#endif

    Filter_Start();
    /* Activa el FIR de adquisicion (FIR_adquisition.h) -- calibration.c
     * carga el de calibracion (FIR_calibration.h) solo durante una corrida
     * y restaura este al terminar (ver cal_pi_start/cal_async_complete). */
    (void)psoc_filter_load_fir_coefficients(g_fir_adquisition_coeffs_q23, FILTER_FIR_NTAPS);
    dma_adc_init();
    capture_engine_reset_hw();
    capture_engine_set_enabled(0u, 0u);
    dma_route_select(0u);

    psoc_prepare_capture_path();
    uart_send_diag(PSOC_EVT_ANALOG_READY, 0u);

#if !PSOC_SUPERMAQUINA_OWNS_DMA_IRQ
#ifdef CY_ISR_isr_DMA_DelSig_RAM_H
    isr_DMA_DelSig_RAM_StartEx(isr_DMA_DelSig_RAM_Handler);
#endif
#ifdef CY_ISR_isr_DMA_Filter_RAM_H
    isr_DMA_Filter_RAM_StartEx(isr_DMA_Filter_RAM_Handler);
#endif
#endif
    fixed_timers_init();
#if !PSOC_SUPERMAQUINA_OWNS_SYNC_IRQ
#ifdef CY_ISR_isr_SyncIn_H
    isr_SyncIn_StartEx(isr_SyncIn);
#endif
#endif
    isr_SuperMaquina_StartEx(isr_SuperMaquina_Handler);
    isr_SuperMaquina_SetPriority(0u);
    Clock_1_Start();
#if PSOC_BUTTON_CAL_ENABLE
#ifdef CY_ISR_isr_Button_H
    isr_Button_ClearPending();
    isr_Button_StartEx(isr_Button_Handler);
#endif
#endif
#if PSOC_SUPERMAQUINA_OWNS_SYNC_IRQ
#ifdef CY_ISR_isr_SyncIn_H
    *isr_SyncIn_INTC_VECTOR = (uint32)isr_SyncIn_SafeVector;
#endif
#endif
    install_irq_traps();

    /* ── Loop de arranque: busca el ESP sin bloquear UART/ADC ───────────── */
    wait_for_esp();
    psoc_calibration_servo_enable(0u);

    /* ── 5 parpadeos rápidos al conectar ─────────────────────────────────── */
    for (i = 0u; i < 5u; i++)
    {
        led_write(0u); wait_ms_timer2(CONNECT_BLINK_MS);
        led_write(1u); wait_ms_timer2(CONNECT_BLINK_MS);
    }
    idle_ping_schedule();

    /* ── Loop principal ─────────────────────────────────────────────────── */
    for (;;)
    {
        if (service_button_calibration()) {
            continue;
        }
        service_runtime();

        if (g_state == PSOC_ARMED || g_state == PSOC_SAMPLING || capture_dump_pending()) {
            idle_ping_stop();
            continue;   /* Sin LED ni pings durante captura y volcado */
        }

        if (g_state == PSOC_IDLE && !capture_dump_pending()) {
            (void)psoc_calibration_servo_service();
            idle_ping_service();
        } else {
            idle_ping_stop();
        }

        service_comm_led();
    }
}

/* [] END OF FILE */
