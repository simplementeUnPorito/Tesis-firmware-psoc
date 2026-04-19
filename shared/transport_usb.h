/* transport_usb.h
 * USB CDC transport — primary data link to PC.
 */

#ifndef TRANSPORT_USB_H
#define TRANSPORT_USB_H

#include "cytypes.h"
#include "transport.h"

/* Registered Transport_t instance. Register in main.c. */
extern Transport_t usb_transport;

/* Send one debug packet over USB CDC.
 * Called by debug.c when channel includes DBG_CH_USB. */
void TransportUSB_SendDebug(uint8 event, const uint8 *payload, uint8 len);

/* Send the PSoC full-state packet [0xCC] so Python can sync its UI.
 * Called by Communication.c after every command and on USB enumeration. */
void TransportUSB_SendState(uint8 adc_cfg, uint8 buf_gain,
                            uint8 streaming, uint8 debug_en,
                            uint8 debug_ch, uint8 vdac_mode,
                            uint8 filter_sel);

/* 1 if USB is enumerated and CDC TX buffer is ready. */
uint8 TransportUSB_IsConnected(void);

#endif /* TRANSPORT_USB_H */
