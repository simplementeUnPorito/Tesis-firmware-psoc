/* buffers.c
 * Core data pipeline shared variable definitions — THESIS PERMANENT
 */

#include "buffers.h"

volatile int32 g_raw_delsig   = 0;
volatile int32 g_filtered     = 0;
volatile uint8 g_sample_ready = 0u;
volatile uint8 g_adc_cfg      = 4u;   /* Default: CFG4 (±6.144 V) */
volatile uint8 g_adc_buf_gain = 8u;   /* Default: buffer gain 8 (CFG4 as in PSoC Creator) */
