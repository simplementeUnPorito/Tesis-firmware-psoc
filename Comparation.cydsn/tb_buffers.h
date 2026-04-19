/* tb_buffers.h
 * Testbench-only data buffers — NOT THESIS PERMANENT
 *
 * Holds SAR geophone samples and VDAC playback state.
 * Remove or stub-out these when migrating to final thesis firmware.
 */

#ifndef TB_BUFFERS_H
#define TB_BUFFERS_H

#include "cytypes.h"

/* Maximum number of points in a VDAC waveform sequence.
 * At 3000 Hz update rate, this gives 1 second of signal. */
#define TB_VDAC_MAX_LEN  3000u

/* Latest ADC_SAR result (geophone raw).
 * 12-bit differential, signed (positive = Geophone_p > Geophone_m).
 * Written by DMA_SAR. Only valid when g_vdac_mode == 0 (geophone mode). */
extern volatile int16 g_sar_latest;

/* VDAC output waveform sequence.
 * Values are 8-bit unsigned centered on 128 (DC midpoint).
 * VDAC_p = g_vdac_seq[idx], VDAC_n = 256 - g_vdac_seq[idx].
 * Loaded via USB command CMD_VDAC_LOAD. */
extern uint8  g_vdac_seq[TB_VDAC_MAX_LEN];
extern volatile uint16 g_vdac_len;   /* Active sequence length (1..TB_VDAC_MAX_LEN) */
extern volatile uint16 g_vdac_idx;   /* Current playback index, wraps at g_vdac_len */

/* Current input source selection.
 * 0 = Geophone (MuxIn channel 0), 1 = VDAC (MuxIn channel 1). */
extern volatile uint8 g_vdac_mode;

/* Current output filter selection.
 * 0 = Filter1/TIA (MuxOut channel 0), 1 = Filter2/Opa (MuxOut channel 1). */
extern volatile uint8 g_filter_sel;

/* Echo of the last VDAC value applied (for transmission as raw_input in VDAC mode). */
extern volatile int8 g_vdac_last_val;

#endif /* TB_BUFFERS_H */
