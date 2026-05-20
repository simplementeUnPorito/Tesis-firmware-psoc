/*******************************************************************************
* main.c — Geofono single-ended
*
* TX (5 bytes):
*   Data:      [0x56][0x00][b2][b1][b0]
*   Heartbeat: [0x56][0x01][pga_code][vdac_val][tx_mode]
*   Cfg-ADC:   [0x56][0x02][res][fsH][fsL]
*   Cfg-PGA:   [0x56][0x03][pga_code][vrefH][vrefL]
*   Cfg-VREF:  [0x56][0x04][pgavdac_code][vdac_val][0x00]
*   ACK:       [0x56][0x07][cmd][val][0x00]
*
* RX — 4 bytes con checksum XOR: [0xAB][cmd][param][cmd^param]
*   0xAB 0xA5 0x00 0xA5  -> send_config()
*   0xAB 0xA1 <en>   <cs>-> stream_enabled (0=off,1=on)
*   0xAB 0xA6 <code> <cs>-> PGAgain_SetGain(code)   LED toggle
*   0xAB 0xA8 <mode> <cs>-> tx_mode (0=crudo,1=flt)
*   0xAB 0xA9 <code> <cs>-> PGAvdac_SetGain(code)   LED toggle
*   0xAB 0xAA <v>    <cs>-> VDAC_SetValue(v)         LED toggle
*
* DEBUG:
*   #define DEBUG 1  ->  rampa creciente en lugar de ADC
*   #define DEBUG 0  ->  datos reales del ADC
*******************************************************************************/

#include "project.h"

/* -------------------------------------------------------------------------- */
#define DEBUG              0

/* ADC_CFG1_SRATE y ADC_CFG1_RESOLUTION vienen de ADC.h (generado).
 * ADC_CFG1_INPUT_RANGE_VALUE = 2.5 (float) → half-rango en mV = 2500.
 * Se calcula en send_config() para no repetir el literal. */
#define HB_PERIOD          1000u      /* heartbeat cada N muestras (~1 s a 1000 SPS) */
#define DEBUG_PERIOD_US    1000u      /* DEBUG=1: rampa UART independiente del ADC */
#define VDAC_INIT_VALUE    0x94u
#define TIMEOUT_COUNTS     240000u    /* 10 ms @ 24 MHz */

/* -------------------------------------------------------------------------- */
static volatile int32 g_adc_raw      = 0;
static volatile uint8 g_sample_ready = 0u;
static volatile int32 g_flt          = 0;
static volatile uint8 g_filter_ready = 0u;

static uint8 g_pga_code = PGAgain_DEFAULT_GAIN;
static uint8 g_pgavdac_code = PGAvdac_DEFAULT_GAIN;
static uint8 g_vdac_val = VDAC_INIT_VALUE;
static uint8 tx_mode    = 0u;
static uint8 stream_enabled = 0u;

/*
 * Protocolo RX — 4 estados:
 *   0: esperar 0xAB
 *   1: esperar cmd  (0xA1 / 0xA5 / 0xA6 / 0xA8 / 0xA9 / 0xAA)
 *   2: esperar param
 *   3: esperar checksum (cmd ^ param) — si valido, ejecutar
 *
 * Fix watchdog: rx_watchdog_start/stop llaman isr_Timer_ClearPending()
 * para evitar que una interrupcion pendiente reinicie rx_state justo
 * despues de limpiar watchdog_rx, borrando el contexto del comando.
 */
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
}

CY_ISR(isr_FilterReady)
{
    uint32 r = Filter_Read24(Filter_CHANNEL_A);
    g_flt = (r & 0x800000u) ? (int32)(r | 0xFF000000u) : (int32)r;
    g_filter_ready = 1u;
}

CY_ISR(isr_Timer)
{
    Timer_ReadStatusRegister();
    watchdog_rx = 1u;
}

/* -------------------------------------------------------------------------- */

static void rx_watchdog_start(void)
{
    Timer_Stop();
    Timer_ReadStatusRegister();    /* limpiar flag de overflow */
    isr_Timer_ClearPending();      /* limpiar pending en NVIC — evita falsa disparo */
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
    uint16 fs_rep       = (uint16)ADC_CFG1_SRATE;
    /* Derivado de ADC_CFG1_INPUT_RANGE_VALUE (float, ej. 2.5) → 2500 mV */
    uint16 vref_halfmv  = (uint16)((float32)ADC_CFG1_INPUT_RANGE_VALUE * 1000.0f);

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
    if (code <= 8u)
    {
        g_pga_code = code;
        PGAgain_SetGain(code);
    }
}

