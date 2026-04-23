/*******************************************************************************
* main.c — PSoC5 simplificado
*
* Flujo de datos (todas las rutas por ISR):
*
*   isr_DelSig (1500 SPS, ADC_DelSig CFG1 18-bit ±6.144V):
*     1. GetResult32() → g_ds  (lectura limpia el flag de la ISR)
*     2. Escribe g_ds en Filter STAGEA → LSB dispara calculo DFB
*     3. g_delsig_ready = 1
*
*   isr_Filter (conectado a Filter_DFB_DMA_Req_A en TopDesign):
*     1. Lee Filter HOLDA (+0 latch+ack, +1, +2) → g_flt
*        Leer HOLDA[0] desaserta DMA_Req_A (no necesita ClearPending)
*     2. g_filter_ready = 1
*
*   isr_SAR (free-running, 12-bit ±1.024V, ~55 kSPS):
*     1. GetResult16() → g_sar
*     2. g_sar_ready = 1
*
*   isr_VDAC (Timer 24MHz, VDAC state machine):
*     - Genera señal Ricker diferencial
*     - Heartbeat cada ciclo (3000 muestras activas + 3000 pausa)
*
* Main loop: cuando g_delsig_ready && g_filter_ready && g_sar_ready → envia paquete.
* Tasa efectiva = 1500 paquetes/s (cadenciada por el DelSig).
*
* Packet TX: [0xAA][0x00][sar_hi][sar_lo][ds2][ds1][ds0][flt2][flt1][flt0]
*   sar = int16 big-endian
*   ds  = int24 big-endian signed  (raw GetResult32, 18-bit en CFG1)
*   flt = int24 big-endian signed  (salida DFB del sample anterior — 1 muestra delay causal)
* Heartbeat: [0xAA][0x01][0x00 x8]
*
* CMD RX: [0xBB][0x03][freq_dh_hi][freq_dh_lo][cksum]
*   cksum = 0xBB ^ 0x03 ^ hi ^ lo  →  PSoC responde 'K' / '!'
*
* NOTA TopDesign: isr_Filter debe conectarse a Filter_DFB_DMA_Req_A.
*******************************************************************************/

#include "project.h"
#include "ricker_table.h"

/* ── Globals ISR → main ────────────────────────────────────────── */
static volatile int32  g_ds           = 0;
static volatile int32  g_flt          = 0;
static volatile int16  g_sar          = 0;
static volatile uint8  g_delsig_ready = 0u;
static volatile uint8  g_filter_ready = 0u;
static volatile uint8  g_sar_ready    = 0u;
static volatile uint8  g_heartbeat    = 0u;

/* ── Estado VDAC ───────────────────────────────────────────────── */
static volatile uint16 s_vdac_idx    = 0u;
static volatile uint16 s_pause_count = 0u;
static volatile uint8  s_in_pause    = 0u;

/* ── ISRs ADC + Filtro ─────────────────────────────────────────── */

/* Disparo: fin de conversion ADC_DelSig (1500 SPS).
 * GetResult32() limpia el flag de la ISR en el hardware. */
CY_ISR(isr_DelSigReady)
{
    int32 ds = ADC_DelSig_GetResult32();

    /* Escribe sample al filtro DFB — KEY_LOW: MSB primero, LSB dispara */
    Filter_Write24(Filter_CHANNEL_A,(uint32)ds);
    

    g_ds           = ds;
    g_delsig_ready = 1u;
}

/* Disparo: Filter_DFB_DMA_Req_A — salida de canal A lista en HOLDA.
 * Leer HOLDA[0] latch + desaserta DMA_Req_A (no necesita ClearPending). */
CY_ISR(isr_FilterReady)
{
    
    g_flt          = Filter_Read24(Filter_CHANNEL_A);
    g_filter_ready = 1u;
}

/* Disparo: fin de conversion ADC_SAR (~55 kSPS, free-running).
 * GetResult16() limpia el flag de la ISR en el hardware. */
CY_ISR(isr_SARReady)
{
    g_sar       = ADC_SAR_GetResult16();
    g_sar_ready = 1u;
}

/* ── ISR Timer — VDAC state machine ───────────────────────────── */
CY_ISR(isr_VDAC_Handler)
{
    Timer_ReadStatusRegister();   /* limpia el flag del timer */

    if (s_in_pause)
    {
        VDACIn_p_SetValue(128u);
        VDACIn_n_SetValue(128u);
        if (++s_pause_count >= 3000u)
        {
            s_pause_count = 0u;
            s_in_pause    = 0u;
            s_vdac_idx    = 0u;
            g_heartbeat   = 1u;
        }
    }
    else
    {
        uint8 val = ricker_table[s_vdac_idx];
        VDACIn_p_SetValue(val);
        VDACIn_n_SetValue((uint8)(255u - val));
        if (++s_vdac_idx >= 3000u)
        {
            s_vdac_idx    = 0u;
            s_in_pause    = 1u;
            s_pause_count = 0u;
        }
    }
}

