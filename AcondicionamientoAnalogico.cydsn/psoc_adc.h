#ifndef PSOC_ADC_H
#define PSOC_ADC_H

#include "project.h"

/* Tamaño del lote HW (canal DMA_*_RAM_Lote) en muestras. Vive acá porque
 * tanto main.c como calibration.c ya incluyen este header; calibration.c
 * además lo cruza en tiempo de compilación contra CAL_AVG_N_GEO_* (ver
 * calibration.c) para garantizar que un lote HW equivalga exactamente a
 * una ventana de calibración. */
#ifndef PSOC_ADC_LOTE_SAMPLES
#define PSOC_ADC_LOTE_SAMPLES 64u
#endif

int32 psoc_adc_counts_right_aligned(int32 adc_counts);
void psoc_adc_select_capture_config(void);
void psoc_adc_clear_isr_sample(void);
void psoc_adc_note_isr_sample(int32 counts);
uint8 psoc_adc_take_isr_sample(int32 *out_counts);

void  psoc_adc_clear_isr_window(void);
void  psoc_adc_note_isr_window(int32 sum, uint8 n);
uint8 psoc_adc_take_isr_window(int32 *out_sum, uint8 *out_n);

#endif
