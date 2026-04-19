/* tb_buffers.c
 * Testbench-only data buffer definitions — NOT THESIS PERMANENT
 */

#include "tb_buffers.h"

volatile int16 g_sar_latest  = 0;

uint8  g_vdac_seq[TB_VDAC_MAX_LEN];
volatile uint16 g_vdac_len   = 1u;   /* Default: single-sample flat */
volatile uint16 g_vdac_idx   = 0u;
volatile uint8  g_vdac_mode  = 0u;   /* Default: geophone input */
volatile uint8  g_filter_sel = 0u;   /* Default: Filter1 / TIA output */
volatile int8   g_vdac_last_val = 0;
