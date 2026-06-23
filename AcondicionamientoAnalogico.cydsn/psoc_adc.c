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

static volatile int32 g_psoc_adc_isr_window_sum   = 0;
static volatile uint8 g_psoc_adc_isr_window_n      = 0u;
static volatile uint8 g_psoc_adc_isr_window_ready  = 0u;

void psoc_adc_clear_isr_window(void)
{
    uint8 saved = CyEnterCriticalSection();
    g_psoc_adc_isr_window_ready = 0u;
    CyExitCriticalSection(saved);
}

void psoc_adc_note_isr_window(int32 sum, uint8 n)
{
    g_psoc_adc_isr_window_sum = sum;
    g_psoc_adc_isr_window_n = n;
    g_psoc_adc_isr_window_ready = 1u;
}

uint8 psoc_adc_take_isr_window(int32 *out_sum, uint8 *out_n)
{
    uint8 ok = 0u;
    uint8 saved;
    if (out_sum == (int32 *)0 || out_n == (uint8 *)0) {
        return 0u;
    }
    saved = CyEnterCriticalSection();
    if (g_psoc_adc_isr_window_ready) {
        *out_sum = g_psoc_adc_isr_window_sum;
        *out_n = g_psoc_adc_isr_window_n;
        g_psoc_adc_isr_window_ready = 0u;
        ok = 1u;
    }
    CyExitCriticalSection(saved);
    return ok;
}

static volatile int32 g_psoc_adc_isr_block[PSOC_ADC_LOTE_SAMPLES];
static volatile uint8 g_psoc_adc_isr_block_n     = 0u;
static volatile uint8 g_psoc_adc_isr_block_ready = 0u;

void psoc_adc_clear_isr_block(void)
{
    uint8 saved = CyEnterCriticalSection();
    g_psoc_adc_isr_block_ready = 0u;
    CyExitCriticalSection(saved);
}

void psoc_adc_note_isr_block(const int32 *samples, uint8 n)
{
    uint8 i;
    uint8 count = (n > PSOC_ADC_LOTE_SAMPLES) ? (uint8)PSOC_ADC_LOTE_SAMPLES : n;
    for (i = 0u; i < count; i++) {
        g_psoc_adc_isr_block[i] = samples[i];
    }
    g_psoc_adc_isr_block_n = count;
    g_psoc_adc_isr_block_ready = 1u;
}

uint8 psoc_adc_take_isr_block(int32 *out_samples, uint8 *out_n, uint8 max_n)
{
    uint8 ok = 0u;
    uint8 saved;
    uint8 i;
    uint8 count;
    if (out_samples == (int32 *)0 || out_n == (uint8 *)0) {
        return 0u;
    }
    saved = CyEnterCriticalSection();
    if (g_psoc_adc_isr_block_ready) {
        count = (g_psoc_adc_isr_block_n > max_n) ? max_n : g_psoc_adc_isr_block_n;
        for (i = 0u; i < count; i++) {
            out_samples[i] = g_psoc_adc_isr_block[i];
        }
        *out_n = count;
        g_psoc_adc_isr_block_ready = 0u;
        ok = 1u;
    }
    CyExitCriticalSection(saved);
    return ok;
}

static volatile int32 g_psoc_adc_isr_filtered_counts = 0;
static volatile uint8 g_psoc_adc_isr_filtered_ready  = 0u;

void psoc_adc_clear_isr_filtered_sample(void)
{
    uint8 saved = CyEnterCriticalSection();
    g_psoc_adc_isr_filtered_ready = 0u;
    CyExitCriticalSection(saved);
}

void psoc_adc_note_isr_filtered_sample(int32 counts)
{
    g_psoc_adc_isr_filtered_counts = counts;
    g_psoc_adc_isr_filtered_ready = 1u;
}

uint8 psoc_adc_take_isr_filtered_sample(int32 *out_counts)
{
    uint8 ok = 0u;
    uint8 saved;
    if (out_counts == (int32 *)0) {
        return 0u;
    }
    saved = CyEnterCriticalSection();
    if (g_psoc_adc_isr_filtered_ready) {
        *out_counts = g_psoc_adc_isr_filtered_counts;
        g_psoc_adc_isr_filtered_ready = 0u;
        ok = 1u;
    }
    CyExitCriticalSection(saved);
    return ok;
}
