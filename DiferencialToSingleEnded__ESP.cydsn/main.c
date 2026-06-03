/*******************************************************************************
* main.c — Geofono single-ended, transporte por UART hacia el ESP esclavo.
*
* Cadena: MATLAB → maestro → esclavo (ESP-NOW) → PSoC (UART).
* El PSoC muestrea N lotes de 30 muestras RAW (sin Filter) y los envía en
* tiempo real al ESP esclavo por UART. El arranque del muestreo es por el pin
* digital SYNC_IN (lo maneja el ESP esclavo).
*
* ───────────────────────────────────────────────────────────────────────────
*  REQUISITOS EN PSOC CREATOR (TopDesign.cysch / .cydwr) — PENDIENTE (usuario):
*    1. RENOMBRAR el componente UART  "UART_PC"  ->  "UART".
*    2. QUITAR el componente "Filter" (FIR) y su ISR "isr_Filter".
*    3. QUITAR el componente "SPI Slave (SPIS_1)" y el pin "DATA_READY".
*    4. CONSERVAR el pin digital input "SYNC_IN" con interrupt
*       "Rising and Falling edges" e ISR "isr_SyncIn".
*    5. CABLEAR la UART al ESP esclavo:
*         UART.TX (PSoC)  ->  RX del ESP esclavo
*         UART.RX (PSoC)  <-  TX del ESP esclavo
*       Baud actual: 115200 (debe coincidir con PSOC_UART_BAUD del firmware del
*       esclavo). El batch es de 95 bytes a ~34 lotes/s.
*       Configurar TX Buffer Size >= 96 y RX Buffer Size >= 16 (modo Interrupt)
*       para que UART_PutArray no bloquee y no se pierdan muestras entre lotes.
*    6. CONSERVAR: ADC (DelSig), VDAC, PGAgain, PGAvdac, OPA*, LPF_1/2, Timer,
*       LED, isr_DelSig, isr_Timer.
* ───────────────────────────────────────────────────────────────────────────
*
* TX UART (al ESP) — frame de datos RAW (95 bytes):
*   [0xAB][n=30][seq_lo][seq_hi] + 30×3 bytes (raw LE: b0,b1,b2) + [crc XOR]
*
* El muestreo SIEMPRE arranca por el flanco de subida del pin SYNC_IN (lo
* levanta el ESP esclavo, tanto en START normal como en "Ver"). La UART solo
* lleva N, VDAC, configuración y las muestras del ADC.
*
* RX UART (del ESP) — comandos con checksum XOR:
*   1 parámetro : [0xAB][cmd][param][cmd^param]
*       0xA5 status/probe (responde ping)        0xA6 set PGAgain (0-8)
*       0xA9 set PGAvdac (0-8)                   0xAA set VDAC (0-255)
*       0xB1 pre-start/arm (espera flanco SYNC)  0xB3 debug PSoC rampa (0/1)
*       0xB4 start-now ADC real (solo diagnostico; Ver usa flanco SYNC)
*       0xC1 pong del ESP (respuesta al ping de deteccion)
*   2 parámetros: [0xAB][0xA3][n_lo][n_hi][0xA3^n_lo^n_hi]
*       0xA3 set N (lotes, 16 bits)
*
* Detección mutua PSoC ↔ ESP:
*   PSoC → ESP : [0xAB][0xC0][0x00][0xC0]  (ping)
*   ESP  → PSoC: [0xAB][0xC1][0x00][0xC1]  (pong)
*   PSoC → ESP : [0xAB][0xC2][cmd][val][0xC2^cmd^val] (config ACK)
*   PSoC queda buscando al ESP mientras sigue procesando UART y batches.
*******************************************************************************/

#include "project.h"

/* -------------------------------------------------------------------------- */
#define VDAC_INIT_VALUE    0x94u
#define TIMER_TICK_MS      10u
#define TIMEOUT_COUNTS     240000u    /* 10 ms @ 24 MHz — tick de sistema     */

#define BATCH_SAMPLES      30u
#define FRAME_BYTES        (4u + BATCH_SAMPLES * 3u + 1u)   /* 95 */

