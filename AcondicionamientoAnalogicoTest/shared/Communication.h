/* Communication.h
 * Multi-transport communication manager — THESIS PERMANENT
 *
 * ── Data packet (PSoC -> PC), 12 bytes, header 0xAA ──────────────────────
 *   [0]  0xAA
 *   [1]  flags      bit0=mux_in (0=Geo,1=VDAC)  bit1=mux_out (0=F1,1=F2)
 *   [2]  raw_lo     int16 raw_input LSB
 *   [3]  raw_hi     int16 raw_input MSB
 *   [4]  analog_b0  post-analog byte 0 (LSB)
 *   [5]  analog_b1
 *   [6]  analog_b2  (MSB / sign byte)
 *   [7]  digital_b0 post-digital byte 0
 *   [8]  digital_b1
 *   [9]  digital_b2
 *  [10]  gain_byte  low nibble=cfg(1-4), high nibble=log2(buf_gain) (0-3)
 *  [11]  crc        XOR of bytes [0..10]
 *
 * ── State packet (PSoC -> PC), 10 bytes, header 0xCC ─────────────────────
 *   [0]  0xCC
 *   [1]  7           payload length
 *   [2]  adc_cfg     1-4
 *   [3]  buf_gain    1/2/4/8
 *   [4]  streaming   0/1
 *   [5]  debug_en    0/1
 *   [6]  debug_ch    0=USB 1=UART 2=BOTH
 *   [7]  vdac_mode   0=Geophone 1=VDAC
 *   [8]  filter_sel  0=Filter1 1=Filter2
 *   [9]  crc         XOR of bytes [0..8]
 *
 * ── Debug packet (PSoC -> PC), 4+ bytes, header 0xDD ─────────────────────
 *   [0]  0xDD
 *   [1]  event code  (see debug.h)
 *   [2]  payload len
 *   [3+] payload
 *   [-]  crc         XOR of all preceding bytes
 *
 * ── Command frame (PC -> PSoC): [0xBB][CMD][LEN][payload...] ─────────────
 *   CMD_START       0x01  LEN=0            — begin streaming
 *   CMD_STOP        0x02  LEN=0            — halt streaming
 *   CMD_SET_MUX_IN  0x03  LEN=1  [0/1]    — 0=Geophone, 1=VDAC
 *   CMD_SET_MUX_OUT 0x04  LEN=1  [0/1]    — 0=Filter1, 1=Filter2
 *   CMD_SET_ADC     0x05  LEN=2  [cfg][bg] — cfg:1-4, bg:1/2/4/8
 *   CMD_VDAC_LOAD   0x06  [len_hi][len_lo][data...] — 2-byte length, no LEN field
 *   CMD_SET_DEBUG   0x07  LEN=1  [0/1]    — enable/disable debug
 *   CMD_SET_DBG_CH  0x08  LEN=1  [0/1/2]  — debug channel USB/UART/BOTH
 *   CMD_GET_STATE   0x09  LEN=0            — request state packet immediately
 *   CMD_LOAD_FIR    0x0A  [len_hi][len_lo][c0_lo][c0_hi]...[cN_lo][cN_hi]
 *                         2-byte length (like VDAC_LOAD); each coefficient is
 *                         2 bytes little-endian int16 Q1.15.
 *                         max payload = 255 * 2 = 510 bytes.
 *   CMD_FIR_CLEAR   0x0B  LEN=0            — disable FIR, pass-through mode
 */

#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include "cytypes.h"
#include "transport.h"

/* Command codes */
#define COMM_CMD_START        0x01u
#define COMM_CMD_STOP         0x02u
#define COMM_CMD_SET_MUX_IN   0x03u
#define COMM_CMD_SET_MUX_OUT  0x04u
#define COMM_CMD_SET_ADC      0x05u
#define COMM_CMD_VDAC_LOAD    0x06u
#define COMM_CMD_SET_DEBUG    0x07u
#define COMM_CMD_SET_DBG_CH   0x08u
#define COMM_CMD_GET_STATE    0x09u
#define COMM_CMD_LOAD_FIR     0x0Au  /* 2-byte length: n_taps*2 bytes of Q1.15 int16 */
#define COMM_CMD_FIR_CLEAR    0x0Bu  /* LEN=0: disable FIR, pass-through */

/* Packet / frame markers */
#define COMM_PKT_DATA_HDR   0xAAu
#define COMM_PKT_STATE_HDR  0xCCu
#define COMM_CMD_MARKER     0xBBu
#define COMM_PKT_DATA_SIZE  12u

/* Initialize all registered transports. */
void Communication_Init(void);

/* Register a data transport for parallel sample dispatch (max COMM_MAX_TRANSPORTS). */
void Communication_RegisterTransport(Transport_t *t);

/* Poll: handle USB RX (command parsing), run all transport tasks. */
void Communication_Task(void);

/* Dispatch one sample frame to all enabled, ready transports. */
void Communication_SendSample(int16 raw_input, int32 post_analog,
                               int32 post_digital, uint8 flags);

/* Send PSoC full-state packet over USB CDC. Called after every command. */
void Communication_SendState(void);

/* 1 if USB CDC is enumerated. */
uint8 Communication_IsConnected(void);

/* 1 if host has sent CMD_START. */
uint8 Communication_IsStreaming(void);

#endif /* COMMUNICATION_H */
