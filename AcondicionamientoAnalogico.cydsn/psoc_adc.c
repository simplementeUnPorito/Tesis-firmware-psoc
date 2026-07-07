#include "psoc_adc.h"

static volatile int32 g_psoc_adc_isr_counts = 0;
static volatile uint8 g_psoc_adc_isr_ready = 0u;

/* Config activa pedida por el host (ADC_CF_2V5 | ADC_CF_0V512). */
static uint8 g_psoc_adc_config = ADC_CF_2V5;
/* Override de calibración: fuerza 2V5 sin perder la elección del usuario. */
static uint8 g_psoc_adc_cal_force_2v5 = 0u;

int32 psoc_adc_counts_right_aligned(int32 adc_counts)
{
    /* Solo ADC_CF_2V5 es left-aligned (ALIGNMENT=1, DEC_DIV=32); las demás
     * configs entregan counts right-aligned (DEC_DIV=0). Se consulta
     * ADC_Config (variable del componente, mantenida por
     * ADC_SelectConfiguration) y no g_psoc_adc_config, porque durante la
     * calibración el override puede tener aplicada una config distinta. */
#if (ADC_CF_2V5_DEC_DIV != 0)
    if (ADC_Config == ADC_CF_2V5) {
        adc_counts /= ADC_CF_2V5_DEC_DIV;
    }
#endif
    return adc_counts;
}

uint8 psoc_adc_get_config(void)
{
    return g_psoc_adc_config;
}

uint8 psoc_adc_set_config(uint8 cfg)
{
    if ((cfg != ADC_CF_2V5) && (cfg != ADC_CF_0V512)) {
        return 0u;
    }
    g_psoc_adc_config = cfg;
    return 1u;
}

uint16 psoc_adc_effective_fs_hz(void)
{
    /* "Actual conv. rate" del customizer: la división de clock no alcanza
     * el nominal de 1000 SPS y ambas configs expuestas quedan en 1020 SPS. */
    return 1020u;
}

void psoc_adc_cal_override(uint8 force_2v5)
{
    g_psoc_adc_cal_force_2v5 = (force_2v5 != 0u) ? 1u : 0u;
}

void psoc_adc_select_calibration_config(void)
{
    psoc_adc_cal_override(1u);
    psoc_adc_select_capture_config();
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
    uint8 cfg = g_psoc_adc_cal_force_2v5 ? (uint8)ADC_CF_2V5 : g_psoc_adc_config;
    /* SelectConfiguration hace ADC_Stop + InitConfig + compensación GCOR y
     * deja ADC_Config actualizado; con restart=0 no arranca conversiones.
     * ADC_Start posterior solo hace Enable (initVar!=0) y no pisa la config. */
    ADC_SelectConfiguration(cfg, 0u);
    /* Nuestros DMA leen ADC_DEC_SAMP_PTR en orden low-mid-high. La config 2
     * generada usa coherencia LOW, que libera el registro en el primer byte y
     * puede mezclar muestras. Forzamos HIGH para que el byte llave sea el
     * último que lee el DMA, igual que en la config 1. */
    ADC_SetCoherency(ADC_DEC_SAMP_KEY_HIGH);
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