/* Estados del PSoC */
#define PSOC_IDLE          0u
#define PSOC_ARMED         1u
#define PSOC_SAMPLING      2u

/* Ping / pong de detección mutua con el ESP */
#define PSOC_CMD_PING      0xC0u   /* PSoC → ESP: ¿estás ahí?           */
#define PSOC_CMD_PONG      0xC1u   /* ESP  → PSoC: confirmación          */
#define PSOC_CMD_CFG_ACK   0xC2u   /* PSoC -> ESP: config aplicada       */

/* Período de ping en el loop de arranque (ms) */
#define PING_PERIOD_MS     700u
#define PING_OFF_MS        100u    /* LED apagado al final de cada ciclo */
#define IDLE_PING_MS       1000u

/* LED operacional: ventana de "comunicando" expresada en ticks del Timer. */
#define COMM_WINDOW_MS     500u
#define COMM_BLINK_MS       50u

#define MS_TO_TICKS(ms)       (((ms) + TIMER_TICK_MS - 1u) / TIMER_TICK_MS)
#define PING_PERIOD_TICKS     MS_TO_TICKS(PING_PERIOD_MS)
#define PING_OFF_TICKS        MS_TO_TICKS(PING_OFF_MS)
#define IDLE_PING_TICKS       MS_TO_TICKS(IDLE_PING_MS)
#define COMM_WINDOW_TICKS     MS_TO_TICKS(COMM_WINDOW_MS)
#define COMM_BLINK_TICKS      MS_TO_TICKS(COMM_BLINK_MS)
#define CONNECT_BLINK_TICKS   MS_TO_TICKS(80u)

/* -------------------------------------------------------------------------- */
/* Muestra ADC cruda */
static volatile int32  g_adc_raw       = 0;

/* Lote en construcción */
static volatile int32  g_batch_raw[BATCH_SAMPLES];
static volatile uint16 g_batch_fill    = 0u;
static volatile uint8  g_batch_ready   = 0u;
static          uint16 g_seq           = 0u;

/* Configuración / estado */
static uint8  g_pga_code     = PGAgain_DEFAULT_GAIN;
static uint8  g_pgavdac_code = PGAvdac_DEFAULT_GAIN;
static uint8  g_vdac_val     = VDAC_INIT_VALUE;

static volatile uint8  g_state        = PSOC_IDLE;
static          uint16 g_n_batches    = 0u;
static volatile uint16 g_batches_sent = 0u;
static volatile uint8  g_debug_psoc   = 0u;
static          uint32 g_dbg_cnt      = 0u;

/* RX UART */
static volatile uint8 rx_state    = 0u;
static volatile uint8 rx_cmd      = 0u;
static volatile uint8 rx_p1       = 0u;
static volatile uint8 rx_p2       = 0u;
static volatile uint8 watchdog_rx = 0u;
static volatile uint32 g_timer_ticks = 0u;

/* LED operacional (post-arranque) */
static          uint8  g_esp_connected   = 0u;
static          uint32 g_comm_countdown  = 0u;

/* Watchdog UART via Timer */
static volatile uint8 g_rx_watch_ticks = 0u;
#define RX_WATCHDOG_TICKS  1u

/* -------------------------------------------------------------------------- */

static const uint8 g_ping_frame[4] = {
    0xABu, PSOC_CMD_PING, 0x00u, PSOC_CMD_PING  /* checksum = 0xC0^0x00 */
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

static int32 adc_counts_right_aligned(int32 adcCounts)
{
    /* ADC_GetResult32() is correct for 18-bit DelSig, but this project uses
     * left alignment to bit 23. Divide by the generated DEC_DIV to transmit
     * normal signed ADC counts instead of left-aligned register units. */
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

CY_ISR(isr_DelSigReady)
{
    g_adc_raw = adc_counts_right_aligned(ADC_GetResult32());

    if (g_state == PSOC_SAMPLING && g_batch_ready == 0u &&
        g_batch_fill < BATCH_SAMPLES)
    {
        g_batch_raw[g_batch_fill] = g_adc_raw;
        g_batch_fill++;
        if (g_batch_fill >= BATCH_SAMPLES)
        {
            g_batch_ready = 1u;
        }
    }
}

/* isr_Timer: tick de sistema + watchdog UART. */
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
            saved = CyEnterCriticalSection();
            g_batch_fill    = 0u;
            g_batch_ready   = 0u;
            g_batches_sent  = 0u;
            g_dbg_cnt       = 0u;
            g_state         = PSOC_SAMPLING;
            CyExitCriticalSection(saved);
            ADC_StartConvert();
        }
    }
    else
    {
        ADC_StopConvert();
        saved = CyEnterCriticalSection();
        g_batch_fill  = 0u;
        g_batch_ready = 0u;
        g_state       = PSOC_IDLE;
        CyExitCriticalSection(saved);
    }
}

