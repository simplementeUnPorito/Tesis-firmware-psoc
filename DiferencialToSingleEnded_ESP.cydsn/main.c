/*******************************************************************************
* main.c — Geofono single-ended con transporte SPI esclavo hacia ESP32
*
* TX UART (5 bytes, se mantiene para debug/MATLAB serie):
*   Data:      [0x56][0x00][b2][b1][b0]
*   Heartbeat: [0x56][0x01][pga_code][vdac_val][tx_mode]
*   Cfg-ADC:   [0x56][0x02][res][fsH][fsL]
*   Cfg-PGA:   [0x56][0x03][pga_code][vrefH][vrefL]
*   Cfg-VREF:  [0x56][0x04][pgavdac_code][vdac_val][0x00]
*   ACK:       [0x56][0x07][cmd][val][0x00]
*
* RX UART — 4 bytes con checksum XOR: [0xAB][cmd][param][cmd^param]
*   (comandos idénticos a la versión original)
*
* SPI ESCLAVO (ESP32 = maestro):
*   Frame 306 bytes: [0xAB][n_lo][n_hi][seq_lo][seq_hi][flags]
*                    + 30 muestras × 10 bytes
*   Pin DATA_READY (salida): HIGH = frame listo, lo baja el PSoC tras SPI_DONE.
*
*   REQUISITOS EN PSOC CREATOR (TopDesign.cysch):
*     1. Agregar componente "SPI Slave (SPIS)" → nombre: SPIS_1
*        - Mode: Mode 00 (CPOL=0, CPHA=0)
*        - TX Buffer Size: 320 (o mayor)
*        - Interrupt: ninguno requerido (polling en main)
*     2. Agregar "Digital Output Pin" → nombre: DATA_READY
*        - Conectar al puerto físico elegido
*     3. Conectar pines del SPIS_1:
*        - SCLK ← ESP GPIO 18
*        - MISO → ESP GPIO 19
*        - MOSI ← ESP GPIO 23
*        - SS   ← ESP GPIO 5
*        - DATA_READY → ESP GPIO 4
*
* DEBUG:
*   #define DEBUG 1  ->  rampa creciente en lugar de ADC
*   #define DEBUG 0  ->  datos reales del ADC
*******************************************************************************/

#include "project.h"

/* -------------------------------------------------------------------------- */
#define DEBUG              0

#define HB_PERIOD          1000u
#define DEBUG_PERIOD_US    1000u
#define VDAC_INIT_VALUE    0x94u
#define TIMEOUT_COUNTS     240000u    /* 10 ms @ 24 MHz */

/* SPI batch */
#define SPI_BATCH_SIZE     30u
#define SPI_FRAME_BYTES    (6u + SPI_BATCH_SIZE * 10u)   /* 306 */

/* -------------------------------------------------------------------------- */
/* Muestras para UART (legado) */
static volatile int32 g_adc_raw      = 0;
static volatile uint8 g_sample_ready = 0u;
static volatile int32 g_flt          = 0;
static volatile uint8 g_filter_ready = 0u;

/* Batch SPI — doble buffer implícito: g_spi_fill se usa solo cuando
 * g_spi_ready == 0, por lo que el array no necesita protección adicional. */
static volatile int32  g_spi_raw [SPI_BATCH_SIZE];
static volatile int32  g_spi_flt [SPI_BATCH_SIZE];
static volatile uint8  g_spi_gain[SPI_BATCH_SIZE];
static volatile uint16 g_spi_fill  = 0u;
static volatile uint8  g_spi_ready = 0u;   /* batch completo, listo para SPI */
static          uint16 g_spi_seq   = 0u;

static uint8 g_pga_code     = PGAgain_DEFAULT_GAIN;
static uint8 g_pgavdac_code = PGAvdac_DEFAULT_GAIN;
static uint8 g_vdac_val     = VDAC_INIT_VALUE;
static uint8 tx_mode        = 0u;
static uint8 stream_enabled = 0u;

/* RX UART */
static volatile uint8 rx_state    = 0u;
static volatile uint8 rx_cmd      = 0u;
static volatile uint8 rx_param    = 0u;
static volatile uint8 watchdog_rx = 0u;

/* -------------------------------------------------------------------------- */

