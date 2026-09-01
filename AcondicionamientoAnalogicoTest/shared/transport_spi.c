/* transport_spi.c
 * SPI Slave transport implementation — PSoC 5LP side.
 *
 * Depends on PSoC Creator SPIS component named "SPIS_Ext".
 * Requires: DATA_READY output pin P0[0] named "SPI_DataReady" in TopDesign.
 *
 * Key design decisions:
 *   - Sample ring buffer is filled from Communication_SendSample() (main loop).
 *   - When ring is full, DATA_READY is raised.
 *   - SPIS TX ISR pre-loads the next frame byte into the FIFO so the ESP32
 *     can clock at full speed without stalls.
 *   - Ring swap: on SS deassertion (SPIS_STS_SPI_DONE), the completed ring
 *     is retired and a new ring starts.
 */

#include "project.h"
#include "transport_spi.h"
#include "buffers.h"
#include "tb_buffers.h"
#include "AnalogToDigital.h"

/* ── Ring buffer ─────────────────────────────────────────────────────────────
 * Double-buffered: while ESP reads ring[tx_idx], we fill ring[fill_idx]. */

static uint8  s_ring[2][SPI_FRAME_BYTES];
static uint8  s_fill_idx = 0u;   /* ring currently being filled (0 or 1) */
static uint8  s_tx_idx   = 0u;   /* ring currently being transmitted    */
static uint16 s_fill_pos = 0u;   /* byte position inside fill ring      */
static uint16 s_tx_pos   = 0u;   /* byte position inside tx ring        */
static uint16 s_seq      = 0u;   /* batch sequence counter (wraps)      */
static uint8  s_drdy     = 0u;   /* DATA_READY currently asserted       */
static uint8  s_enabled  = 0u;

/* ── Helpers ─────────────────────────────────────────────────────────────────*/

static void SetDRDY(uint8 high)
{
    s_drdy = high;
    SPI_DataReady_Write(high ? 1u : 0u);   /* component name from TopDesign */
}

static void BuildFrameHeader(uint8 *buf, uint16 seq, uint8 global_flags)
{
    buf[0] = SPI_FRAME_MARKER;
    buf[1] = (uint8)(SPI_BATCH_SAMPLES & 0xFFu);
    buf[2] = (uint8)(SPI_BATCH_SAMPLES >> 8u);
    buf[3] = (uint8)(seq & 0xFFu);
    buf[4] = (uint8)(seq >> 8u);
    buf[5] = global_flags;
}

/* ── Transport vtable ────────────────────────────────────────────────────────*/

void TransportSPI_Init(void)
{
    s_fill_idx = 0u;
    s_tx_idx   = 1u;   /* tx ring starts pointing away from fill ring */
    s_fill_pos = 6u;   /* header will be written at commit time */
    s_tx_pos   = 0u;
    s_seq      = 0u;
    s_drdy     = 0u;
    s_enabled  = 1u;
    SetDRDY(0u);
    SPIS_Ext_Start();
    SPIS_Ext_ClearRxBuffer();
    SPIS_Ext_ClearTxBuffer();
}

void TransportSPI_Task(void)
{
    uint8 status;

    if (!s_enabled) { return; }

    status = SPIS_Ext_ReadStatus();

    /* SS deasserted: transaction complete — retire the TX ring, reset */
    if (status & SPIS_Ext_STS_SPI_DONE)
    {
        SetDRDY(0u);
        SPIS_Ext_ClearTxBuffer();
        s_tx_pos = 0u;
        /* fill ring is already filling; tx ring will swap next commit */
    }

    /* Pre-load TX FIFO while space available and DATA_READY is asserted */
    if (s_drdy)
    {
        while ((SPIS_Ext_GetTxBufferSize() < 4u) &&
               (s_tx_pos < SPI_FRAME_BYTES))
        {
            SPIS_Ext_WriteTxData(s_ring[s_tx_idx][s_tx_pos]);
            s_tx_pos++;
        }
    }
}

uint8 TransportSPI_IsReady(void)
{
    return s_enabled && !s_drdy;   /* ready only when not currently transmitting */
}

void TransportSPI_SendSample(int16 raw, int32 post_analog, int32 post_digital,
                              uint8 flags, uint8 gain_byte)
{
    uint8 *dst;
    uint16 pos;

    if (!s_enabled || s_drdy) { return; }   /* drop sample if busy */

    pos = s_fill_pos;
    dst = &s_ring[s_fill_idx][pos];

    /* Pack 10 bytes per sample */
    dst[0] = (uint8)(raw & 0xFF);
    dst[1] = (uint8)((raw >> 8) & 0xFF);
    dst[2] = (uint8)(post_analog & 0xFF);
    dst[3] = (uint8)((post_analog >> 8)  & 0xFF);
    dst[4] = (uint8)((post_analog >> 16) & 0xFF);
    dst[5] = (uint8)(post_digital & 0xFF);
    dst[6] = (uint8)((post_digital >> 8)  & 0xFF);
    dst[7] = (uint8)((post_digital >> 16) & 0xFF);
    dst[8] = gain_byte;
    dst[9] = flags;

    s_fill_pos += 10u;

    if (s_fill_pos >= SPI_FRAME_BYTES)
    {
        /* Ring full: write header, swap, assert DATA_READY */
        uint8 gflags = (uint8)((g_vdac_mode  & 0x01u)       |
                               ((g_filter_sel & 0x01u) << 1u) |
                               ((g_fir_loaded  & 0x01u) << 2u));

        BuildFrameHeader(s_ring[s_fill_idx], s_seq++, gflags);

        s_tx_idx   = s_fill_idx;
        s_fill_idx = (uint8)(1u - s_fill_idx);   /* toggle 0↔1 */
        s_fill_pos = 6u;   /* reserve space for next header */
        s_tx_pos   = 0u;

        SetDRDY(1u);   /* signal ESP32: data ready */
    }
}

/* ── Transport vtable instance ───────────────────────────────────────────────*/

Transport_t spi_transport = {
    .init        = TransportSPI_Init,
    .task        = TransportSPI_Task,
    .is_ready    = TransportSPI_IsReady,
    .send_sample = TransportSPI_SendSample,
    .enabled     = 0u,    /* disabled by default; enable in main.c when wired */
    .name        = "SPI"
};
