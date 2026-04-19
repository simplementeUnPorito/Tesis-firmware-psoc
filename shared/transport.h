/* transport.h
 * Data-transport interface for parallel sample dispatch.
 *
 * Register implementations with Communication_RegisterTransport().
 * Communication_SendSample() iterates all enabled transports.
 *
 * Current transports: USB CDC (usb_transport in transport_usb.c)
 * Future stubs:       UART-ESP32, SPI, I2C, BLE
 */

#ifndef TRANSPORT_H
#define TRANSPORT_H

#include "cytypes.h"

#define COMM_MAX_TRANSPORTS  4u

typedef struct {
    void   (*init)(void);
    void   (*task)(void);
    uint8  (*is_ready)(void);
    void   (*send_sample)(int16 raw_input, int32 post_analog,
                          int32 post_digital, uint8 flags, uint8 gain_byte);
    uint8  enabled;
    const char *name;
} Transport_t;

#endif /* TRANSPORT_H */