CY_ISR(isr_DelSigReady)
{
    g_adc_raw      = ADC_GetResult32();
    g_sample_ready = 1u;
    Filter_Write24(Filter_CHANNEL_A, (uint32)g_adc_raw);

    /* Acumular muestra en batch SPI */
    if (g_spi_fill < SPI_BATCH_SIZE && g_spi_ready == 0u)
    {
        g_spi_raw [g_spi_fill] = g_adc_raw;
        g_spi_flt [g_spi_fill] = g_flt;       /* último valor filtrado disponible */
        g_spi_gain[g_spi_fill] = g_pga_code;
        g_spi_fill++;
        if (g_spi_fill >= SPI_BATCH_SIZE)
        {
            g_spi_ready = 1u;                  /* señal para main loop */
        }
    }
}

CY_ISR(isr_FilterReady)
{
    uint32 r = Filter_Read24(Filter_CHANNEL_A);
    g_flt = r;
    g_filter_ready = 1u;
}

CY_ISR(isr_Timer)
{
    Timer_ReadStatusRegister();
    watchdog_rx = 1u;
}

/*
 * isr_SyncIn — flanco en el pin SYNC_IN del ESP esclavo.
 *
 * REQUIERE EN PSOC CREATOR (TopDesign):
 *   - Pin digital input "SYNC_IN" con interrupt "Rising and Falling edges"
 *   - ISR "isr_SyncIn" conectado al pin
 *
 * Flanco de subida (SYNC_IN=1): reinicia buffer y arranca el ADC.
 * Flanco de bajada (SYNC_IN=0): para el ADC y limpia el buffer.
 */
