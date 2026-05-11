/*******************************************************************************
* main.c — Geofono diferencial: ADC_DelSig -> Filter DFB -> UART
*
* Protocolo TX (5 bytes por paquete):
*   Data:    [0x56][0x00][b2][b1][b0]    int24 big-endian, DC removido por IIR
*   HB:      [0x56][0x01][0][0][0]
*   Cfg-ADC: [0x56][0x02][res][fsH][fsL]
*   Cfg-PGA: [0x56][0x03][code][vH][vL]
*   VRef-St: [0x56][0x04][vdac_p][vdac_n][0x01]
*   VRef-Cfg:[0x56][0x06][pVRef][nVRef][0x00]
*
* Comandos RX:
*   0xA5            → reenvía config completa
*   0xA6 <code>     → PGA_SetGain(code)
*   0xAA <pV> <nV>  → setea VDAC_p = pV, VDAC_n = nV de inmediato
*
* Los VDACs se setean directamente — sin servo, sin búsqueda, sin locking.
* El usuario elige los valores óptimos observando el osciloscopio y el promedio
* DC que MATLAB muestra en tiempo real.
*******************************************************************************/

#include "project.h"
#include "coeficientes_notch.h"

#define ADC_VREF_HALFMV 6144u

/* ── Tamaño de paquete ───────────────────────────────────────────────────── */
#if   ADC_DelSig_CFG1_RESOLUTION <= 8u
  #define PKT_DATA_BYTES 1u
#elif ADC_DelSig_CFG1_RESOLUTION <= 16u
  #define PKT_DATA_BYTES 2u
#else
  #define PKT_DATA_BYTES 3u
#endif
#define PKT_TOTAL_BYTES (2u + PKT_DATA_BYTES)
#define HB_PERIOD 3000u
#define DC_SHIFT  14u
#define DEBUG     0

/* ── Referencias VDAC ───────────────────────────────────────────────────── */
static uint8 vdac_p_ref = 0x94u;   /* pVRef: referencia TIA_p */
static uint8 vdac_n_ref = 0x94u;   /* nVRef: referencia TIA_n */

/* ── Globals ISR → main ─────────────────────────────────────────────────── */
static volatile int32 g_flt          = 0;
static volatile uint8 g_filter_ready = 0u;
static volatile int32 ds             = 0;
static volatile uint8 g_pga_code     = PGA_n_GAIN_01;

CY_ISR(isr_DelSigReady)
{
    ds = ADC_DelSig_GetResult32();
    Filter_Write24(Filter_CHANNEL_A, (uint32)ds);
}

CY_ISR(isr_FilterReady)
{
    uint32 raw = Filter_Read24(Filter_CHANNEL_A);
    g_flt = (raw & 0x800000u) ? (int32)(raw | 0xFF000000u) : (int32)raw;
    g_filter_ready = 1u;
}

static int32 fir_notch(int32 input)
{
    static int32  fir_buf[FILTER_ORDER + 1];
    static uint16 fir_head = 0u;
    double acc = 0.0;
    uint16 i, idx;
    fir_buf[fir_head] = input;
    idx = fir_head;
    for (i = 0u; i <= (uint16)FILTER_ORDER; i++)
    {
        acc += (double)fir_buf[idx] * filter_coeffs[i];
        idx = (idx == 0u) ? (uint16)FILTER_ORDER : (idx - 1u);
    }
    fir_head = (fir_head >= (uint16)FILTER_ORDER) ? 0u : (fir_head + 1u);
    return (int32)acc;
}

static long long dc_iir = 0;

/* ── Paquetes TX ─────────────────────────────────────────────────────────── */
static void send_vref_status(void)
{
    uint8 pkt[5u];
    pkt[0]=0x56u; pkt[1]=0x04u;
    pkt[2]=vdac_p_ref; pkt[3]=vdac_n_ref; pkt[4]=0x01u;
    UART_PC_PutArray(pkt, 5u);
}

static void send_vref_cfg(void)
{
    uint8 pkt[5u];
    pkt[0]=0x56u; pkt[1]=0x06u;
    pkt[2]=vdac_p_ref; pkt[3]=vdac_n_ref; pkt[4]=0x00u;
    UART_PC_PutArray(pkt, 5u);
}

static void send_config(void)
{
    uint8 cfg[5u];
    cfg[0]=0x56u; cfg[1]=0x02u;
    cfg[2]=(uint8)ADC_DelSig_CFG1_RESOLUTION;
    cfg[3]=(uint8)((ADC_DelSig_CFG1_SRATE>>8u)&0xFFu);
    cfg[4]=(uint8)( ADC_DelSig_CFG1_SRATE     &0xFFu);
    UART_PC_PutArray(cfg, 5u);

    cfg[1]=0x03u; cfg[2]=g_pga_code;
    cfg[3]=(uint8)((ADC_VREF_HALFMV>>8u)&0xFFu);
    cfg[4]=(uint8)( ADC_VREF_HALFMV     &0xFFu);
    UART_PC_PutArray(cfg, 5u);

    send_vref_status();
    send_vref_cfg();
}

