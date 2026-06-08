/*******************************************************************************
* main.c — Firmware PSoC unificado para geófono (GEO) y martillo (HAMMER).
*
* El hardware del TopDesign determina automáticamente el modo de compilación:
*   Si el proyecto tiene PGAgain  → CLASS = GEO    (geófono diferencial)
*   Si el proyecto tiene PGA      → CLASS = HAMMER  (martillo piezoeléctrico)
* No hay que cambiar nada en este archivo al cambiar de proyecto.
*
* ─────────────────────────────────────────────────────────────────────────────
*  GEO — componentes TopDesign:
*    UART, ADC, VDAC, PGAgain, PGAvdac, OPAref, PGAp, PGAn, LPF_1, LPF_2,
*    OPAbp, OPAadder, OPAlp, Timer, LED, isr_DelSig, isr_Timer, isr_SyncIn.
*
*  HAMMER — componentes TopDesign:
*    UART, ADC, VDAC, PGA, PGA_ref, Opa_ref, LPF_ADC, LPF_ref,
*    Timer, LED, isr_DelSig, isr_Timer, isr_SyncIn.
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
*       0xA5 status/probe   0xA6 set PGAgain (0-8)   0xA9 set PGAvdac (0-8)
*       0xAA set VDAC       0xB1 arm (espera SYNC)   0xB3 debug rampa (0/1)
*       0xB4 start-now      0xC1 pong
*   2 param : [0xAB][0xA3][n_lo][n_hi][0xA3^n_lo^n_hi]   (set N lotes)
*
* Detección PSoC ↔ ESP:
*   PSoC → ESP : [0xAB][0xC0][0x00][0xC0]             (ping)
*   ESP  → PSoC: [0xAB][0xC1][0x00][0xC1]             (pong)
*   PSoC → ESP : [0xAB][0xC2][cmd][val][0xC2^cmd^val]  (config ACK)
*******************************************************************************/

#include "project.h"

/* ── Auto-detección de hardware por los componentes del TopDesign ────────── */
#define GEO    0
#define HAMMER 1

#if defined(PGAgain_DEFAULT_GAIN)
  #define CLASS GEO
#elif defined(PGA_DEFAULT_GAIN)
  #define CLASS HAMMER
#else
  #error "Hardware no reconocido: el TopDesign debe tener PGAgain (GEO) o PGA (HAMMER)."
#endif

/* -------------------------------------------------------------------------- */
#define VDAC_INIT_VALUE    0x94u
#define TIMER_TICK_MS      10u
#define TIMEOUT_COUNTS     240000u    /* 10 ms @ 24 MHz — tick de sistema */

#define BATCH_SAMPLES      30u
#define FRAME_BYTES        (4u + BATCH_SAMPLES * 3u + 1u)   /* 95 */
#define SAMPLE_BYTES       3u
#define CAPTURE_BATCH_BYTES (BATCH_SAMPLES * SAMPLE_BYTES)

#ifndef PSOC_CAPTURE_MAX_BATCHES
#define PSOC_CAPTURE_MAX_BATCHES 512u
#endif

#define PSOC_IDLE          0u
#define PSOC_ARMED         1u
#define PSOC_SAMPLING      2u

#define PSOC_CMD_PING      0xC0u
#define PSOC_CMD_PONG      0xC1u
#define PSOC_CMD_CFG_ACK   0xC2u

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

#ifndef PSOC_DEBUG_LED
#define PSOC_DEBUG_LED        1
#endif

/* -------------------------------------------------------------------------- */
static volatile int32  g_adc_raw          = 0;

static volatile uint8  g_capture_raw[PSOC_CAPTURE_MAX_BATCHES][CAPTURE_BATCH_BYTES];
static volatile uint16 g_batch_fill       = 0u;
static volatile uint8  g_batch_ready      = 0u;
static          uint16 g_seq              = 0u;
static volatile uint16 g_batches_captured = 0u;
static volatile uint8  g_capture_done     = 0u;

