/* transport_spi.h
 * SPI Slave transport — PSoC 5LP side of the PSoC ↔ ESP32 link.
 *
 * Role: PSoC acts as SPI SLAVE.  The ESP32 is the SPI master and drives
 * the clock, SS, MOSI.  PSoC drives MISO and a DATA_READY GPIO.
 *
 * Protocol overview:
 *   1. PSoC accumulates SPI_BATCH_SAMPLES samples in a ring buffer.
 *   2. When the ring is full, PSoC raises DATA_READY pin (active-high).
 *   3. ESP32 asserts SS and clocks out SPI_FRAME_BYTES.
 *   4. PSoC loads the TX FIFO byte-by-byte in its SPI ISR.
 *   5. On SS deassertion, PSoC clears DATA_READY and resets the ring.
 *
 * Frame layout (SPI_FRAME_BYTES = 6 + SPI_BATCH_SAMPLES * 10):
 *   [0]     0xAB  (frame start marker)
 *   [1]     n_samples_lo  (always == SPI_BATCH_SAMPLES)
 *   [2]     n_samples_hi
 *   [3]     seq_lo   (batch sequence counter, wraps at 65535)
 *   [4]     seq_hi
 *   [5]     flags    (global: bit0=vdac_mode, bit1=filter_sel, bit2=fir_loaded)
 *   Then, for each sample:
 *   [+0..1] raw_input  (int16 LE — SAR or VDAC deviation)
 *   [+2..4] post_analog (int24 LE — ADC_DelSig before DFB)
 *   [+5..7] post_digital (int24 LE — after DFB + optional FIR)
 *   [+8]    gain_byte  (low nibble=cfg, high nibble=log2(buf_gain))
 *   [+9]    sample_flags (per-sample: same bit encoding as 0xAA packet)
 *
 * Hardware wiring:
 *   PSoC MISO  ──  ESP32 MISO  (GPIO 19 on VSPI)
 *   PSoC MOSI  ──  ESP32 MOSI  (GPIO 23)
 *   PSoC SCLK  ──  ESP32 SCK   (GPIO 18)
 *   PSoC SS    ──  ESP32 SS    (GPIO 5)
 *   PSoC P0[0] ──  ESP32 GPIO 4  (DATA_READY, active high)
 *
 * SPI parameters:
 *   Mode    : CPOL=0, CPHA=0 (Mode 0)
 *   Bitorder: MSB first
 *   Clock   : max 4 MHz (PSoC slave requires ≥ 20× system clock ratio)
 *   Word    : 8-bit
 *
 * PSoC Creator component: SPIS_v2_50 (or SPIS_v2_70)
 *   Internal name in project: "SPIS_Ext"
 *   TX buffer depth: 4 (hardware FIFO) + software ring
 *   RX buffer depth: 4 (hardware FIFO) — only used for future CMD path
 */

#ifndef TRANSPORT_SPI_H
#define TRANSPORT_SPI_H

#include "cytypes.h"
#include "transport.h"

/* Number of samples accumulated per SPI batch.
 * At 1500 Hz, 30 samples = 20 ms per batch (50 batches/s).
 * Frame size = 6 + 30*10 = 306 bytes.  At 4 MHz SPI: 612 μs transfer. */
#define SPI_BATCH_SAMPLES  30u
#define SPI_FRAME_BYTES    (6u + SPI_BATCH_SAMPLES * 10u)   /* 306 */
#define SPI_FRAME_MARKER   0xABu

/* DATA_READY GPIO — adjust pin if schematic changes */
#define SPI_DRDY_PORT      0u      /* P0[0] */
#define SPI_DRDY_PIN       0u

/* External Transport_t for registration */
extern Transport_t spi_transport;

/* Direct init / send (used inside transport vtable) */
void TransportSPI_Init(void);
void TransportSPI_Task(void);
uint8 TransportSPI_IsReady(void);
void TransportSPI_SendSample(int16 raw, int32 post_analog, int32 post_digital,
                              uint8 flags, uint8 gain_byte);

#endif /* TRANSPORT_SPI_H */
