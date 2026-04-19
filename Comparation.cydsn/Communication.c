/* Communication.c
 * Multi-transport communication manager — THESIS PERMANENT
 *
 * Responsibilities:
 *   - USB CDC RX: frame parsing and command dispatch
 *   - Transport registry: fan-out SendSample to all enabled transports
 *   - State sync: send 0xCC state packet after every command so Python
 *                 only updates its UI once PSoC confirms the change
 */

#include "project.h"
#include "Communication.h"
#include "transport_usb.h"
#include "transport_uart.h"
#include "debug.h"
#include "buffers.h"
#include "tb_buffers.h"
#include "AnalogToDigital.h"
#include "TestBench.h"

/* ---- Transport registry -------------------------------------------------- */

static Transport_t *s_transports[COMM_MAX_TRANSPORTS];
static uint8        s_transport_count = 0u;

void Communication_RegisterTransport(Transport_t *t)
{
    if (s_transport_count < COMM_MAX_TRANSPORTS)
    {
        s_transports[s_transport_count++] = t;
    }
}

/* ---- Streaming / debug state --------------------------------------------- */

static uint8 s_streaming = 0u;

/* ---- Gain byte encoding -------------------------------------------------- */

/* Encodes cfg (1-4) and buf_gain (1/2/4/8) into one byte:
 *   low nibble  = cfg
 *   high nibble = log2(buf_gain)  (0=1x, 1=2x, 2=4x, 3=8x) */
static uint8 EncodeGainByte(uint8 cfg, uint8 buf_gain)
{
    uint8 gc = 0u;
    if      (buf_gain >= 8u) { gc = 3u; }
    else if (buf_gain >= 4u) { gc = 2u; }
    else if (buf_gain >= 2u) { gc = 1u; }
    return (uint8)((gc << 4u) | (cfg & 0x0Fu));
}

/* ---- RX command parser state machine ------------------------------------- */

typedef enum {
    RX_WAIT_MARKER = 0,
    RX_WAIT_CMD,
    RX_WAIT_LEN,
    RX_WAIT_LEN2,     /* second length byte for VDAC_LOAD only */
    RX_WAIT_PAYLOAD
} RxState_t;

static RxState_t s_rx_state    = RX_WAIT_MARKER;
static uint8     s_rx_cmd      = 0u;
static uint8     s_rx_len_hi   = 0u;   /* VDAC_LOAD: high byte of 16-bit length */
static uint8     s_rx_buf[TB_VDAC_MAX_LEN];
static uint16    s_rx_count    = 0u;
static uint16    s_rx_expected = 0u;

static void ProcessCommand(uint8 cmd, const uint8 *payload, uint16 len)
{
    uint8 dbg_pld[2];

    dbg_pld[0] = cmd;
    Debug_Send(DBG_EVT_CMD_RX, dbg_pld, 1u);

    switch (cmd)
    {
        case COMM_CMD_START:
            s_streaming = 1u;
            Debug_Send(DBG_EVT_STREAM_ON, (void *)0u, 0u);
            break;

        case COMM_CMD_STOP:
            s_streaming = 0u;
            Debug_Send(DBG_EVT_STREAM_OFF, (void *)0u, 0u);
            break;

        case COMM_CMD_SET_MUX_IN:
            if (len >= 1u) { TestBench_SetInput(payload[0]); }
            break;

        case COMM_CMD_SET_MUX_OUT:
            if (len >= 1u) { TestBench_SetOutput(payload[0]); }
            break;

        case COMM_CMD_SET_ADC:
            /* payload: [cfg:1-4][buf_gain:1/2/4/8] */
            if (len >= 2u)
            {
                uint8 cfg      = payload[0];
                uint8 buf_gain = payload[1];
                AnalogToDigital_SetADC(cfg, buf_gain);
                dbg_pld[0] = g_adc_cfg;
                dbg_pld[1] = g_adc_buf_gain;
                Debug_Send(DBG_EVT_ADC_CHG, dbg_pld, 2u);
            }
            break;

        case COMM_CMD_VDAC_LOAD:
            /* payload is raw data; len == sequence length */
            if (len > 0u && len <= TB_VDAC_MAX_LEN)
            {
                TestBench_SetVdacSeq(payload, len);
                dbg_pld[0] = (uint8)(len >> 8u);
                dbg_pld[1] = (uint8)(len & 0xFFu);
                Debug_Send(DBG_EVT_VDAC_LOADED, dbg_pld, 2u);
            }
            break;

        case COMM_CMD_SET_DEBUG:
            if (len >= 1u) { Debug_SetEnabled(payload[0]); }
            break;

        case COMM_CMD_SET_DBG_CH:
            if (len >= 1u) { Debug_SetChannel(payload[0]); }
            break;

        case COMM_CMD_GET_STATE:
            /* State packet sent unconditionally after switch */
            break;

        default:
            break;
    }

    /* Always confirm current state so Python syncs its UI */
    Communication_SendState();
}

