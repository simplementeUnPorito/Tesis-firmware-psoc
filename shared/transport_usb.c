/* transport_usb.c
 * USB CDC transport implementation.
 *
 * Packet types sent by this module:
 *   0xAA — data sample (12 bytes)
 *   0xCC — state/config confirmation (10 bytes)
 *   0xDD — debug event (4+ bytes)
 */

#include "project.h"
#include "transport_usb.h"
#include "debug.h"

#define USB_CONFIG_NUM   0u   /* 0-based: first Device Descriptor */

/* ---- Internal helpers ---------------------------------------------------- */

static uint8 CalcCrc(const uint8 *buf, uint8 len)
{
    uint8 crc = 0u;
    uint8 i;
    for (i = 0u; i < len; i++) { crc ^= buf[i]; }
    return crc;
}

static uint8 CalcDebugCrc(uint8 event, uint8 len, const uint8 *payload)
{
    uint8 crc = DBG_HEADER ^ event ^ len;
    uint8 i;
    for (i = 0u; i < len; i++) { crc ^= payload[i]; }
    return crc;
}

/* ---- Public API ---------------------------------------------------------- */

void TransportUSB_SendDebug(uint8 event, const uint8 *payload, uint8 len)
{
    uint8 hdr[3];
    uint8 crc;

    if (USBFS_GetConfiguration() == 0u) { return; }
    if (!USBFS_CDCIsReady())            { return; }

    hdr[0] = DBG_HEADER;
    hdr[1] = event;
    hdr[2] = len;

    USBFS_PutData(hdr, 3u);
    if (len > 0u && payload != (void *)0u)
    {
        USBFS_PutData((uint8 *)payload, len);
    }
    crc = CalcDebugCrc(event, len, payload != (void *)0u ? payload : hdr);
    USBFS_PutData(&crc, 1u);
}

/* State packet: [0xCC][LEN=7][cfg][buf_gain][streaming][debug_en][debug_ch]
 *               [vdac_mode][filter_sel][CRC]   total = 10 bytes */
#define STATE_HDR    0xCCu
#define STATE_PLEN   7u

void TransportUSB_SendState(uint8 adc_cfg, uint8 buf_gain,
                            uint8 streaming, uint8 debug_en,
                            uint8 debug_ch, uint8 vdac_mode,
                            uint8 filter_sel)
{
    uint8 pkt[2u + STATE_PLEN + 1u];   /* hdr + len + payload + crc */

    if (USBFS_GetConfiguration() == 0u) { return; }
    if (!USBFS_CDCIsReady())            { return; }

    pkt[0] = STATE_HDR;
    pkt[1] = STATE_PLEN;
    pkt[2] = adc_cfg;
    pkt[3] = buf_gain;
    pkt[4] = streaming;
    pkt[5] = debug_en;
    pkt[6] = debug_ch;
    pkt[7] = vdac_mode;
    pkt[8] = filter_sel;
    pkt[9] = CalcCrc(pkt, 9u);

    USBFS_PutData(pkt, 10u);
}

uint8 TransportUSB_IsConnected(void)
{
    return (USBFS_GetConfiguration() != 0u) ? 1u : 0u;
}

/* ---- Transport_t callbacks ----------------------------------------------- */

static void USB_Init(void)
{
    USBFS_Start(USB_CONFIG_NUM, USBFS_5V_OPERATION);
}

static void USB_Task(void)
{
    /* RX handled directly in Communication.c to parse commands */
}

static uint8 USB_IsReady(void)
{
    return (USBFS_GetConfiguration() != 0u && USBFS_CDCIsReady()) ? 1u : 0u;
}

static void USB_SendSample(int16 raw_input, int32 post_analog,
                           int32 post_digital, uint8 flags, uint8 gain_byte)
{
    uint8 pkt[12u];

    pkt[0]  = 0xAAu;
    pkt[1]  = flags;
    pkt[2]  = (uint8)((uint16)raw_input & 0xFFu);
    pkt[3]  = (uint8)(((uint16)raw_input >> 8u) & 0xFFu);
    pkt[4]  = (uint8)((uint32)post_analog         & 0xFFu);
    pkt[5]  = (uint8)(((uint32)post_analog  >> 8u) & 0xFFu);
    pkt[6]  = (uint8)(((uint32)post_analog  >> 16u)& 0xFFu);
    pkt[7]  = (uint8)((uint32)post_digital         & 0xFFu);
    pkt[8]  = (uint8)(((uint32)post_digital >> 8u) & 0xFFu);
    pkt[9]  = (uint8)(((uint32)post_digital >> 16u)& 0xFFu);
    pkt[10] = gain_byte;
    pkt[11] = CalcCrc(pkt, 11u);

    USBFS_PutData(pkt, 12u);
}

Transport_t usb_transport = {
    USB_Init,
    USB_Task,
    USB_IsReady,
    USB_SendSample,
    1u,       /* enabled by default */
    "USB_CDC"
};
