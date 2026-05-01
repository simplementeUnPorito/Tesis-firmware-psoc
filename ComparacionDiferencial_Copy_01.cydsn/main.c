/*******************************************************************************
* main.c — Geofono diferencial: ADC_DelSig -> Filter DFB -> FIR Notch -> UART
*
* Flujo de datos (ISR-driven):
*
*   isr_DelSig  (EOC del ADC_DelSig, ~1500 SPS):
*     1. GetResult32() -> escribe directo en Filter STAGEA via Filter_Write24
*
*   isr_Filter  (Filter_DFB_DMA_Req_A — salida canal A lista en HOLDA):
*     1. Filter_Read24(CHANNEL_A) -> g_flt
*     2. g_filter_ready = 1
*
*   Main loop:
*     - Lee g_flt, aplica FIR notch software
*     - Envia paquete de 5 bytes por UART_PC a 460800 bps
*     - Heartbeat cada 3000 muestras (~2 s a 1500 SPS)
*
* Protocolo TX (5 bytes):
*   Data:      [0x56][0x00][notch2][notch1][notch0]
*   Heartbeat: [0x56][0x01][0x00  ][0x00  ][0x00  ]
*   notch = int24 big-endian signed (salida FIR notch sobre la salida DFB)
*
* NOTA TopDesign: isr_Filter debe conectarse a Filter_DFB_DMA_Req_A.
*******************************************************************************/

#include "project.h"
#include "coeficientes_notch.h"

/*
 * DEBUG = 1: envia rampa 0,1,2,3... en vez de datos reales.
 *   - Si MATLAB muestra una rampa suave → byte order y alineacion correctos.
 *   - Si la rampa esta scrambled → algun byte esta en orden incorrecto.
 *   - La rampa sube de 0 a 16777215 (0xFFFFFF) y vuelve a 0.
 * DEBUG = 0: operacion normal (notch FIR sobre DFB).
 */
#define DEBUG 0

/* ── Globals ISR → main ─────────────────────────────────────────────────── */
static volatile int32 g_flt          = 0;
static volatile uint8 g_filter_ready = 0u;
static volatile int32 ds = 0u;
/* ── ISR: ADC_DelSig fin de conversion ──────────────────────────────────── */
CY_ISR(isr_DelSigReady)
{
    ds = ADC_DelSig_GetResult32();
    Filter_Write24(Filter_CHANNEL_A, (uint32)ds);
}

/* ── ISR: Filter DFB canal A listo ─────────────────────────────────────── */
CY_ISR(isr_FilterReady)
{
    uint32 raw = Filter_Read24(Filter_CHANNEL_A);
    g_flt = (raw & 0x800000u) ? (int32)(raw | 0xFF000000u) : (int32)raw;
    g_filter_ready = 1u;
}

/* ── Filtro FIR Notch software (buffer circular) ───────────────────────── */
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

/* ── Main ───────────────────────────────────────────────────────────────── */
int main(void)
{
    static uint16 hb_count = 0u;
    uint8  pkt[5u];
    int32  notch_out;
    int32  flt;
    uint8  saved;

    CyGlobalIntEnable;

    UART_PC_Start();

    /* Front-end analogico diferencial */
    PGA_p_1_Start();
    PGA_p_2_Start();
    
    PGA_n_1_Start();
    PGA_n_2_Start();
    
    
    OpaBP_p_Start();
    OpaBP_n_Start();
    
    OpaLP_p_Start();
    OpaLP_n_Start();

    /* Filter DFB */
    Filter_Start();
    Filter_SetCoherency(Filter_CHANNEL_A, Filter_KEY_LOW);
    Filter_SetDalign(Filter_STAGEA_DALIGN, 0u);
    Filter_SetDalign(Filter_HOLDA_DALIGN,  0u);

    /* ADC_DelSig — modo Continuous */
    ADC_DelSig_Start();
    ADC_DelSig_StartConvert();
    isr_DelSig_StartEx(isr_DelSigReady);
    isr_Filter_StartEx(isr_FilterReady);

    for(;;)
    {
        if (g_filter_ready)
        {
            saved = CyEnterCriticalSection();
            flt            = g_flt;
            g_filter_ready = 0u;
            CyExitCriticalSection(saved);

            //notch_out = fir_notch(flt);

            pkt[0] = 0x56u;
            pkt[1] = 0x00u;
#if DEBUG
            {
                static uint32 dbg_cnt = 0u;
                pkt[2] = (uint8)((dbg_cnt >> 16) & 0xFFu);
                pkt[3] = (uint8)((dbg_cnt >>  8) & 0xFFu);
                pkt[4] = (uint8)( dbg_cnt        & 0xFFu);
                dbg_cnt = (dbg_cnt + 1u) & 0x00FFFFFFu;
            }
#else
            pkt[2] = (uint8)((flt >> 16) & 0xFFu);
            pkt[3] = (uint8)((flt >>  8) & 0xFFu);
            pkt[4] = (uint8)( flt        & 0xFFu);
#endif
            UART_PC_PutArray(pkt, 5u);

            if (++hb_count >= 3000u)
            {
                hb_count = 0u;
                pkt[0] = 0x56u; pkt[1] = 0x01u;
                pkt[2] = 0u;    pkt[3] = 0u;   pkt[4] = 0u;
                UART_PC_PutArray(pkt, 5u);
            }
        }
    }

    return 0u;
}

/* [] END OF FILE */