/* ── Parser CMD ────────────────────────────────────────────────── */
#define CMD_MARKER  0xBBu
#define CMD_FREQ    0x03u
#define RSP_OK      ((uint8)'K')
#define RSP_ERR     ((uint8)'!')

static void process_rx(void)
{
    static uint8 state   = 0u;
    static uint8 cmd     = 0u;
    static uint8 freq_hi = 0u;
    static uint8 freq_lo = 0u;
    uint16 freq_dh, period;

    while (UART_PC_GetRxBufferSize() > 0u)
    {
        uint8 b = UART_PC_GetChar();
        switch (state)
        {
            case 0u: if (b == CMD_MARKER) { state = 1u; } break;
            case 1u: cmd = b; state = 2u; break;
            case 2u:
                if (cmd == CMD_FREQ) { freq_hi = b; state = 3u; }
                else                 { state = 0u; }
                break;
            case 3u: freq_lo = b; state = 4u; break;
            case 4u:
                if (b == (uint8)(CMD_MARKER ^ CMD_FREQ ^ freq_hi ^ freq_lo))
                {
                    freq_dh = ((uint16)freq_hi << 8u) | (uint16)freq_lo;
                    if (freq_dh > 0u)
                    {
                        period = (uint16)(80000UL / (uint32)freq_dh);
                        if (period > 0u) { period -= 1u; }
                        Timer_WritePeriod(period);
                        Timer_WriteCounter(period);
                        s_vdac_idx    = 0u;
                        s_pause_count = 0u;
                        s_in_pause    = 0u;
                    }
                    UART_PC_PutChar(RSP_OK);
                }
                else { UART_PC_PutChar(RSP_ERR); }
                state = 0u;
                break;
            default: state = 0u; break;
        }
    }
}

/* ── Main ──────────────────────────────────────────────────────── */
int main(void)
{
    uint8  pkt[10u];
    uint8  i, saved;
    int32  ds, flt;
    int16  sar;

    CyGlobalIntEnable;

    UART_PC_Start();
    Opamp_ref_Start();
    TIA_p_Start();
    TIA_n_Start();
    PGA_p_Start();
    PGA_n_Start();
    VDACIn_p_Start();
    VDACIn_n_Start();
    VDACIn_p_SetValue(128u);
    VDACIn_n_SetValue(128u);

    Filter_Start();
    Filter_SetCoherency(Filter_CHANNEL_A, Filter_KEY_LOW);
    Filter_SetDalign(Filter_STAGEA_DALIGN, 0u);
    Filter_SetDalign(Filter_HOLDA_DALIGN,  0u);

    /* ADC_DelSig: SelectConfiguration(CFG1, restart=1) llama Start() + StartConvert() */
    ADC_DelSig_Start();
    ADC_DelSig_StartConvert();
    isr_DelSig_StartEx(isr_DelSigReady);
    isr_Filter_StartEx(isr_FilterReady);

    /* ADC_SAR: free-running, ISR captura cada resultado */
    ADC_SAR_Start();
    ADC_SAR_StartConvert();
    isr_SAR_StartEx(isr_SARReady);

    isr_Timer_StartEx(isr_VDAC_Handler);
    Timer_WritePeriod(7999u);
    Timer_Start();

    for(;;)
    {
        process_rx();

        if (g_heartbeat)
        {
            g_heartbeat = 0u;
            pkt[0]=0xAAu; pkt[1]=0x01u;
            pkt[2]=0u; pkt[3]=0u; pkt[4]=0u;
            pkt[5]=0u; pkt[6]=0u; pkt[7]=0u; pkt[8]=0u; pkt[9]=0u;
            for (i=0u; i<10u; i++) { UART_PC_PutChar(pkt[i]); }
        }

        /* Envia paquete solo cuando los tres canales tienen datos frescos.
         * g_sar_ready se cumple casi siempre (SAR ~55kSPS >> 1500SPS del DelSig).
         * g_filter_ready se cumple pocos us despues de g_delsig_ready. */
        if (g_delsig_ready && g_filter_ready && g_sar_ready)
        {
            saved = CyEnterCriticalSection();
            ds             = g_ds;
            flt            = g_flt;
            sar            = g_sar;
            g_delsig_ready = 0u;
            g_filter_ready = 0u;
            g_sar_ready    = 0u;
            CyExitCriticalSection(saved);

            pkt[0] = 0xAAu;
            pkt[1] = 0x00u;
            pkt[2] = (uint8)((sar >> 8)  & 0xFFu);
            pkt[3] = (uint8)( sar        & 0xFFu);
            pkt[4] = (uint8)((ds  >> 16) & 0xFFu);
            pkt[5] = (uint8)((ds  >>  8) & 0xFFu);
            pkt[6] = (uint8)( ds         & 0xFFu);
            pkt[7] = (uint8)((flt >> 16) & 0xFFu);
            pkt[8] = (uint8)((flt >>  8) & 0xFFu);
            pkt[9] = (uint8)( flt        & 0xFFu);
            for (i=0u; i<10u; i++) { UART_PC_PutChar(pkt[i]); }
        }
    }

    return 0u;
}

/* [] END OF FILE */
