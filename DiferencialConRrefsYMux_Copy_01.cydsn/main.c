/*******************************************************************************
* main.c — Geofono diferencial: ADC_DelSig -> Filter DFB -> UART
*
* Protocolo TX (5 bytes por paquete, igual que DiferencialConRrefs_Copy_01_Copy_01):
*   Data:      [0x56][0x00][b2][b1][b0]   int24 big-endian, DC removido por IIR
*   Heartbeat: [0x56][0x01][0x00][0x00][0x00]
*   Cfg-ADC:   [0x56][0x02][res][fsH][fsL]
*   Cfg-PGA:   [0x56][0x03][code][vH][vL]
*   VRef-St:   [0x56][0x04][vdac_p][vdac_n][0x01]
*   AMUX-St:   [0x56][0x05][ch][0x00][0x00]    ch: 0=dif 1=CM+ 2=CM-
*   VRef-Cfg:  [0x56][0x06][pVRef][nVRef][0x00]
*
* Comandos RX:
*   0xA5            -> reenvía config completa
*   0xA6 <code>     -> PGA_SetGain(code)
*   0xA7 <ch>       -> AMux_FastSelect(ch)  ch: 0=dif 1=CM+ 2=CM-
*   0xAA <pV> <nV>  -> setea VDAC_p = pV, VDAC_n = nV de inmediato
*******************************************************************************/

#include "project.h"

#define ADC_VREF_HALFMV 6144u
#define HB_PERIOD       3000u
#define DC_SHIFT        14u

static uint8 vdac_p_ref = 0x94u;
static uint8 vdac_n_ref = 0x94u;
static uint8 g_amux_ch  = 0u;

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

static long long dc_iir = 0;

static void send_vref_status(void)
{
    uint8 pkt[5u];
    pkt[0]=0x56u; pkt[1]=0x04u;
    pkt[2]=vdac_p_ref; pkt[3]=vdac_n_ref; pkt[4]=0x01u;
    UART_PC_PutArray(pkt, 5u);
}

static void send_amux_status(void)
{
    uint8 pkt[5u];
    pkt[0]=0x56u; pkt[1]=0x05u;
    pkt[2]=g_amux_ch; pkt[3]=0u; pkt[4]=0u;
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
    send_amux_status();
}

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

    PGA_p_Start(); PGA_n_Start();
    PGA_p_SetGain(g_pga_code); PGA_n_SetGain(g_pga_code);
    OpaBP_p_Start(); OpaBP_n_Start();
    TIA_p_Start();  TIA_n_Start();

    AMux_Start();
    AMux_FastSelect(0u);

    VDAC_p_Start(); VDAC_n_Start(); Opa_p_Start(); Opa_n_Start();
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
        while (UART_PC_GetRxBufferSize() > 0u)
        {
            rx = UART_PC_ReadRxData();
            if (rx_state == 0u)
            {
                switch (rx)
                {
                    case 0xA5u: send_config(); break;
                    case 0xA6u: case 0xA7u: case 0xAAu:
                        rx_cmd=rx; rx_bcount=0u; rx_state=1u; break;
                    default: break;
                }
            }
            else
            {
                rx_buf[rx_bcount]=rx; rx_bcount++;
                switch (rx_cmd)
                {
                    case 0xA6u:
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

                    case 0xA7u:
                        if (rx_bcount>=1u)
                        {
                            if (rx_buf[0u]<=2u)
                            {
                                g_amux_ch=rx_buf[0u];
                                AMux_FastSelect(g_amux_ch);
                                dc_iir = 0;
                                send_amux_status();
                            }
                            rx_state=0u;
                        }
                        break;

                    case 0xAAu:
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

        if (g_filter_ready)
        {
            saved=CyEnterCriticalSection();
            flt=g_flt; raw=ds; g_filter_ready=0u;
            CyExitCriticalSection(saved);
            (void)flt;

            dc_iir += (long long)raw - (dc_iir>>DC_SHIFT);

            {
                int32 raw_ac = raw - (int32)(dc_iir>>DC_SHIFT);
                pkt[0]=0x56u; pkt[1]=0x00u;
                pkt[2]=(uint8)((raw_ac>>16u)&0xFFu);
                pkt[3]=(uint8)((raw_ac>> 8u)&0xFFu);
                pkt[4]=(uint8)( raw_ac      &0xFFu);
                UART_PC_PutArray(pkt, 5u);
            }

            if (++hb_count>=HB_PERIOD)
            {
                hb_count=0u;
                pkt[0]=0x56u; pkt[1]=0x01u;
                pkt[2]=0u; pkt[3]=0u; pkt[4]=0u;
                UART_PC_PutArray(pkt, 5u);
            }
        }
    }
    return 0u;
}

/* [] END OF FILE */