#if CLASS == GEO
static uint8  g_pga_code     = PGAgain_DEFAULT_GAIN;
static uint8  g_pgavdac_code = PGAvdac_DEFAULT_GAIN;
#else  /* HAMMER */
static uint8  g_pga_code     = PGA_DEFAULT_GAIN;
static uint8  g_pgavdac_code = PGA_ref_DEFAULT_GAIN;
#endif

static uint8  g_vdac_val     = VDAC_INIT_VALUE;

static volatile uint8  g_state        = PSOC_IDLE;
static          uint16 g_n_batches    = 0u;
static volatile uint16 g_batches_sent = 0u;
static volatile uint8  g_debug_psoc   = 0u;
static          uint32 g_dbg_cnt      = 0u;

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
    frame[1] = 0xC3u;
    frame[2] = fs_lo;
    frame[3] = fs_hi;
    frame[4] = (uint8)(0xC3u ^ fs_lo ^ fs_hi);
    UART_PutArray(frame, (uint8)sizeof(frame));
}

static int32 adc_counts_right_aligned(int32 adcCounts)
{
    #if(ADC_CFG1_DEC_DIV != 0)
        if (ADC_Config == ADC_CFG1) { adcCounts /= ADC_CFG1_DEC_DIV; }
    #endif
    #if((ADC_CFG2_DEC_DIV != 0) && (ADC_DEFAULT_NUM_CONFIGS > 1))
        if (ADC_Config == ADC_CFG2) { adcCounts /= ADC_CFG2_DEC_DIV; }
    #endif
    #if((ADC_CFG3_DEC_DIV != 0) && (ADC_DEFAULT_NUM_CONFIGS > 2))
        if (ADC_Config == ADC_CFG3) { adcCounts /= ADC_CFG3_DEC_DIV; }
    #endif
    #if((ADC_CFG4_DEC_DIV != 0) && (ADC_DEFAULT_NUM_CONFIGS > 3))
        if (ADC_Config == ADC_CFG4) { adcCounts /= ADC_CFG4_DEC_DIV; }
    #endif
    return adcCounts;
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
    ADC_StopConvert();
    timer_stop_quiet();
    saved = CyEnterCriticalSection();
    g_debug_psoc = debugMode ? 1u : 0u;
    capture_reset_locked();
    g_state = PSOC_SAMPLING;
    CyExitCriticalSection(saved);
    ADC_StartConvert();
}

