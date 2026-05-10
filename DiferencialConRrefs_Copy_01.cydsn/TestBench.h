/* TestBench.h
 * Testbench analog front-end control — NOT THESIS PERMANENT
 *
 * Manages the hardware specific to the filter comparison testbench:
 *   - MuxIn:    selects Geophone or VDAC as ADC_DelSig input
 *   - MuxOut:   selects Filter1(TIA) or Filter2(Opa) output to ADC_DelSig
 *   - VDACIn_p/n: differential 8-bit DAC for test signal generation
 *   - ADC_SAR:  direct geophone monitoring (independent of filter path)
 *   - Timer_VDAC + isr_VDAC: drives VDAC at 3000 Hz from sequence buffer
 *
 * NOTE: Requires Timer_VDAC component (3000 Hz) and isr_VDAC interrupt
 *       to be added manually in PSoC Creator TopDesign before building.
 */

#ifndef TESTBENCH_H
#define TESTBENCH_H

#include "cytypes.h"

/* MuxIn channel assignments */
#define TB_INPUT_GEOPHONE  0u
#define TB_INPUT_VDAC      1u

/* MuxOut channel assignments */
#define TB_OUTPUT_FILTER1_TIA  0u
#define TB_OUTPUT_FILTER2_OPA  1u

/* Initialize all testbench hardware.
 * Starts MuxIn, MuxOut, VDACs, ADC_SAR, and VDAC timer.
 * Defaults to geophone input + Filter1 output. */
void TestBench_Init(void);

/* Select the analog input source.
 * source: TB_INPUT_GEOPHONE or TB_INPUT_VDAC */
void TestBench_SetInput(uint8 source);

/* Select which filter output is routed to ADC_DelSig.
 * output: TB_OUTPUT_FILTER1_TIA or TB_OUTPUT_FILTER2_OPA */
void TestBench_SetOutput(uint8 output);

/* Load a VDAC waveform sequence.
 * data: array of uint8 values centered on 128 (DC midpoint).
 * len:  number of samples (1..TB_VDAC_MAX_LEN).
 * VDAC_n is computed as 256-VDAC_p automatically in the ISR. */
void TestBench_SetVdacSeq(const uint8 *data, uint16 len);

/* Periodic task — call from main loop.
 * Currently no polling needed; reserved for future use. */
void TestBench_Task(void);

#endif /* TESTBENCH_H */
