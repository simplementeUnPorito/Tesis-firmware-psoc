/* AnalogToDigital.c
 * ADC_DelSig + DFB Filter pipeline implementation — THESIS PERMANENT
 *
 * DMA chain layout:
 *  Chain A (ADC -> Filter STAGEA, 3 bytes, hardware trigger ADC EOC)
 *  Chain B (ADC -> g_raw_delsig, 3 bytes, triggered by Chain A TERMOUT)
 *  Chain C (Filter HOLDA -> g_filtered, 3 bytes, hardware trigger Filter DMA_Req_A)
 *           TERMOUT on Chain C fires isr_DMA_Filter → sets g_sample_ready
 */

#include "project.h"
#include "AnalogToDigital.h"
#include "buffers.h"

#include "DMA_DelSig_FIlter_dma.h"
#include "DMA_DelSig_RAM_dma.h"
#include "DMA_Filter_dma.h"

static uint8 s_td_adc_to_filter;
static uint8 s_td_adc_to_ram;
static uint8 s_td_filter_to_ram;

void AnalogToDigital_Init(void)
{
    /* --- Chain A: ADC_DelSig -> Filter STAGEA (3 bytes) --- */
    DMA_DelSig_FIlter_DmaHandle = DMA_DelSig_FIlter_DmaInitialize(
        3u, 1u,
        HI16(CYDEV_PERIPH_BASE),
        HI16(CYDEV_PERIPH_BASE));

    s_td_adc_to_filter = CyDmaTdAllocate();
    CyDmaTdSetConfiguration(
        s_td_adc_to_filter, 3u, s_td_adc_to_filter,
        CY_DMA_TD_INC_SRC_ADR | CY_DMA_TD_INC_DST_ADR | CY_DMA_TD_TERMOUT0_EN);
    CyDmaTdSetAddress(
        s_td_adc_to_filter,
        LO16((uint32)ADC_DelSig_DEC_SAMP_PTR),
        LO16((uint32)Filter_DFB__STAGEA));
    CyDmaChSetInitialTd(DMA_DelSig_FIlter_DmaHandle, s_td_adc_to_filter);

    /* --- Chain B: ADC_DelSig -> g_raw_delsig (3 bytes) --- */
    DMA_DelSig_RAM_DmaHandle = DMA_DelSig_RAM_DmaInitialize(
        3u, 1u,
        HI16(CYDEV_PERIPH_BASE),
        HI16(CYDEV_SRAM_BASE));

    s_td_adc_to_ram = CyDmaTdAllocate();
    CyDmaTdSetConfiguration(
        s_td_adc_to_ram, 3u, s_td_adc_to_ram,
        CY_DMA_TD_INC_SRC_ADR | CY_DMA_TD_INC_DST_ADR);
    CyDmaTdSetAddress(
        s_td_adc_to_ram,
        LO16((uint32)ADC_DelSig_DEC_SAMP_PTR),
        LO16((uint32)&g_raw_delsig));
    CyDmaChSetInitialTd(DMA_DelSig_RAM_DmaHandle, s_td_adc_to_ram);

    /* --- Chain C: Filter HOLDA -> g_filtered (3 bytes) --- */
    DMA_Filter_DmaHandle = DMA_Filter_DmaInitialize(
        3u, 1u,
        HI16(CYDEV_PERIPH_BASE),
        HI16(CYDEV_SRAM_BASE));

    s_td_filter_to_ram = CyDmaTdAllocate();
    CyDmaTdSetConfiguration(
        s_td_filter_to_ram, 3u, s_td_filter_to_ram,
        CY_DMA_TD_INC_SRC_ADR | CY_DMA_TD_INC_DST_ADR | CY_DMA_TD_TERMOUT0_EN);
    CyDmaTdSetAddress(
        s_td_filter_to_ram,
        LO16((uint32)Filter_DFB__HOLDA),
        LO16((uint32)&g_filtered));
    CyDmaChSetInitialTd(DMA_Filter_DmaHandle, s_td_filter_to_ram);

    /* DFB coherency: reading HOLDA (LSB) latches the full 24-bit word */
    Filter_SetCoherency(Filter_CHANNEL_A, Filter_KEY_LOW);
    Filter_SetDalign(Filter_STAGEA_DALIGN, 0u);
    Filter_SetDalign(Filter_HOLDA_DALIGN,  0u);
}

void AnalogToDigital_Start(void)
{
    Filter_Start();

    CyDmaChEnable(DMA_DelSig_FIlter_DmaHandle, 1u);
    CyDmaChEnable(DMA_DelSig_RAM_DmaHandle,    1u);
    CyDmaChEnable(DMA_Filter_DmaHandle,        1u);

    /* Start in default config (CFG4 = ±6.144 V, buf_gain 8) */
    ADC_DelSig_SelectConfiguration(ADC_DelSig_CFG4, 1u);
    ADC_DelSig_SetBufferGain(ADC_DelSig_BUF_GAIN_8X);
    ADC_DelSig_Start();
    ADC_DelSig_StartConvert();
    /* ADC_SAR is started by TestBench_Init() */
}

void AnalogToDigital_Stop(void)
{
    ADC_DelSig_StopConvert();
    ADC_DelSig_Stop();
    Filter_Stop();

    CyDmaChDisable(DMA_DelSig_FIlter_DmaHandle);
    CyDmaChDisable(DMA_DelSig_RAM_DmaHandle);
    CyDmaChDisable(DMA_Filter_DmaHandle);
}

void AnalogToDigital_SetADC(uint8 cfg, uint8 buf_gain)
{
    uint8 psoc_cfg;
    uint8 psoc_gain;

    /* Clamp to valid config range */
    if (cfg < ATD_CFG_1 || cfg > ATD_CFG_4) { cfg = ATD_CFG_4; }

    /* CFG1 constraint: buffer gain must be 1 */
    if (cfg == ATD_CFG_1) { buf_gain = ATD_BGAIN_1; }

    /* Map cfg index to PSoC Creator constants */
    switch (cfg)
    {
        case ATD_CFG_1: psoc_cfg = ADC_DelSig_CFG1; break;
        case ATD_CFG_2: psoc_cfg = ADC_DelSig_CFG2; break;
        case ATD_CFG_3: psoc_cfg = ADC_DelSig_CFG3; break;
        case ATD_CFG_4:
        default:        psoc_cfg = ADC_DelSig_CFG4; break;
    }

    /* Map gain value to PSoC Creator buffer gain constant */
    if      (buf_gain >= ATD_BGAIN_8) { psoc_gain = ADC_DelSig_BUF_GAIN_8X; buf_gain = ATD_BGAIN_8; }
    else if (buf_gain >= ATD_BGAIN_4) { psoc_gain = ADC_DelSig_BUF_GAIN_4X; buf_gain = ATD_BGAIN_4; }
    else if (buf_gain >= ATD_BGAIN_2) { psoc_gain = ADC_DelSig_BUF_GAIN_2X; buf_gain = ATD_BGAIN_2; }
    else                              { psoc_gain = ADC_DelSig_BUF_GAIN_1X; buf_gain = ATD_BGAIN_1; }

    ADC_DelSig_SelectConfiguration(psoc_cfg, 1u);
    ADC_DelSig_SetBufferGain(psoc_gain);

    g_adc_cfg      = cfg;
    g_adc_buf_gain = buf_gain;
}

uint8 AnalogToDigital_GetCfg(void)
{
    return g_adc_cfg;
}

uint8 AnalogToDigital_GetBufGain(void)
{
    return g_adc_buf_gain;
}
