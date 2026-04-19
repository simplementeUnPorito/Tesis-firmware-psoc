/* isr_handlers.c
 * ISR implementations
 */

#include "project.h"
#include "isr_handlers.h"
#include "buffers.h"
#include "tb_buffers.h"

/* isr_DMA_Filter: new (raw, filtered) sample pair is ready.
 * Called at 1500 Hz. Keep short — just set the flag; main loop handles USB TX. */
CY_ISR(isr_DMA_Filter_Handler)
{
    g_sample_ready = 1u;
}

/* isr_SAR: DMA_SAR has written the latest SAR value to g_sar_latest.
 * Nothing else needed; the DMA already did the work. */
CY_ISR(isr_SAR_Handler)
{
    /* Intentionally empty — g_sar_latest is updated by DMA_SAR. */
}

/* isr_VDAC: advance VDAC sequence at 3000 Hz.
 * Applies differential: VDAC_p = seq[idx], VDAC_n = 256 - seq[idx].
 * Saves the signed deviation from midpoint as g_vdac_last_val for telemetry. */
CY_ISR(isr_VDAC_Handler)
{
    uint8 val;
    uint8 val_n;

    /* Read Timer_VDAC status register to clear the TC interrupt flag */
    (void)Timer_VDAC_ReadStatusRegister();

    if (g_vdac_mode == 1u)  /* Only output when in VDAC mode */
    {
        val   = g_vdac_seq[g_vdac_idx];
        val_n = (uint8)((uint16)256u - (uint16)val);  /* Complementary */

        VDACIn_p_SetValue(val);
        VDACIn_n_SetValue(val_n);

        /* Store signed deviation (centered on 128) for raw_input telemetry */
        g_vdac_last_val = (int8)((int16)val - 128);

        /* Advance index with wrap */
        g_vdac_idx++;
        if (g_vdac_idx >= g_vdac_len) { g_vdac_idx = 0u; }
    }
}