/* -------------------------------------------------------------------------- */

static void rx_watchdog_start(void)
{
    g_rx_watch_ticks = 0u;
    watchdog_rx      = 0u;
}

static void rx_watchdog_stop(void)
{
    g_rx_watch_ticks = 0u;
    watchdog_rx      = 0u;
}

static void led_toggle(void)
{
    if (g_state != PSOC_SAMPLING) {
        LED_Write(LED_Read() ^ 0x01u);
    }
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
    if (code <= 8u) { g_pga_code = code; PGAgain_SetGain(code); }
}

static void PGAvdac_Set(uint8 code)
{
    if (code <= 8u) { g_pgavdac_code = code; PGAvdac_SetGain(code); }
}

static void psoc_arm(void)
{
    uint8 saved;
    ADC_StopConvert();
    saved = CyEnterCriticalSection();
    g_batch_fill   = 0u;
    g_batch_ready  = 0u;
    g_batches_sent = 0u;
    g_dbg_cnt      = 0u;
    g_state        = PSOC_ARMED;
    CyExitCriticalSection(saved);
}

static void psoc_start_now(void)
{
    uint8 saved;
    ADC_StopConvert();
    saved = CyEnterCriticalSection();
    g_debug_psoc   = 0u;
    g_batch_fill   = 0u;
    g_batch_ready  = 0u;
    g_batches_sent = 0u;
    g_dbg_cnt      = 0u;
    g_state        = PSOC_SAMPLING;
    CyExitCriticalSection(saved);
    ADC_StartConvert();
}

static void uart_build_and_send(void)
{
    uint8  frame[FRAME_BYTES];
    uint8 *p;
    uint16 i;
    uint8  crc = 0u;
    int32  val;

    frame[0] = 0xABu;
    frame[1] = (uint8)BATCH_SAMPLES;
    frame[2] = (uint8)( g_seq        & 0xFFu);
    frame[3] = (uint8)((g_seq >> 8u) & 0xFFu);

    p = &frame[4];
    for (i = 0u; i < BATCH_SAMPLES; i++, p += 3u)
    {
        if (g_debug_psoc)
        {
            val = (int32)(g_dbg_cnt & 0x00FFFFFFu);
            g_dbg_cnt++;
        }
        else
        {
            val = g_batch_raw[i];
        }
        p[0] = (uint8)( val        & 0xFFu);
        p[1] = (uint8)((val >>  8u) & 0xFFu);
        p[2] = (uint8)((val >> 16u) & 0xFFu);
    }

    for (i = 0u; i < (FRAME_BYTES - 1u); i++) { crc ^= frame[i]; }
    frame[FRAME_BYTES - 1u] = crc;

    g_seq++;
    UART_PutArray(frame, FRAME_BYTES);

    /* Batch enviado -> renovar ventana de comunicacion para el LED */
    g_comm_countdown = COMM_WINDOW_TICKS;
}

