/* transport_uart.c
 * UART_PC transport implementation.
 */

#include "project.h"
#include "transport_uart.h"
#include "debug.h"

/* ---- Debug packet builder ------------------------------------------------ */

static uint8 CalcDebugCrc(uint8 event, uint8 len, const uint8 *payload)
{
    uint8 crc = DBG_HEADER ^ event ^ len;
    uint8 i;
    for (i = 0u; i < len; i++) { crc ^= payload[i]; }
    return crc;
}

void TransportUART_SendDebug(uint8 event, const uint8 *payload, uint8 len)
{
    uint8 hdr[3];
    uint8 crc;

    hdr[0] = DBG_HEADER;
    hdr[1] = event;
    hdr[2] = len;

    UART_PC_PutArray(hdr, 3u);
    if (len > 0u && payload != (void *)0u)
    {
        UART_PC_PutArray((uint8 *)payload, len);
    }
    crc = CalcDebugCrc(event, len, payload != (void *)0u ? payload : hdr);
    UART_PC_PutChar(crc);
}

/* ---- State packet sender -------------------------------------------------- */

#define STATE_HDR_UART  0xCCu
#define STATE_PLEN_UART 7u

static uint8 CalcCrc(const uint8 *buf, uint8 len)
{
    uint8 crc = 0u;
    uint8 i;
    for (i = 0u; i < len; i++) { crc ^= buf[i]; }
    return crc;
}

void TransportUART_SendState(uint8 adc_cfg, uint8 buf_gain,
                             uint8 streaming, uint8 debug_en,
                             uint8 debug_ch, uint8 vdac_mode,
                             uint8 filter_sel)
{
    uint8 pkt[2u + STATE_PLEN_UART + 1u];

    pkt[0] = STATE_HDR_UART;
    pkt[1] = STATE_PLEN_UART;
    pkt[2] = adc_cfg;
    pkt[3] = buf_gain;
    pkt[4] = streaming;
    pkt[5] = debug_en;
    pkt[6] = debug_ch;
    pkt[7] = vdac_mode;
    pkt[8] = filter_sel;
    pkt[9] = CalcCrc(pkt, 9u);

    UART_PC_PutArray(pkt, 10u);
}

void TransportUART_Init(void)
{
    UART_PC_Start();
}

uint8 TransportUART_RxAvailable(void)
{
    return UART_PC_GetRxBufferSize();
}

uint8 TransportUART_RxRead(void)
{
    return UART_PC_ReadRxData();
}

/* ---- Transport_t callbacks ----------------------------------------------- */

static void UART_Init(void)
{
    /* UART_PC already started by TransportUART_Init(); avoid double Start() */
}

static void UART_Task(void)
{
    /* Reserved: future RX from ESP32 */
}

static uint8 UART_IsReady(void)
{
    return 1u;   /* UART TX is always ready (buffered internally by PSoC) */
}

static void UART_SendSample(int16 raw_input, int32 post_analog,
                            int32 post_digital, uint8 flags, uint8 gain_byte)
{
    /* Stub — implement compact binary frame here when ESP32 link is needed */
    (void)raw_input; (void)post_analog; (void)post_digital;
    (void)flags;     (void)gain_byte;
}

Transport_t uart_transport = {
    UART_Init,
    UART_Task,
    UART_IsReady,
    UART_SendSample,
    0u,        /* disabled by default — enable in main.c if needed */
    "UART_PC"
};