static void ParseRxByte(uint8 b)
{
    switch (s_rx_state)
    {
        case RX_WAIT_MARKER:
            if (b == COMM_CMD_MARKER) { s_rx_state = RX_WAIT_CMD; }
            break;

        case RX_WAIT_CMD:
            s_rx_cmd   = b;
            s_rx_state = RX_WAIT_LEN;
            break;

        case RX_WAIT_LEN:
            if (s_rx_cmd == COMM_CMD_VDAC_LOAD)
            {
                /* Special: next 2 bytes are the 16-bit sequence length */
                s_rx_len_hi = b;
                s_rx_state  = RX_WAIT_LEN2;
            }
            else if (b == 0u)
            {
                ProcessCommand(s_rx_cmd, s_rx_buf, 0u);
                s_rx_state = RX_WAIT_MARKER;
            }
            else
            {
                s_rx_expected = (uint16)b;
                s_rx_count    = 0u;
                s_rx_state    = RX_WAIT_PAYLOAD;
            }
            break;

        case RX_WAIT_LEN2:
            s_rx_expected = ((uint16)s_rx_len_hi << 8u) | (uint16)b;
            s_rx_count    = 0u;
            if (s_rx_expected == 0u)
            {
                s_rx_state = RX_WAIT_MARKER;   /* empty VDAC_LOAD: ignore */
            }
            else
            {
                s_rx_state = RX_WAIT_PAYLOAD;
            }
            break;

        case RX_WAIT_PAYLOAD:
            if (s_rx_count < sizeof(s_rx_buf))
            {
                s_rx_buf[s_rx_count] = b;
            }
            s_rx_count++;
            if (s_rx_count >= s_rx_expected)
            {
                ProcessCommand(s_rx_cmd, s_rx_buf, s_rx_count);
                s_rx_state = RX_WAIT_MARKER;
            }
            break;
    }
}

/* ---- Public API ---------------------------------------------------------- */

void Communication_Init(void)
{
    uint8 i;
    s_streaming = 0u;
    s_rx_state  = RX_WAIT_MARKER;

    for (i = 0u; i < s_transport_count; i++)
    {
        if (s_transports[i]->init != (void *)0u)
        {
            s_transports[i]->init();
        }
    }
}

void Communication_Task(void)
{
    uint8 i;

    /* Run all transport tasks */
    for (i = 0u; i < s_transport_count; i++)
    {
        if (s_transports[i]->task != (void *)0u)
        {
            s_transports[i]->task();
        }
    }

    /* USB CDC RX — command parsing */
    if (USBFS_GetConfiguration() == 0u) { return; }

    if (USBFS_DataIsReady())
    {
        uint8  rxBuf[64u];
        uint16 rxLen = USBFS_GetAll(rxBuf);
        uint16 i2;
        for (i2 = 0u; i2 < rxLen; i2++) { ParseRxByte(rxBuf[i2]); }
    }
}

void Communication_SendSample(int16 raw_input, int32 post_analog,
                               int32 post_digital, uint8 flags)
{
    uint8 i;
    uint8 gain_byte;

    if (!s_streaming) { return; }

    gain_byte = EncodeGainByte(g_adc_cfg, g_adc_buf_gain);

    for (i = 0u; i < s_transport_count; i++)
    {
        Transport_t *t = s_transports[i];
        if (t->enabled && t->is_ready && t->is_ready() && t->send_sample)
        {
            t->send_sample(raw_input, post_analog, post_digital, flags, gain_byte);
        }
    }
}

void Communication_SendState(void)
{
    TransportUSB_SendState(
        g_adc_cfg,
        g_adc_buf_gain,
        s_streaming,
        Debug_IsEnabled(),
        Debug_GetChannel(),
        g_vdac_mode,
        g_filter_sel);
}

uint8 Communication_IsConnected(void)
{
    return TransportUSB_IsConnected();
}

uint8 Communication_IsStreaming(void)
{
    return s_streaming;
}