CY_ISR(isr_DelSigReady)
{
    g_adc_raw = adc_counts_right_aligned(ADC_GetResult32());

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
    if (SYNC_IN_Read())
    {
        if (g_state == PSOC_ARMED)
        {
            psoc_enter_sampling(g_debug_psoc);
        }
    }
    else
    {
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

/* -------------------------------------------------------------------------- */

static void rx_watchdog_start(void) { g_rx_watch_ticks = 0u; watchdog_rx = 0u; }
static void rx_watchdog_stop(void)  { g_rx_watch_ticks = 0u; watchdog_rx = 0u; }

static void led_write(uint8 value)
{
#if defined(LED_Write)
    LED_Write(value);
#else
    (void)value;
#endif
}

static void led_toggle(void)
{
#if defined(LED_Write) && defined(LED_Read)
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

static uint8 ticks_due(uint32 now, uint32 due)
{
    return ((int32)(now - due) >= 0) ? 1u : 0u;
}

static void PGAgain_Set(uint8 code)
{
    if (code <= 8u) {
        g_pga_code = code;
#if CLASS == GEO
        PGAgain_SetGain(code);
#else  /* HAMMER */
        PGA_SetGain(code);
#endif
    }
}

static void PGAvdac_Set(uint8 code)
{
    if (code <= 8u) {
        g_pgavdac_code = code;
#if CLASS == GEO
        PGAvdac_SetGain(code);
#else  /* HAMMER */
        PGA_ref_SetGain(code);
#endif
    }
}

static void psoc_arm(void)
{
    uint8 saved;
    ADC_StopConvert();
    saved = CyEnterCriticalSection();
    capture_reset_locked();
    g_state = PSOC_ARMED;
    CyExitCriticalSection(saved);
}

static void psoc_start_now(void)
{
    psoc_enter_sampling(0u);
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
                    case 0xB1u: case 0xB3u: case 0xB4u: case PSOC_CMD_PONG:
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

                g_esp_connected  = 1u;
                g_comm_countdown = COMM_WINDOW_TICKS;

                switch (rx_cmd)
                {
                    case PSOC_CMD_PONG: break;
                    case 0xA5u:
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
                        VDAC_SetValue(rx_p1); g_vdac_val = rx_p1;
                        uart_send_cfg_ack(0xAAu, g_vdac_val);
                        led_toggle(); break;
                    case 0xA3u:
                        g_n_batches = (uint16)rx_p1 | ((uint16)rx_p2 << 8u); break;
                    case 0xB1u:
                        psoc_arm(); led_toggle(); break;
                    case 0xB4u:
                        psoc_start_now(); led_toggle(); break;
                    case 0xB3u:
                        g_debug_psoc = (rx_p1 == 0u) ? 0u : 1u;
                        if (g_debug_psoc) {
                            psoc_enter_sampling(1u);
                        } else {
                            ADC_StopConvert();
                            timer_start_runtime();
                            g_batch_fill   = 0u;
                            g_batch_ready  = 0u;
                            g_capture_done = 0u;
                            g_state        = PSOC_IDLE;
                        }
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
    if (g_state == PSOC_SAMPLING) {
        if (g_capture_done == 0u) {
            return;   /* Silencio total: sin UART RX/TX, sin LED, sin pings. */
        }
        ADC_StopConvert();
        timer_start_runtime();
        g_capture_done = 0u;
        g_state = PSOC_IDLE;
    }

    if (capture_dump_pending()) {
        uart_send_capture_batch(g_batches_sent);
        g_batches_sent++;
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
        service_runtime();
    }
}

static void wait_for_esp(void)
{
    uint32 nextPing  = timer_now_ticks();
    uint32 pingStart = nextPing;

    led_write(1u);
    while (!g_esp_connected)
    {
        uint32 now;
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

int main(void)
{
    uint8 i;

    CyGlobalIntEnable;

    UART_Start();

#if CLASS == GEO
    /* ── Cadena analógica del geófono diferencial ── */
    OPAref_Start();
    PGAp_Start();  PGAn_Start();
    PGAp_SetGain(PGAp_GAIN_02);
    PGAn_SetGain(PGAp_GAIN_02);
    LPF_1_Start(); LPF_2_Start();
    PGAgain_Start();
    PGAgain_SetGain(g_pga_code);
    OPAbp_Start();
    OPAadder_Start();
    PGAvdac_Start();
    PGAvdac_SetGain(g_pgavdac_code);
    OPAlp_Start();
#else  /* HAMMER */
    /* ── Cadena analógica del martillo piezoeléctrico ── */
    LPF_ref_Start();
    LPF_ADC_Start();
    Opa_ref_Start();
    PGA_ref_Start();
    PGA_ref_SetGain(g_pgavdac_code);
    PGA_Start();
    PGA_SetGain(g_pga_code);
    Opa_LP_Start();
#endif

    VDAC_Start();
    VDAC_SetValue(VDAC_INIT_VALUE);

    ADC_Start();
    ADC_StopConvert();

    isr_DelSig_StartEx(isr_DelSigReady);
    isr_Timer_StartEx(isr_Timer);
    isr_SyncIn_StartEx(isr_SyncIn);

    Timer_Stop();
    Timer_WritePeriod(TIMEOUT_COUNTS);
    Timer_Start();

    /* ── Loop de arranque: busca el ESP sin bloquear UART/ADC ───────────── */
    wait_for_esp();

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

        service_runtime();
        if (g_state == PSOC_SAMPLING || capture_dump_pending()) {
            continue;   /* Sin LED ni pings durante captura y volcado */
        }
        now = timer_now_ticks();

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