/* ── Main ───────────────────────────────────────────────────────────────── */
int main(void)
{
    static uint16 hb_count  = 0u;
    static uint8  rx_state  = 0u;
    static uint8  rx_cmd    = 0u;
    static uint8  rx_bcount = 0u;
    static uint8  rx_buf[2u];

    uint8  pkt[5u];
    int32  flt, raw;
    uint8  saved, rx;

    CyGlobalIntEnable;

    UART_PC_Start();
    Opa_ref_Start();
    PGA_p_Start(); PGA_n_Start();
    PGA_p_SetGain(g_pga_code); PGA_n_SetGain(g_pga_code);
    OpaBP_p_Start(); OpaBP_n_Start();
    TIA_p_Start();  TIA_n_Start();

    VDAC_p_Start(); VDAC_n_Start();
    VDAC_p_SetValue(vdac_p_ref);
    VDAC_n_SetValue(vdac_n_ref);

    Filter_Start();
    Filter_SetCoherency(Filter_CHANNEL_A, Filter_KEY_LOW);
    Filter_SetDalign(Filter_STAGEA_DALIGN, 0u);
    Filter_SetDalign(Filter_HOLDA_DALIGN,  0u);

    ADC_DelSig_Start();
    ADC_DelSig_StartConvert();
    isr_DelSig_StartEx(isr_DelSigReady);
    isr_Filter_StartEx(isr_FilterReady);

    CyDelay(10u);
    send_config();

    for(;;)
    {
        /* ── Comandos UART ────────────────────────────────────────────────── */
        while (UART_PC_GetRxBufferSize() > 0u)
        {
            rx = UART_PC_ReadRxData();
            if (rx_state == 0u)
            {
                switch (rx)
                {
                    case 0xA5u: send_config(); break;
                    case 0xA6u: case 0xAAu:
                        rx_cmd=rx; rx_bcount=0u; rx_state=1u; break;
                    default: break;
                }
            }
            else
            {
                rx_buf[rx_bcount]=rx; rx_bcount++;
                switch (rx_cmd)
                {
                    case 0xA6u:   /* 1 byte: PGA code */
                        if (rx_bcount>=1u)
                        {
                            if (rx_buf[0u]<=8u)
                            {
                                g_pga_code=rx_buf[0u];
                                PGA_p_SetGain(rx_buf[0u]);
                                PGA_n_SetGain(rx_buf[0u]);
                                send_config();
                            }
                            rx_state=0u;
                        }
                        break;

                    case 0xAAu:   /* 2 bytes: pVRef, nVRef — aplicar de inmediato */
                        if (rx_bcount>=2u)
                        {
                            vdac_p_ref=rx_buf[0u];
                            vdac_n_ref=rx_buf[1u];
                            VDAC_p_SetValue(vdac_p_ref);
                            VDAC_n_SetValue(vdac_n_ref);
                            send_vref_status();
                            send_vref_cfg();
                            rx_state=0u;
                        }
                        break;

                    default: rx_state=0u; break;
                }
            }
        }

        /* ── Muestra lista ───────────────────────────────────────────────── */
        if (g_filter_ready)
        {
            saved=CyEnterCriticalSection();
            flt=g_flt; raw=ds; g_filter_ready=0u;
            CyExitCriticalSection(saved);
            (void)flt;

            /* DC IIR: elimina componente DC de la señal transmitida */
            dc_iir += (long long)raw - (dc_iir>>DC_SHIFT);

            /* TX: muestra con DC removido */
            {
                int32 raw_ac = raw - (int32)(dc_iir>>DC_SHIFT);
                pkt[0]=0x56u; pkt[1]=0x00u;
#if DEBUG
                { static uint32 c=0u;
#if PKT_DATA_BYTES>=3
                  pkt[2]=(uint8)((c>>16u)&0xFFu);
#endif
#if PKT_DATA_BYTES>=2
                  pkt[PKT_TOTAL_BYTES-2u]=(uint8)((c>>8u)&0xFFu);
#endif
                  pkt[PKT_TOTAL_BYTES-1u]=(uint8)(c&0xFFu);
                  c=(c+1u)&((1ul<<(PKT_DATA_BYTES*8u))-1ul); }
#else
#if PKT_DATA_BYTES==1
                pkt[2]=(uint8)(raw_ac&0xFFu);
#elif PKT_DATA_BYTES==2
                pkt[2]=(uint8)((raw_ac>>8u)&0xFFu);
                pkt[3]=(uint8)( raw_ac     &0xFFu);
#else
                pkt[2]=(uint8)((raw_ac>>16u)&0xFFu);
                pkt[3]=(uint8)((raw_ac>> 8u)&0xFFu);
                pkt[4]=(uint8)( raw_ac      &0xFFu);
#endif
#endif
                UART_PC_PutArray(pkt, PKT_TOTAL_BYTES);
            }

            if (++hb_count>=HB_PERIOD)
            {
                hb_count=0u;
                pkt[0]=0x56u; pkt[1]=0x01u;
                pkt[2]=0u; pkt[3]=0u; pkt[4]=0u;
                UART_PC_PutArray(pkt, PKT_TOTAL_BYTES);
            }
        }
    }
    return 0u;
}

/* [] END OF FILE */
