#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include "cytypes.h"

void UART_Init(void);
void send_config(void);
void process_rx_byte(uint8 rx);

#endif /* COMMUNICATION_H */
