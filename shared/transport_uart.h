/* transport_uart.h
 * UART_PC transport — debug output + future sample data expansion.
 *
 * Currently:
 *   - Debug packets [0xDD]... sent via UART_PC (used by debug.c)
 *   - send_sample stub: no-op (future: compact binary frame for ESP32)
 *
 * To add UART sample streaming (e.g. to ESP32):
 *   Implement TransportUART_SendSample() with a compact frame format
 *   and register uart_transport in main.c.
 */

#ifndef TRANSPORT_UART_H
#define TRANSPORT_UART_H

#include "cytypes.h"
#include "transport.h"

/* Registered Transport_t instance (disabled by default). */
extern Transport_t uart_transport;

/* Send one debug packet over UART_PC.
 * Called by debug.c when channel includes DBG_CH_UART. */
void TransportUART_SendDebug(uint8 event, const uint8 *payload, uint8 len);

#endif /* TRANSPORT_UART_H */
