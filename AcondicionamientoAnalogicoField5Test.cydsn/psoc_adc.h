#ifndef PSOC_ADC_H
#define PSOC_ADC_H

#include "project.h"

/* Única fuente de verdad para la tasa nativa. El build falla si el customizer
 * vuelve a quedar con configuraciones distintas. program_psoc.ps1 además las
 * fuerza con adc_2604.params antes de generar el HEX. */
#define PSOC_ADC_NATIVE_FS_HZ 2604u
#if (ADC_CF_2V5_SRATE != PSOC_ADC_NATIVE_FS_HZ) || \
    (ADC_CF_0V512_SRATE != PSOC_ADC_NATIVE_FS_HZ) || \
    (ADC_CF_1V024_SRATE != PSOC_ADC_NATIVE_FS_HZ) || \
    (ADC_CF_0V625_SRATE != PSOC_ADC_NATIVE_FS_HZ)
#error "Las cuatro configuraciones ADC deben usar PSOC_ADC_NATIVE_FS_HZ"
#endif

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

/* Config de usuario del ADC (rango de entrada). Se exponen las 4 configs
 * del componente: ADC_CF_2V5 (1, ±2.5 V), ADC_CF_0V512 (2, ±0.512 V),
 * ADC_CF_1V024 (3, ±1.024 V) y ADC_CF_0V625 (4, ±0.625 V), las cuatro a
 * 2604 SPS nativos / 18 bits para que la Fs base del sistema sea única.
 * RAM only: al reset se vuelve a 2V5. */
uint8  psoc_adc_get_config(void);
uint8  psoc_adc_set_config(uint8 cfg);

/* Factor de decimación con promedio, aplicado en el ISR de captura (main.c)
 * después de la selección RAW/FIR. 1 = sin decimar (2604 Hz tal cual).
 * RAM only: al reset vuelve a 1. Rango válido 1..PSOC_ADC_DECIMATION_MAX. */
#define PSOC_ADC_DECIMATION_MAX 100u
uint8  psoc_adc_get_decimation(void);
uint8  psoc_adc_set_decimation(uint8 factor);

/* Fs efectiva = 2604 / factor_de_decimación (redondeada hacia abajo). */
uint16 psoc_adc_effective_fs_hz(void);

/* Mientras el override esté activo, psoc_adc_select_capture_config fuerza
 * ADC_CF_2V5: la calibración (targets en counts de ±2.5 V,
 * calibration_tables.h) debe correr siempre en ese rango. */
void psoc_adc_select_calibration_config(void);
void psoc_adc_cal_override(uint8 force_2v5);
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