static void PGAvdac_Set(uint8 code)
{
    if (code <= 8u)
    {
        g_pgavdac_code = code;
        PGAvdac_SetGain(code);
    }
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
    LED_Write(0u);

    CyDelay(10u);
    send_config();

    for (;;)
    {
        /* ==== Watchdog RX ================================================ */
        if (watchdog_rx)
        {
            watchdog_rx = 0u;
            rx_state    = 0u;
            rx_cmd      = 0u;
            rx_param    = 0u;
        }

        /* ==== RX — protocolo 4 bytes con checksum ======================== */
        while (UART_PC_GetRxBufferSize() > 0u)
        {
            rx = UART_PC_ReadRxData();

            switch (rx_state)
            {
                case 0u:    /* esperar guard 0xAB */
                    if (rx == 0xABu)
                    {
                        rx_state = 1u;
                        rx_watchdog_start();
                    }
                    break;

                case 1u:    /* esperar comando */
                    rx_watchdog_start();
                    switch (rx)
                    {
                        case 0xA5u:
                        case 0xA1u:
                        case 0xA6u:
                        case 0xA8u:
                        case 0xA9u:
                        case 0xAAu:
                            rx_cmd   = rx;
                            rx_state = 2u;
                            break;
                        default:
                            rx_watchdog_stop();
                            rx_state = 0u;
                            break;
                    }
                    break;

                case 2u:    /* esperar parametro */
                    rx_param = rx;
                    rx_state = 3u;
                    rx_watchdog_start();
                    break;

                case 3u:    /* esperar checksum — validar y ejecutar */
                    rx_watchdog_stop();
                    rx_state = 0u;

                    if (rx != (uint8)(rx_cmd ^ rx_param)) { break; }

                    switch (rx_cmd)
                    {
                        case 0xA5u:
                            send_config();
                            break;

                        case 0xA1u:
                            stream_enabled = (rx_param == 0u) ? 0u : 1u;
#if DEBUG
                            if (stream_enabled != 0u)
                            {
                                dbg_cnt = 0u;
                            }
#endif
                            send_ack(0xA1u, stream_enabled);
                            if (stream_enabled != 0u)
                            {
                                send_config();
                            }
                            led_toggle();
                            break;

                        case 0xA6u:
                            PGAgain_Set(rx_param);
                            send_config();
                            led_toggle();
                            break;

                        case 0xA8u:
                            tx_mode = (rx_param == 0u) ? 0u : 1u;
                            send_ack(0xA8u, tx_mode);
                            break;

                        case 0xA9u:
                            PGAvdac_Set(rx_param);
                            send_ack(0xA9u, g_pgavdac_code);
                            send_config();
                            led_toggle();
                            break;

                        case 0xAAu:
                            VDAC_SetValue(rx_param);
                            g_vdac_val = rx_param;
                            send_ack(0xAAu, rx_param);
                            send_config();
                            led_toggle();
                            break;

                        default:
                            break;
                    }
                    break;

                default:
                    rx_state = 0u;
                    break;
            }
        }

#if DEBUG
        /* ==== DEBUG: rampa UART autosuficiente ============================ */
        if (stream_enabled != 0u)
        {
            pkt[0] = 0x56u; pkt[1] = 0x00u;
            pkt[2] = (uint8)((dbg_cnt >> 16u) & 0xFFu);
            pkt[3] = (uint8)((dbg_cnt >>  8u) & 0xFFu);
            pkt[4] = (uint8)( dbg_cnt          & 0xFFu);
            dbg_cnt = (dbg_cnt + 1u) & 0x00FFFFFFu;
            UART_PC_PutArray(pkt, 5u);

            if (++hb_count >= HB_PERIOD)
            {
                hb_count = 0u;
                pkt[0]=0x56u; pkt[1]=0x01u;
                pkt[2]=g_pga_code; pkt[3]=g_vdac_val; pkt[4]=tx_mode;
                UART_PC_PutArray(pkt, 5u);
            }

            CyDelayUs(DEBUG_PERIOD_US);
        }
#else
        /* ==== TX crudo (tx_mode == 0) ==================================== */
        if (stream_enabled != 0u && tx_mode == 0u && g_sample_ready)
        {
            saved = CyEnterCriticalSection();
            raw            = g_adc_raw;
            g_sample_ready = 0u;
            CyExitCriticalSection(saved);

            pkt[0] = 0x56u; pkt[1] = 0x00u;
            pkt[2] = (uint8)((raw >> 16u) & 0xFFu);
            pkt[3] = (uint8)((raw >>  8u) & 0xFFu);
            pkt[4] = (uint8)( raw         & 0xFFu);
            UART_PC_PutArray(pkt, 5u);

            if (++hb_count >= HB_PERIOD)
            {
                hb_count = 0u;
                pkt[0]=0x56u; pkt[1]=0x01u;
                pkt[2]=g_pga_code; pkt[3]=g_vdac_val; pkt[4]=tx_mode;
                UART_PC_PutArray(pkt, 5u);
            }
        }

        /* ==== TX filtrado (tx_mode == 1) ================================= */
        if (stream_enabled != 0u && tx_mode == 1u && g_filter_ready)
        {
            saved = CyEnterCriticalSection();
            raw            = g_flt;
            g_filter_ready = 0u;
            CyExitCriticalSection(saved);

            pkt[0] = 0x56u; pkt[1] = 0x00u;
            pkt[2] = (uint8)((raw >> 16u) & 0xFFu);
            pkt[3] = (uint8)((raw >>  8u) & 0xFFu);
            pkt[4] = (uint8)( raw         & 0xFFu);
            UART_PC_PutArray(pkt, 5u);

            if (++hb_count >= HB_PERIOD)
            {
                hb_count = 0u;
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