/* Procesa todos los bytes pendientes en el buffer RX de la UART.
 * Llamado tanto desde el loop de arranque como desde el loop principal. */
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

                /* Cualquier comando válido del ESP = conexión establecida */
                g_esp_connected  = 1u;
                g_comm_countdown = COMM_WINDOW_TICKS;

                switch (rx_cmd)
                {
                    case PSOC_CMD_PONG:  /* pong: ya marcamos g_esp_connected arriba */
                        break;
                    case 0xA5u:
                        uart_send_ping();
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
                        g_n_batches = (uint16)rx_p1 | ((uint16)rx_p2 << 8u);
                        break;
                    case 0xB1u:
                        psoc_arm(); led_toggle(); break;
                    case 0xB4u:
                        psoc_start_now(); led_toggle(); break;
                    case 0xB3u:
                        g_debug_psoc = (rx_p1 == 0u) ? 0u : 1u;
                        g_dbg_cnt = 0u;
                        if (g_debug_psoc) {
                            g_batch_fill   = 0u;
                            g_batch_ready  = 0u;
                            g_batches_sent = 0u;
                            g_state        = PSOC_SAMPLING;
                            ADC_StartConvert();
                        } else {
                            ADC_StopConvert();
                            g_batch_fill  = 0u;
                            g_batch_ready = 0u;
                            g_state       = PSOC_IDLE;
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
    if (watchdog_rx)
    {
        watchdog_rx = 0u;
        rx_state    = 0u;
    }

    uart_service();

    if (g_batch_ready)
    {
        uart_build_and_send();
        g_batch_ready  = 0u;
        g_batch_fill   = 0u;
        g_batches_sent++;

        if (g_n_batches != 0u && g_batches_sent >= g_n_batches)
        {
            ADC_StopConvert();
            g_state = PSOC_IDLE;
        }
    }
}

static void service_comm_led(uint32 now)
{
    static uint32 lastTick = 0u;
    static uint32 blinkTicks = 0u;
    uint32 elapsed;

    if (g_state == PSOC_SAMPLING) {
        lastTick = now;   /* conservar g_comm_countdown para parpadear después. */
        return;
    }

    if (lastTick == 0u) {
        lastTick = now;
        return;
    }

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
        LED_Write(1u);
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
    uint32 nextPing = timer_now_ticks();
    uint32 pingStart = nextPing;

    LED_Write(1u);
    while (!g_esp_connected)
    {
        uint32 now;
        service_runtime();
        now = timer_now_ticks();

        if (ticks_due(now, nextPing)) {
            uart_send_ping();
            pingStart = now;
            nextPing = now + PING_PERIOD_TICKS;
            LED_Write(1u);
        }

        if ((now - pingStart) >= (PING_PERIOD_TICKS - PING_OFF_TICKS)) {
            LED_Write(0u);
        } else {
            LED_Write(1u);
        }
    }
}

/* -------------------------------------------------------------------------- */

int main(void)
{
    uint8  i;

    CyGlobalIntEnable;

    UART_Start();
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
    VDAC_Start();
    VDAC_SetValue(VDAC_INIT_VALUE);

    ADC_Start();
    ADC_StopConvert();

    isr_DelSig_StartEx(isr_DelSigReady);
    isr_Timer_StartEx(isr_Timer);
    isr_SyncIn_StartEx(isr_SyncIn);

    /* Timer: tick de sistema para watchdog, pings y LED; sin esperas bloqueantes. */
    Timer_Stop();
    Timer_WritePeriod(TIMEOUT_COUNTS);
    Timer_Start();

    /* ── Loop de arranque: busca el ESP sin bloquear UART/ADC ───────────── */
    wait_for_esp();

    /* ── Confirmación: 5 parpadeos rápidos al conectar ────────────────── */
    for (i = 0u; i < 5u; i++)
    {
        LED_Write(0u); wait_ticks(CONNECT_BLINK_TICKS);
        LED_Write(1u); wait_ticks(CONNECT_BLINK_TICKS);
    }

    /* ── Loop principal ─────────────────────────────────────────────────── */
    for (;;)
    {
        static uint32 idlePingDue = 0u;
        uint32 now;

        service_runtime();
        now = timer_now_ticks();

        /* Re-probe del ESP: si el ESP se resetea después de que el PSoC ya
         * salió del loop de arranque, sigue habiendo pings para que el ESP lo
         * detecte sin necesitar abrir el debug serial. No se envía durante
         * sampling para no intercalar bytes dentro del stream de datos. */
        if (g_state == PSOC_IDLE)
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
