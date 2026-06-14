#include "psoc_adc.h"

static volatile int32 g_psoc_adc_isr_counts = 0;
static volatile uint8 g_psoc_adc_isr_ready = 0u;

int32 psoc_adc_counts_right_aligned(int32 adc_counts)
{
#if (ADC_CFG1_DEC_DIV != 0)
    if (ADC_Config == ADC_CFG1) {
        adc_counts /= ADC_CFG1_DEC_DIV;
    }
#endif
    return adc_counts;
}

void psoc_adc_clear_isr_sample(void)
{
    uint8 saved = CyEnterCriticalSection();
    g_psoc_adc_isr_ready = 0u;
    CyExitCriticalSection(saved);
}

void psoc_adc_note_isr_sample(int32 counts)
{
    g_psoc_adc_isr_counts = counts;
    g_psoc_adc_isr_ready = 1u;
}

uint8 psoc_adc_take_isr_sample(int32 *out_counts)
{
    uint8 ok = 0u;
    uint8 saved;
    if (out_counts == (int32 *)0) {
        return 0u;
    }
    saved = CyEnterCriticalSection();
    if (g_psoc_adc_isr_ready) {
        *out_counts = g_psoc_adc_isr_counts;
        g_psoc_adc_isr_ready = 0u;
        ok = 1u;
    }
    CyExitCriticalSection(saved);
    return ok;
}

void psoc_adc_select_capture_config(void)
{
    ADC_Stop();
    ADC_Start();
    ADC_StopConvert();
}
