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
#if ((ADC_DEFAULT_NUM_CONFIGS > 1) && (ADC_CFG2_DEC_DIV != 0))
    if (ADC_Config == ADC_CFG2) {
        adc_counts /= ADC_CFG2_DEC_DIV;
    }
#endif
#if ((ADC_DEFAULT_NUM_CONFIGS > 2) && (ADC_CFG3_DEC_DIV != 0))
    if (ADC_Config == ADC_CFG3) {
        adc_counts /= ADC_CFG3_DEC_DIV;
    }
#endif
#if ((ADC_DEFAULT_NUM_CONFIGS > 3) && (ADC_CFG4_DEC_DIV != 0))
    if (ADC_Config == ADC_CFG4) {
        adc_counts /= ADC_CFG4_DEC_DIV;
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

void psoc_adc_select_calibration_config(void)
{
    psoc_adc_select_capture_config();
}

uint8 psoc_adc_wait_fresh_sample(int32 *out_counts, uint16 timeout_us)
{
    uint16 waited_us = 0u;
    uint16 step_us = 50u;

    if (out_counts == (int32 *)0) {
        return 0u;
    }
    if (timeout_us == 0u) {
        timeout_us = 1u;
    }
    if (timeout_us < step_us) {
        step_us = timeout_us;
    }

    psoc_adc_clear_isr_sample();
    while (!psoc_adc_take_isr_sample(out_counts)) {
        if (waited_us >= timeout_us) {
            return 0u;
        }
        CyDelayUs(step_us);
        waited_us = (uint16)(waited_us + step_us);
    }
    return 1u;
}
