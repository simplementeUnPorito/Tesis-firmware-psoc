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

/* Lote como muestras individuales (no la suma) — quedó sin uso desde que el
 * PI de calibración (calibration.c) pasó a leer el Filter de hardware en vez
 * de un FIR por software prellenado a partir de un lote crudo; se deja
 * definida por si hace falta retomar ese camino. */
void  psoc_adc_clear_isr_block(void);
void  psoc_adc_note_isr_block(const int32 *samples, uint8 n);
uint8 psoc_adc_take_isr_block(int32 *out_samples, uint8 *out_n, uint8 max_n);

/* Última muestra filtrada por el Filter de hardware (DFB), entregada por
 * isr_DMA_Filter_RAM_Handler (main.c) vía DMA_Filter_RAM cada vez que el
 * Canal A del Filter produce una salida nueva. El PI de calibración
 * (cal_pi_run_service, calibration.c) la usa directo como "fir_output" —
 * el FIR ya lo calculó el hardware, no hace falta acumular nada en software. */
void  psoc_adc_clear_isr_filtered_sample(void);
void  psoc_adc_note_isr_filtered_sample(int32 counts);
uint8 psoc_adc_take_isr_filtered_sample(int32 *out_counts);

#endif
