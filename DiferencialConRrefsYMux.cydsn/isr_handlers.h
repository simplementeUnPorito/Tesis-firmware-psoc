/* isr_handlers.h
 * Interrupt service routine prototypes — THESIS PERMANENT (core handlers)
 *
 * Contains both thesis-permanent and testbench ISR handlers.
 * The isr_VDAC_Handler is testbench-only; remove it in final firmware.
 */

#ifndef ISR_HANDLERS_H
#define ISR_HANDLERS_H

#include "cytypes.h"

/* isr_DMA_Filter: fires when DMA_Filter completes (new filtered sample ready).
 * At this point g_raw_delsig and g_filtered are both valid.
 * Sets g_sample_ready = 1 for the main loop to consume.
 * THESIS PERMANENT */
CY_ISR_PROTO(isr_DMA_Filter_Handler);

/* isr_SAR: fires when DMA_SAR completes (new SAR sample in g_sar_latest).
 * Minimal handler — value is already in g_sar_latest via DMA.
 * THESIS PERMANENT (in geophone mode) */
CY_ISR_PROTO(isr_SAR_Handler);

/* isr_VDAC_Handler: fires from Timer_VDAC at 3000 Hz via isr_Timer component.
 * Advances the VDAC sequence and updates VDACIn_p/n outputs.
 * Registered with isr_Timer_StartEx(isr_VDAC_Handler) in TestBench_Init().
 * NOT THESIS PERMANENT */
CY_ISR_PROTO(isr_VDAC_Handler);

#endif /* ISR_HANDLERS_H */
