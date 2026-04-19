/* debug.c
 * Compact binary debug event system.
 */

#include "debug.h"
#include "transport_usb.h"
#include "transport_uart.h"

static uint8 s_enabled = 0u;
static uint8 s_channel = DBG_CH_UART;   /* default: UART only */

void Debug_Init(void)
{
    s_enabled = 0u;
    s_channel = DBG_CH_UART;
}

void Debug_SetEnabled(uint8 en)
{
    s_enabled = (en != 0u) ? 1u : 0u;
}

uint8 Debug_IsEnabled(void)
{
    return s_enabled;
}

void Debug_SetChannel(uint8 ch)
{
    if (ch <= DBG_CH_BOTH) { s_channel = ch; }
}

uint8 Debug_GetChannel(void)
{
    return s_channel;
}

void Debug_Send(uint8 event, const uint8 *payload, uint8 len)
{
    if (!s_enabled) { return; }

    if (s_channel == DBG_CH_USB || s_channel == DBG_CH_BOTH)
    {
        TransportUSB_SendDebug(event, payload, len);
    }
    if (s_channel == DBG_CH_UART || s_channel == DBG_CH_BOTH)
    {
        TransportUART_SendDebug(event, payload, len);
    }
}
