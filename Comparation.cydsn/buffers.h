/* buffers.h
 * Core data pipeline shared variables — THESIS PERMANENT
 *
 * Dependency graph:
 *   DMA_DelSig_RAM  --> g_raw_delsig
 *   DMA_Filter      --> g_filtered
 *   isr_DMA_Filter  sets g_sample_ready = 1 when both are valid
 */

#ifndef BUFFERS_H
#define BUFFERS_H

#include "cytypes.h"

/* Raw ADC_DelSig sample BEFORE digital filter.
 * 17/18-bit signed, right-aligned in 32-bit word.
 * Written by DMA_DelSig_RAM, read by Communication layer. */
extern volatile int32 g_raw_delsig;

/* Filter DFB output sample AFTER notch filter.
 * 24-bit signed result in lower 3 bytes of int32.
 * Written by DMA_Filter, read by Communication layer. */
extern volatile int32 g_filtered;

/* Set to 1 by isr_DMA_Filter when a new sample pair is ready.
 * Main loop clears it after consumption. */
extern volatile uint8 g_sample_ready;

/* Currently active ADC_DelSig configuration (1–4).
 *   CFG1: ±0.512 V, 18-bit, buf_gain MUST be 1
 *   CFG2: ±1.024 V, 17-bit, buf_gain default 8
 *   CFG3: ±2.048 V, 17-bit, buf_gain default 8
 *   CFG4: ±6.144 V, 17-bit, buf_gain default 8 */
extern volatile uint8 g_adc_cfg;

/* Currently applied ADC_DelSig buffer gain (1, 2, 4, or 8).
 * Always 1 for CFG1. CFG2–4 support 1/2/4/8 via ADC_DelSig_SetBufferGain(). */
extern volatile uint8 g_adc_buf_gain;

#endif /* BUFFERS_H */
