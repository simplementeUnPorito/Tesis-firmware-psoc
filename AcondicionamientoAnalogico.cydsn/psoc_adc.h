#ifndef PSOC_ADC_H
#define PSOC_ADC_H

#include "project.h"

int32 psoc_adc_counts_right_aligned(int32 adc_counts);
void psoc_adc_select_capture_config(void);
void psoc_adc_clear_isr_sample(void);
void psoc_adc_note_isr_sample(int32 counts);
uint8 psoc_adc_take_isr_sample(int32 *out_counts);

#endif
