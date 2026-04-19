/* TestBench.c
 * Testbench analog front-end control — NOT THESIS PERMANENT
 *
 * Also owns Chain D (ADC_SAR -> g_sar_latest DMA) since SAR is testbench-only.
 *
 * NOTE: Requires Timer_VDAC (16-bit UDB, 24MHz / period 8000 = 3000 Hz, "On TC" ticked)
 * and isr_Timer interrupt connected to Timer_VDAC interrupt output in TopDesign.
 */

#include "project.h"
#include "TestBench.h"
#include "tb_buffers.h"
#include "isr_handlers.h"

/* SAR DMA — belongs here because SAR is testbench-only */
#include "DMA_SAR_dma.h"

static uint8 s_td_sar_to_ram;

/* ---- SAR DMA init (Chain D) -------------------------------------------- */

static void SAR_DMA_Init(void)
{
    DMA_SAR_DmaHandle = DMA_SAR_DmaInitialize(
        2u, 1u,
        HI16(CYDEV_PERIPH_BASE),
        HI16(CYDEV_SRAM_BASE));

    s_td_sar_to_ram = CyDmaTdAllocate();
    CyDmaTdSetConfiguration(
        s_td_sar_to_ram, 2u, s_td_sar_to_ram,
        CY_DMA_TD_INC_SRC_ADR | CY_DMA_TD_INC_DST_ADR | CY_DMA_TD_TERMOUT0_EN);
    CyDmaTdSetAddress(
        s_td_sar_to_ram,
        LO16((uint32)ADC_SAR_SAR_WRK_PTR),
        LO16((uint32)&g_sar_latest));
    CyDmaChSetInitialTd(DMA_SAR_DmaHandle, s_td_sar_to_ram);
}

/* ---- Public API --------------------------------------------------------- */

void TestBench_Init(void)
{
    /* Analog multiplexers */
    MuxIn_Start();
    MuxOut_Start();
    MuxIn_FastSelect(TB_INPUT_GEOPHONE);
    MuxOut_FastSelect(TB_OUTPUT_FILTER1_TIA);
    g_vdac_mode  = TB_INPUT_GEOPHONE;
    g_filter_sel = TB_OUTPUT_FILTER1_TIA;

    /* VDACs at midpoint (zero differential) */
    VDACIn_p_Start();
    VDACIn_n_Start();
    VDACIn_p_SetValue(128u);
    VDACIn_n_SetValue(128u);

    /* Default VDAC sequence: one sample at midpoint (silence) */
    g_vdac_seq[0] = 128u;
    g_vdac_len    = 1u;
    g_vdac_idx    = 0u;

    /* SAR ADC + DMA (free-running at ~55 kSPS) */
    SAR_DMA_Init();
    isr_SAR_StartEx(isr_SAR_Handler);
    ADC_SAR_Start();
    CyDmaChEnable(DMA_SAR_DmaHandle, 1u);

    /* VDAC update timer ISR — isr_Timer is the PSoC component name */
    isr_Timer_StartEx(isr_VDAC_Handler);

    /* Start Timer_VDAC (24 MHz / 8000 = 3000 Hz).
     * "On TC" must be checked in PSoC Creator Timer_VDAC Interrupts tab. */
    Timer_VDAC_Start();
}

void TestBench_SetInput(uint8 source)
{
    if (source == TB_INPUT_GEOPHONE || source == TB_INPUT_VDAC)
    {
        MuxIn_FastSelect(source);
        g_vdac_mode = source;

        if (source == TB_INPUT_GEOPHONE)
        {
            VDACIn_p_SetValue(128u);
            VDACIn_n_SetValue(128u);
        }
    }
}

void TestBench_SetOutput(uint8 output)
{
    if (output == TB_OUTPUT_FILTER1_TIA || output == TB_OUTPUT_FILTER2_OPA)
    {
        MuxOut_FastSelect(output);
        g_filter_sel = output;
    }
}

void TestBench_SetVdacSeq(const uint8 *data, uint16 len)
{
    uint16 i;

    isr_Timer_Disable();

    if (len > TB_VDAC_MAX_LEN) { len = TB_VDAC_MAX_LEN; }
    for (i = 0u; i < len; i++) { g_vdac_seq[i] = data[i]; }
    g_vdac_len = len;
    g_vdac_idx = 0u;

    isr_Timer_Enable();
}

void TestBench_Task(void)
{
    /* Reserved */
}
