/* main.c — Geophone Filter Testbench Firmware
 *
 * Boot sequence:
 *   1. Interrupts on
 *   2. Debug system (UART) — first so boot events can be logged
 *   3. ISR registration
 *   4. Register USB transport (UART transport disabled by default)
 *   5. Init all hardware modules; send debug event per module
 *   6. Start ADC pipeline
 *   7. Main loop: Communication_Task + sample dispatch
 *
 * State sync: after USB enumerates, Communication_SendState() is called once
 * so Python immediately knows the PSoC's current config without guessing.
 */

#include "project.h"
#include "buffers.h"
#include "tb_buffers.h"
#include "AnalogToDigital.h"
#include "Communication.h"
#include "TestBench.h"
#include "isr_handlers.h"
#include "debug.h"
#include "transport_usb.h"
#include "transport_uart.h"

int main(void)
{
    uint8 was_connected = 0u;
    uint8 dbg_pld[2];

    CyGlobalIntEnable;

    /* Debug system first — events go via USB CDC (same channel as data) */
    Debug_Init();
    Debug_SetEnabled(1u);
    Debug_SetChannel(DBG_CH_USB);

    /* Register ISRs */
    isr_DMA_Filter_StartEx(isr_DMA_Filter_Handler);

    /* Register data transports (USB always; UART sample stream disabled) */
    Communication_RegisterTransport(&usb_transport);
    /* Future: Communication_RegisterTransport(&uart_transport); */

    /* Initialize hardware — debug events emitted per module */
    AnalogToDigital_Init();
    dbg_pld[0] = g_adc_cfg;
    dbg_pld[1] = g_adc_buf_gain;
    Debug_Send(DBG_EVT_BOOT_ADC, dbg_pld, 2u);

    Debug_Send(DBG_EVT_BOOT_DMA, (void *)0u, 0u);

    Communication_Init();   /* starts UART_PC and USBFS */

    TestBench_Init();
    Debug_Send(DBG_EVT_BOOT_TB, (void *)0u, 0u);

    AnalogToDigital_Start();

    for(;;)
    {
        Communication_Task();

        /* Send boot event + state once when USB first enumerates */
        if (!was_connected && Communication_IsConnected())
        {
            was_connected = 1u;
            Debug_Send(DBG_EVT_BOOT_USB, (void *)0u, 0u);
            Communication_SendState();
        }
        else if (was_connected && !Communication_IsConnected())
        {
            was_connected = 0u;
        }

        if (g_sample_ready)
        {
            g_sample_ready = 0u;

            /* Software FIR: always run so delay line stays current.
             * When g_fir_loaded==0, FIR_Process simply copies g_filtered. */
            AnalogToDigital_FIR_Process(g_filtered);

            if (Communication_IsStreaming())
            {
                int16 raw_input;
                uint8 flags;
                int32 post_digital;

                flags = (uint8)((g_vdac_mode  & 0x01u)       |
                                ((g_filter_sel & 0x01u) << 1u));

                raw_input = (g_vdac_mode == 0u)
                          ? g_sar_latest
                          : (int16)g_vdac_last_val;

                /* post_digital: use FIR output when a filter is loaded,
                 * otherwise use raw DFB output (pass-through). */
                post_digital = g_fir_loaded ? g_fir_out : g_filtered;

                Communication_SendSample(raw_input, g_raw_delsig,
                                         post_digital, flags);
            }
        }
    }
}
