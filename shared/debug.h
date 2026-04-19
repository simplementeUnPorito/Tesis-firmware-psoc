/* debug.h
 * Compact binary debug event system.
 *
 * Packet format (PSoC -> PC): [0xDD][EVENT][LEN][payload...][CRC]
 *   CRC = XOR of all bytes including header.
 *   Minimum packet: 4 bytes (no payload).
 *
 * Debug channel is selectable at runtime (USB CDC, UART_PC, or both).
 * Enabled/disabled independently of the data streaming state.
 */

#ifndef DEBUG_H
#define DEBUG_H

#include "cytypes.h"

/* Debug packet header — distinct from 0xAA (data) and 0xBB (command) */
#define DBG_HEADER          0xDDu

/* Debug channel selection */
#define DBG_CH_USB          0u
#define DBG_CH_UART         1u
#define DBG_CH_BOTH         2u

/* Event codes */
#define DBG_EVT_BOOT_ADC    0x10u  /* ADC init OK       payload: [cfg][buf_gain]  */
#define DBG_EVT_BOOT_DMA    0x11u  /* DMA chains ready  payload: none             */
#define DBG_EVT_BOOT_USB    0x12u  /* USB enumerated    payload: none             */
#define DBG_EVT_BOOT_TB     0x13u  /* TestBench init OK payload: none             */
#define DBG_EVT_STREAM_ON   0x20u  /* Streaming started payload: none             */
#define DBG_EVT_STREAM_OFF  0x21u  /* Streaming stopped payload: none             */
#define DBG_EVT_CMD_RX      0x30u  /* Command received  payload: [cmd_code]       */
#define DBG_EVT_DMA_ERR     0x40u  /* DMA error         payload: [chain_id 0-3]   */
#define DBG_EVT_ADC_ERR     0x41u  /* ADC lost lock     payload: none             */
#define DBG_EVT_VDAC_LOADED 0x50u  /* VDAC loaded       payload: [len_hi][len_lo] */
#define DBG_EVT_ADC_CHG     0x60u  /* ADC config change payload: [cfg][buf_gain]  */

/* Initialize debug system (default: UART channel, disabled). */
void Debug_Init(void);

/* Enable or disable debug event transmission. */
void Debug_SetEnabled(uint8 en);
uint8 Debug_IsEnabled(void);

/* Set transmission channel: DBG_CH_USB, DBG_CH_UART, or DBG_CH_BOTH. */
void Debug_SetChannel(uint8 ch);
uint8 Debug_GetChannel(void);

/* Transmit one debug event. No-op if debug is disabled. */
void Debug_Send(uint8 event, const uint8 *payload, uint8 len);

#endif /* DEBUG_H */
