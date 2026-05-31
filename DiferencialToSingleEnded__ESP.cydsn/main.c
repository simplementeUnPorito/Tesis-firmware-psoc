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
*       Baud sugerido: 460800 u 921600 (debe coincidir con PSOC_UART_BAUD del
*       firmware del esclavo). El batch es de 95 bytes a ~34 lotes/s.
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
*       0xA5 enviar config (no-op, reservado)   0xA6 set PGAgain (0-8)
*       0xA9 set PGAvdac (0-8)                   0xAA set VDAC (0-255)
*       0xB1 pre-start/arm (espera flanco SYNC)  0xB3 debug PSoC rampa (0/1)
*   2 parámetros: [0xAB][0xA3][n_lo][n_hi][0xA3^n_lo^n_hi]
*       0xA3 set N (lotes, 16 bits)
*******************************************************************************/

#include "project.h"

/* -------------------------------------------------------------------------- */
#define VDAC_INIT_VALUE    0x94u
#define TIMEOUT_COUNTS     240000u    /* 10 ms @ 24 MHz */

#define BATCH_SAMPLES      30u
#define FRAME_BYTES        (4u + BATCH_SAMPLES * 3u + 1u)   /* 95 */

/* Estados del PSoC */
#define PSOC_IDLE          0u
#define PSOC_ARMED         1u   /* armado: espera flanco SYNC para muestrear */
#define PSOC_SAMPLING      2u

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
static          uint16 g_n_batches    = 0u;     /* lotes a capturar (16 bits) */
static volatile uint16 g_batches_sent = 0u;
static volatile uint8  g_debug_psoc   = 0u;     /* 1 = rampa en vez de ADC */
static          uint32 g_dbg_cnt      = 0u;

/* RX UART */
static volatile uint8 rx_state    = 0u;
static volatile uint8 rx_cmd      = 0u;
static volatile uint8 rx_p1       = 0u;
static volatile uint8 rx_p2       = 0u;
static volatile uint8 watchdog_rx = 0u;

/* -------------------------------------------------------------------------- */

CY_ISR(isr_DelSigReady)
{
    g_adc_raw = ADC_GetResult32();

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

CY_ISR(isr_Timer)
{
    Timer_ReadStatusRegister();
    watchdog_rx = 1u;
}

/*
 * isr_SyncIn — flanco en el pin SYNC_IN (lo maneja el ESP esclavo).
 *   Subida  : si está ARMED -> arranca muestreo (SAMPLING).
 *   Bajada  : para el muestreo.
 */
CY_ISR(isr_SyncIn)
{
    uint8 saved;
    if (SYNC_IN_Read())     /* flanco de subida → START */
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
    else                    /* flanco de bajada → STOP */
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
    Timer_Stop();
    Timer_ReadStatusRegister();
    isr_Timer_ClearPending();
    Timer_WritePeriod(TIMEOUT_COUNTS);
    watchdog_rx = 0u;
    Timer_Start();
}

static void rx_watchdog_stop(void)
{
    Timer_Stop();
    Timer_ReadStatusRegister();
    isr_Timer_ClearPending();
    watchdog_rx = 0u;
}

static void led_toggle(void)
{
    LED_Write(LED_Read() ^ 0x01u);
}

static void PGAgain_Set(uint8 code)
{
    if (code <= 8u) { g_pga_code = code; PGAgain_SetGain(code); }
}

static void PGAvdac_Set(uint8 code)
{
    if (code <= 8u) { g_pgavdac_code = code; PGAvdac_SetGain(code); }
}

/* Arma el PSoC para que un flanco SYNC dispare la captura de N lotes. */
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

/* Construye y envía el frame UART de 95 bytes (raw, 30 muestras). */
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

    for (i = 0u; i < (FRAME_BYTES - 1u); i++)
    {
        crc ^= frame[i];
    }
    frame[FRAME_BYTES - 1u] = crc;

    g_seq++;
    UART_PutArray(frame, FRAME_BYTES);
}

