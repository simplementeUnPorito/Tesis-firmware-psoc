/* AnalogToDigital.h
 * ADC_DelSig + DFB Filter pipeline manager — THESIS PERMANENT
 *
 * ADC_DelSig configurations (set in PSoC Creator, selected at runtime):
 *   CFG1: ±0.512 V, 18-bit, 1464 SPS  — buffer gain MUST stay 1
 *   CFG2: ±1.024 V, 17-bit, 1488 SPS  — buffer gain default 8, adjustable
 *   CFG3: ±2.048 V, 17-bit, 1488 SPS  — buffer gain default 8, adjustable
 *   CFG4: ±6.144 V, 17-bit, 1488 SPS  — buffer gain default 8, adjustable
 *
 * Buffer gain effect: effective_range = BASE_RANGE[cfg] × 8 / buf_gain
 * (CFG1 is exempt — its range is always ±0.512 V with gain=1)
 */

#ifndef ANALOG_TO_DIGITAL_H
#define ANALOG_TO_DIGITAL_H

#include "cytypes.h"

/* ADC configuration indices (match PSoC Creator ADC_DelSig_CFGx constants) */
#define ATD_CFG_1   1u   /* ±0.512 V, 18-bit — buf_gain locked to 1 */
#define ATD_CFG_2   2u   /* ±1.024 V, 17-bit */
#define ATD_CFG_3   3u   /* ±2.048 V, 17-bit */
#define ATD_CFG_4   4u   /* ±6.144 V, 17-bit */

/* Buffer gain options (valid for CFG2-4; CFG1 only accepts ATD_BGAIN_1) */
#define ATD_BGAIN_1   1u
#define ATD_BGAIN_2   2u
#define ATD_BGAIN_4   4u
#define ATD_BGAIN_8   8u

#define ATD_SAMPLE_RATE_HZ  1500u

/* Initialize DMA channels. Call before AnalogToDigital_Start(). */
void AnalogToDigital_Init(void);

/* Start continuous ADC conversion and digital filter. */
void AnalogToDigital_Start(void);

/* Stop conversion and filter. */
void AnalogToDigital_Stop(void);

/* Set both ADC config and buffer gain atomically.
 * cfg: ATD_CFG_1..ATD_CFG_4
 * buf_gain: ATD_BGAIN_1/2/4/8
 * Constraint: if cfg==ATD_CFG_1, buf_gain is forced to 1 regardless. */
void AnalogToDigital_SetADC(uint8 cfg, uint8 buf_gain);

/* Return current config (1-4). */
uint8 AnalogToDigital_GetCfg(void);

/* Return current buffer gain (1, 2, 4, or 8). */
uint8 AnalogToDigital_GetBufGain(void);

/* ── Software FIR stage (second filter, after DFB hardware filter) ──────────
 *
 * Up to ATD_FIR_MAX_TAPS taps, Q1.15 int16 coefficients.
 * Delay line is a power-of-2 circular buffer (256 slots) for efficient
 * uint8 index arithmetic — no modulo needed.
 *
 * Typical workflow from Python:
 *   1. Design filter (scipy.signal), convert coefficients to Q1.15 int16.
 *   2. Send CMD_LOAD_FIR (0x0A) with raw coefficient bytes (2 bytes/tap, LE).
 *   3. PSoC calls AnalogToDigital_FIR_Load(); g_fir_loaded is set to 1.
 *   4. Main loop calls AnalogToDigital_FIR_Process(g_filtered) every sample.
 *   5. Communication_SendSample uses g_fir_out as post_digital channel.
 */

#define ATD_FIR_MAX_TAPS  255u   /* max taps; delay line is 256 for uint8 wrap */

/* Output of the software FIR stage.  Updated every sample when loaded.
 * When no FIR is loaded (g_fir_loaded == 0), mirrors g_filtered. */
extern volatile int32  g_fir_out;

/* 1 when valid FIR coefficients are loaded and processing is active. */
extern volatile uint8  g_fir_loaded;

/* Load FIR filter.
 *   coeffs  : Q1.15 signed int16 array (values in [-32768 .. 32767]).
 *             Normalize so sum(|h|) ≤ 1.0 to avoid overflow.
 *   n_taps  : number of coefficients (1..ATD_FIR_MAX_TAPS).
 *             Pass 0 to disable (same as FIR_Clear). */
void AnalogToDigital_FIR_Load(const int16 *coeffs, uint8 n_taps);

/* Apply FIR to one 24-bit sample; result stored in g_fir_out.
 * Pass-through (g_fir_out = sample) when g_fir_loaded == 0. */
void AnalogToDigital_FIR_Process(int32 sample);

/* Disable FIR, clear delay line, reset g_fir_loaded to 0. */
void AnalogToDigital_FIR_Clear(void);

#endif /* ANALOG_TO_DIGITAL_H */