CY_ISR(isr_SyncIn)
{
    uint8 saved;
    if (SYNC_IN_Read())     /* flanco de subida → START */
    {
        saved = CyEnterCriticalSection();
        g_spi_fill  = 0u;
        g_spi_ready = 0u;
        CyExitCriticalSection(saved);
        ADC_StartConvert();
    }
    else                    /* flanco de bajada → STOP */
    {
        ADC_StopConvert();
        saved = CyEnterCriticalSection();
        g_spi_fill  = 0u;
        g_spi_ready = 0u;
        CyExitCriticalSection(saved);
        DATA_READY_Write(0u);
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

static void send_config(void)
{
    uint8  cfg[5u];
    uint16 fs_rep      = (uint16)ADC_CFG1_SRATE;
    uint16 vref_halfmv = (uint16)((float32)ADC_CFG1_INPUT_RANGE_VALUE * 1000.0f);

    cfg[0] = 0x56u; cfg[1] = 0x02u;
    cfg[2] = (uint8)ADC_CFG1_RESOLUTION;
    cfg[3] = (uint8)((fs_rep      >> 8u) & 0xFFu);
    cfg[4] = (uint8)( fs_rep             & 0xFFu);
    UART_PC_PutArray(cfg, 5u);

    cfg[1] = 0x03u;
    cfg[2] = g_pga_code;
    cfg[3] = (uint8)((vref_halfmv >> 8u) & 0xFFu);
    cfg[4] = (uint8)( vref_halfmv        & 0xFFu);
    UART_PC_PutArray(cfg, 5u);

    cfg[1] = 0x04u;
    cfg[2] = g_pgavdac_code;
    cfg[3] = g_vdac_val;
    cfg[4] = 0x00u;
    UART_PC_PutArray(cfg, 5u);
}

static void send_ack(uint8 cmd, uint8 val)
{
    uint8 ack[5u] = {0x56u, 0x07u, cmd, val, 0x00u};
    UART_PC_PutArray(ack, 5u);
}

static void PGAgain_Set(uint8 code)
{
    if (code <= 8u) { g_pga_code = code; PGAgain_SetGain(code); }
}

static void PGAvdac_Set(uint8 code)
{
    if (code <= 8u) { g_pgavdac_code = code; PGAvdac_SetGain(code); }
}

/* -------------------------------------------------------------------------- */

/* Construye el frame SPI de 306 bytes y lo carga en el buffer TX del SPIS_1.
 * Llamar solo desde main loop cuando g_spi_ready == 1. */
static void spi_build_and_send(void)
{
    uint8  frame[SPI_FRAME_BYTES];
    uint8 *p;
    uint16 i;
    int32  raw, flt;
    uint8  gain;
    uint8  saved;

    /* Copiar batch con sección crítica mínima (solo el flag de control) */
    saved = CyEnterCriticalSection();
    /* Los arrays g_spi_* no se tocan por ISR mientras g_spi_ready == 1 */
    CyExitCriticalSection(saved);

    frame[0] = 0xABu;
    frame[1] = (uint8)( SPI_BATCH_SIZE         & 0xFFu);
    frame[2] = (uint8)((SPI_BATCH_SIZE >> 8u)  & 0xFFu);
    frame[3] = (uint8)( g_spi_seq              & 0xFFu);
    frame[4] = (uint8)((g_spi_seq  >> 8u)      & 0xFFu);
    frame[5] = tx_mode;

    p = &frame[6];
    for (i = 0u; i < SPI_BATCH_SIZE; i++, p += 10u)
    {
        raw  = (int32)g_spi_raw [i];
        flt  = (int32)g_spi_flt [i];
        gain = g_spi_gain[i];

        /* raw_input (16-bit LE) */
        p[0] = (uint8)( raw        & 0xFFu);
        p[1] = (uint8)((raw >> 8u) & 0xFFu);
        /* post_analog (24-bit LE) — valor ADC crudo */
        p[2] = (uint8)( raw          & 0xFFu);
        p[3] = (uint8)((raw >>  8u)  & 0xFFu);
        p[4] = (uint8)((raw >> 16u)  & 0xFFu);
        /* post_digital (24-bit LE) — filtrado */
        p[5] = (uint8)( flt          & 0xFFu);
        p[6] = (uint8)((flt >>  8u)  & 0xFFu);
        p[7] = (uint8)((flt >> 16u)  & 0xFFu);
        p[8] = gain;
        p[9] = 0x00u;
    }

    g_spi_seq++;

    SPIS_1_ClearTxBuffer();
    SPIS_1_PutArray(frame, SPI_FRAME_BYTES);
    DATA_READY_Write(1u);
}

/* -------------------------------------------------------------------------- */

int main(void)
{
    static uint16 hb_count = 0u;
#if DEBUG
    static uint32 dbg_cnt  = 0u;
#endif

    uint8 pkt[5u];
#if !DEBUG
    int32 raw;
#endif
    uint8 saved, rx;

    CyGlobalIntEnable;

    /* Analógica — sin cambios */
    UART_PC_Start();
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
    Filter_Start();
    Filter_SetCoherency(Filter_CHANNEL_A, Filter_KEY_LOW);
    Filter_SetDalign(Filter_STAGEA_DALIGN, 0u);
    Filter_SetDalign(Filter_HOLDA_DALIGN,  0u);
    ADC_Start();
    ADC_StartConvert();
    isr_DelSig_StartEx(isr_DelSigReady);
    isr_Filter_StartEx(isr_FilterReady);
    isr_Timer_StartEx(isr_Timer);

    /* SPI esclavo */
    SPIS_1_Start();
    DATA_READY_Write(0u);

    /* GPIO hardware sync — arranque/paro del ADC por pulso externo */
    isr_SyncIn_StartEx(isr_SyncIn);

    LED_Write(0u);
    CyDelay(10u);
    send_config();

    for (;;)
    {
        /* ==== Watchdog RX UART ============================================ */
        if (watchdog_rx)
        {
            watchdog_rx = 0u;
            rx_state    = 0u;
            rx_cmd      = 0u;
            rx_param    = 0u;
        }

        /* ==== RX UART — protocolo 4 bytes ================================= */
        while (UART_PC_GetRxBufferSize() > 0u)
        {
            rx = UART_PC_ReadRxData();
            switch (rx_state)
            {
                case 0u:
                    if (rx == 0xABu) { rx_state = 1u; rx_watchdog_start(); }
                    break;
                case 1u:
                    rx_watchdog_start();
                    switch (rx)
                    {
                        case 0xA5u: case 0xA1u: case 0xA6u:
                        case 0xA8u: case 0xA9u: case 0xAAu:
                            rx_cmd = rx; rx_state = 2u; break;
                        default:
                            rx_watchdog_stop(); rx_state = 0u; break;
                    }
                    break;
                case 2u:
                    rx_param = rx; rx_state = 3u; rx_watchdog_start(); break;
                case 3u:
                    rx_watchdog_stop(); rx_state = 0u;
                    if (rx != (uint8)(rx_cmd ^ rx_param)) { break; }
                    switch (rx_cmd)
                    {
                        case 0xA5u: send_config(); break;
                        case 0xA1u:
                            stream_enabled = (rx_param == 0u) ? 0u : 1u;
#if DEBUG
                            if (stream_enabled) { dbg_cnt = 0u; }
#endif
                            send_ack(0xA1u, stream_enabled);
                            if (stream_enabled) { send_config(); }
                            led_toggle(); break;
                        case 0xA6u:
                            PGAgain_Set(rx_param); send_config(); led_toggle(); break;
                        case 0xA8u:
                            tx_mode = (rx_param == 0u) ? 0u : 1u;
                            send_ack(0xA8u, tx_mode); break;
                        case 0xA9u:
                            PGAvdac_Set(rx_param);
                            send_ack(0xA9u, g_pgavdac_code); send_config(); led_toggle(); break;
                        case 0xAAu:
                            VDAC_SetValue(rx_param); g_vdac_val = rx_param;
                            send_ack(0xAAu, rx_param); send_config(); led_toggle(); break;
                        default: break;
                    }
                    break;
                default:
                    rx_state = 0u; break;
            }
        }

        /* ==== SPI: enviar batch cuando está listo ========================= */
        if (g_spi_ready && DATA_READY_Read() == 0u)
        {
            spi_build_and_send();
        }

        /* Bajar DATA_READY cuando el ESP terminó de leer (SPI_DONE) */
        if (DATA_READY_Read())
        {
            uint8 tx_status = SPIS_1_ReadTxStatus();
            if (tx_status & SPIS_1_STS_SPI_DONE)
            {
                DATA_READY_Write(0u);
                saved = CyEnterCriticalSection();
                g_spi_ready = 0u;
                g_spi_fill  = 0u;    /* habilitar acumulación del siguiente batch */
                CyExitCriticalSection(saved);
            }
        }

#if DEBUG
        /* ==== DEBUG: rampa UART =========================================== */
        if (stream_enabled)
        {
            pkt[0]=0x56u; pkt[1]=0x00u;
            pkt[2]=(uint8)((dbg_cnt>>16u)&0xFFu);
            pkt[3]=(uint8)((dbg_cnt>> 8u)&0xFFu);
            pkt[4]=(uint8)( dbg_cnt       &0xFFu);
            dbg_cnt = (dbg_cnt + 1u) & 0x00FFFFFFu;
            UART_PC_PutArray(pkt, 5u);
            if (++hb_count >= HB_PERIOD)
            {
                hb_count=0u;
                pkt[0]=0x56u; pkt[1]=0x01u;
                pkt[2]=g_pga_code; pkt[3]=g_vdac_val; pkt[4]=tx_mode;
                UART_PC_PutArray(pkt, 5u);
            }
            CyDelayUs(DEBUG_PERIOD_US);
        }
#else
        /* ==== TX UART crudo (tx_mode == 0) ================================ */
        if (stream_enabled && tx_mode == 0u && g_sample_ready)
        {
            saved = CyEnterCriticalSection();
            raw            = g_adc_raw;
            g_sample_ready = 0u;
            CyExitCriticalSection(saved);

            pkt[0]=0x56u; pkt[1]=0x00u;
            pkt[2]=(uint8)((raw>>16u)&0xFFu);
            pkt[3]=(uint8)((raw>> 8u)&0xFFu);
            pkt[4]=(uint8)( raw      &0xFFu);
            UART_PC_PutArray(pkt, 5u);

            if (++hb_count >= HB_PERIOD)
            {
                hb_count=0u;
                pkt[0]=0x56u; pkt[1]=0x01u;
                pkt[2]=g_pga_code; pkt[3]=g_vdac_val; pkt[4]=tx_mode;
                UART_PC_PutArray(pkt, 5u);
            }
        }

        /* ==== TX UART filtrado (tx_mode == 1) ============================= */
        if (stream_enabled && tx_mode == 1u && g_filter_ready)
        {
            saved = CyEnterCriticalSection();
            raw            = g_flt;
            g_filter_ready = 0u;
            CyExitCriticalSection(saved);

            pkt[0]=0x56u; pkt[1]=0x00u;
            pkt[2]=(uint8)((raw>>16u)&0xFFu);
            pkt[3]=(uint8)((raw>> 8u)&0xFFu);
            pkt[4]=(uint8)( raw      &0xFFu);
            UART_PC_PutArray(pkt, 5u);

            if (++hb_count >= HB_PERIOD)
            {
                hb_count=0u;
                pkt[0]=0x56u; pkt[1]=0x01u;
                pkt[2]=g_pga_code; pkt[3]=g_vdac_val; pkt[4]=tx_mode;
                UART_PC_PutArray(pkt, 5u);
            }
        }
#endif
    }
    return 0u;
}

/* [] END OF FILE */