/* -------------------------------------------------------------------------- */

int main(void)
{
    uint8 rx;

    CyGlobalIntEnable;

    /* Analógica — sin cambios (sin Filter) */
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
    /* ADC en reposo: el muestreo arranca por SYNC / capture-now */
    ADC_StopConvert();

    isr_DelSig_StartEx(isr_DelSigReady);
    isr_Timer_StartEx(isr_Timer);
    isr_SyncIn_StartEx(isr_SyncIn);

    LED_Write(0u);

    for (;;)
    {
        /* ==== Watchdog RX UART ============================================ */
        if (watchdog_rx)
        {
            watchdog_rx = 0u;
            rx_state    = 0u;
        }

        /* ==== RX UART — comandos (1 o 2 parámetros) ====================== */
        while (UART_GetRxBufferSize() > 0u)
        {
            rx = UART_ReadRxData();
            switch (rx_state)
            {
                case 0u:
                    if (rx == 0xABu) { rx_state = 1u; rx_watchdog_start(); }
                    break;

                case 1u:    /* comando */
                    rx_watchdog_start();
                    switch (rx)
                    {
                        case 0xA5u: case 0xA6u: case 0xA9u: case 0xAAu:
                        case 0xB1u: case 0xB3u:
                            rx_cmd = rx; rx_state = 2u; break;     /* 1 parámetro */
                        case 0xA3u:
                            rx_cmd = rx; rx_state = 4u; break;     /* 2 parámetros */
                        default:
                            rx_watchdog_stop(); rx_state = 0u; break;
                    }
                    break;

                case 2u:    /* param único → luego checksum */
                    rx_p1 = rx; rx_p2 = 0u; rx_state = 3u; rx_watchdog_start();
                    break;

                case 4u:    /* primer param de comando de 2 */
                    rx_p1 = rx; rx_state = 5u; rx_watchdog_start();
                    break;

                case 5u:    /* segundo param de comando de 2 */
                    rx_p2 = rx; rx_state = 3u; rx_watchdog_start();
                    break;

                case 3u:    /* checksum */
                    rx_watchdog_stop(); rx_state = 0u;
                    if (rx != (uint8)(rx_cmd ^ rx_p1 ^ rx_p2)) { break; }
                    switch (rx_cmd)
                    {
                        case 0xA5u: /* enviar config — reservado, no-op */
                            break;
                        case 0xA6u:
                            PGAgain_Set(rx_p1); led_toggle(); break;
                        case 0xA9u:
                            PGAvdac_Set(rx_p1); led_toggle(); break;
                        case 0xAAu:
                            VDAC_SetValue(rx_p1); g_vdac_val = rx_p1; led_toggle(); break;
                        case 0xA3u: /* set N (16 bits) */
                            g_n_batches = (uint16)rx_p1 | ((uint16)rx_p2 << 8u);
                            break;
                        case 0xB1u: /* pre-start / arm (arranque real por flanco SYNC) */
                            psoc_arm(); led_toggle(); break;
                        case 0xB3u: /* debug PSoC rampa */
                            g_debug_psoc = (rx_p1 == 0u) ? 0u : 1u;
                            g_dbg_cnt = 0u; break;
                        default: break;
                    }
                    break;

                default:
                    rx_state = 0u; break;
            }
        }

        /* ==== TX UART: enviar lote completo ============================== */
        if (g_batch_ready)
        {
            uart_build_and_send();

            g_batch_ready = 0u;
            g_batch_fill  = 0u;
            g_batches_sent++;

            /* Parada autónoma al alcanzar N lotes */
            if (g_n_batches != 0u && g_batches_sent >= g_n_batches)
            {
                ADC_StopConvert();
                g_state = PSOC_IDLE;
            }
        }
    }
}

/* [] END OF FILE */
