#include "psoc_hw.h"
#include "sd_spi.h"

static uint8 g_psoc_hw_pga_code    = PSOC_PGA_DEFAULT_CODE;
static uint8 g_psoc_hw_pgaout_code = PSOC_PGAOUT_DEFAULT_CODE;

/* Parámetros de la red IDAC->tensión de la placa. Globales (no macros) para
 * poder ajustarlas si cambia la resistencia sin recompilar las tablas. */
/* Espejo del Control Register: bit i = etapa i en modo sumidero. Se lleva en
 * RAM porque el registro es de solo escritura desde el punto de vista del
 * firmware y hay que poder tocar un bit sin pisar los otros tres. */
static uint8 g_psoc_idac_polarity = 0u;

int16 psoc_hw_idac_clamp_signed(int16 code)
{
    if (code >  PSOC_IDAC_SIGNED_MAX) { return  (int16)PSOC_IDAC_SIGNED_MAX; }
    if (code < -PSOC_IDAC_SIGNED_MAX) { return (int16)(-PSOC_IDAC_SIGNED_MAX); }
    return code;
}

uint8 psoc_hw_idac_apply_polarity(uint8 stage, int16 code)
{
    int16 magnitude = psoc_hw_idac_clamp_signed(code);
    uint8 bit = (uint8)(1u << (stage & 0x03u));

    if (magnitude < 0) {
#if PSOC_IDAC_POLARITY_NEGATIVE_BIT
        g_psoc_idac_polarity |= bit;
#else
        g_psoc_idac_polarity = (uint8)(g_psoc_idac_polarity & (uint8)~bit);
#endif
        magnitude = (int16)(-magnitude);
    } else {
#if PSOC_IDAC_POLARITY_NEGATIVE_BIT
        g_psoc_idac_polarity = (uint8)(g_psoc_idac_polarity & (uint8)~bit);
#else
        g_psoc_idac_polarity |= bit;
#endif
    }
#if defined(CY_CONTROL_REG_polarity_reg_H)
    /* La polaridad se escribe ANTES que la magnitud: si se hiciera al reves,
     * un cambio de signo pasaria un instante por la magnitud nueva con el
     * signo viejo, que es el doble del escalon pedido. */
    polarity_reg_Write(g_psoc_idac_polarity);
#endif
    return (uint8)magnitude;
}

void psoc_hw_idac_polarity_reset(void)
{
#if PSOC_IDAC_POLARITY_NEGATIVE_BIT
    g_psoc_idac_polarity = 0u;
#else
    g_psoc_idac_polarity = 0x0Fu;
#endif
#if defined(CY_CONTROL_REG_polarity_reg_H)
    polarity_reg_Write(g_psoc_idac_polarity);
#endif
}

uint8 psoc_hw_idac_polarity_mask(void)
{
    return g_psoc_idac_polarity;
}

uint32 g_psoc_idac_rset_ohm      = PSOC_IDAC_RSET_OHM_DEFAULT;
uint32 g_psoc_idac_vref_uv       = PSOC_IDAC_VREF_UV_DEFAULT;
uint32 g_psoc_idac_fullscale_na  = PSOC_IDAC_FULLSCALE_NA_DEFAULT;

uint32 psoc_idac_code_to_na(uint8 code)
{
    /* code * I_fs / 255 con redondeo al nA. Con I_fs=31875 nA el producto
     * máximo es 8.1e6, sin riesgo de overflow en 32 bits. */
    return (((uint32)code * g_psoc_idac_fullscale_na) + (PSOC_IDAC_CODE_MAX / 2u)) /
           PSOC_IDAC_CODE_MAX;
}

uint32 psoc_idac_lsb_uv(void)
{
    /* (I_fs/255) [nA] * R [ohm] / 1000 -> µV. Con 125 nA y 30 kΩ da 3750 µV. */
    return ((g_psoc_idac_fullscale_na / PSOC_IDAC_CODE_MAX) * g_psoc_idac_rset_ohm) / 1000u;
}

uint32 psoc_idac_code_to_uv(uint8 code)
{
    /* nA * ohm = nV; /1000 -> µV. El máximo (31875 nA * 30 kΩ) son 956 mV,
     * que en nV son 9.6e8: entra justo en uint32. */
    uint32 na = psoc_idac_code_to_na(code);
    return g_psoc_idac_vref_uv + ((na * g_psoc_idac_rset_ohm) / 1000u);
}

uint8 psoc_idac_uv_to_code(uint32 uv)
{
    uint32 lsb = psoc_idac_lsb_uv();
    uint32 code;

    if (lsb == 0u || uv <= g_psoc_idac_vref_uv) {
        return 0u;
    }
    code = ((uv - g_psoc_idac_vref_uv) + (lsb / 2u)) / lsb;
    return (code > PSOC_IDAC_CODE_MAX) ? (uint8)PSOC_IDAC_CODE_MAX : (uint8)code;
}

static uint16 psoc_hw_pga_code_to_gain_x1000(uint8 code)
{
    switch (code) {
        case 0u: return 1000u;
        case 1u: return 2000u;
        case 2u: return 4000u;
        case 3u: return 8000u;
        case 4u: return 16000u;
        case 5u: return 24000u;
        case 6u: return 32000u;
        case 7u: return 48000u;
        case 8u: return 50000u;
        default: return 1000u;
    }
}

void psoc_hw_set_pga(uint8 code)
{
    g_psoc_hw_pga_code = code;
#if PSOC_HW_CLASS == PSOC_HW_GEO
    PGAgain_SetGain(code);
#else
    PGA_SetGain(code);
#endif
}

uint8 psoc_hw_get_pga_code(void)
{
    return g_psoc_hw_pga_code;
}

uint16 psoc_hw_pga_gain_x1000(void)
{
    return psoc_hw_pga_code_to_gain_x1000(g_psoc_hw_pga_code);
}

void psoc_hw_set_pgaout(uint8 code)
{
    g_psoc_hw_pgaout_code = code;
#if PSOC_HW_CLASS == PSOC_HW_GEO
    PGAout_SetGain(code);
#endif
}

uint8 psoc_hw_get_pgaout_code(void)
{
    return g_psoc_hw_pgaout_code;
}

uint16 psoc_hw_pgaout_gain_x1000(void)
{
    return psoc_hw_pga_code_to_gain_x1000(g_psoc_hw_pgaout_code);
}

void psoc_hw_set_pgavdac(uint8 code)
{
#if PSOC_HW_CLASS == PSOC_HW_GEO
    #if defined(PGAvdac_DEFAULT_GAIN)
    PGAvdac_SetGain(code);
    #else
    (void)code;
    #endif
#else
    #if defined(PGA_ref_DEFAULT_GAIN)
    PGA_ref_SetGain(code);
    #else
    (void)code;
    #endif
#endif
}

void psoc_hw_start_analog(uint8 pga_code, uint8 pgavdac_code, uint8 pgaout_code)
{
#if PSOC_HW_CLASS == PSOC_HW_GEO
    OPAref_Start();
    PGAout_Start();
    psoc_hw_set_pgaout(pgaout_code);
    PGAp_Start();
    PGAn_Start();
    PGAp_SetGain(PGAp_GAIN_02);
    PGAn_SetGain(PGAn_GAIN_02);
    LPF_1_Start();
    LPF_2_Start();
    PGAgain_Start();
    psoc_hw_set_pga(pga_code);
    OPAbp_Start();
    OPAsum_Start();
    #if defined(PGAvdac_DEFAULT_GAIN)
    PGAvdac_Start();
    psoc_hw_set_pgavdac(pgavdac_code);
    #else
    (void)pgavdac_code;
    #endif
    OPAlp_Start();
#else
    Opa_ref_1V_Start();
    LPF_ADC_Start();
    Opa_ref_PGA_Start();
    PGA_Start();
    psoc_hw_set_pga(pga_code);
    Opa_LP_Start();
    (void)pgavdac_code;
    (void)pgaout_code;
#endif
}

/* ==========================================================================
 * Enlace de salida PSoC -> ESP32 por I2C (I2C_1 en modo maestro)
 * --------------------------------------------------------------------------
 * Reemplaza al TX de la UART, que en la placa nueva ya no existe. Se escribe
 * de a un frame completo hacia PSOC_LINK_I2C_ADDR; el ESP lo recibe en su
 * callback onReceive() y lo mete en el mismo parser de frames de siempre.
 *
 * MasterWriteBuf() es asíncrono y se queda con el puntero, así que el frame
 * se copia a un buffer estático antes de arrancar la transferencia. La espera
 * está acotada por un guard: si el bus queda trabado el firmware pierde el
 * frame pero no se cuelga.
 * ========================================================================== */

/* El TopDesign nombra la instancia `I2C`; si en algún rediseño vuelve a salir
 * como `I2C_1` el enlace sigue compilando sin tocar este archivo. */
#if defined(CY_I2C_I2C_H)
    #define LINK_I2C(sym)            I2C_##sym
#elif defined(CY_I2C_I2C_1_H)
    #define LINK_I2C(sym)            I2C_1_##sym
#else
    #error "No hay componente I2C en el TopDesign: el enlace PSoC->ESP lo necesita."
#endif

static uint8 g_link_started = 0u;
static uint8 g_link_last_ok = 1u;
static uint8 g_link_buf[PSOC_LINK_MAX_FRAME];

void psoc_link_start(void)
{
    if (!g_link_started) {
        LINK_I2C(Start)();
        (void)LINK_I2C(MasterClearStatus)();
        g_link_started = 1u;
    }
}

void psoc_link_wait_idle(void)
{
    /* A 400 kHz un frame de 95 bytes tarda ~2.4 ms. El guard cubre órdenes de
     * magnitud más que eso sin bloquear el main loop de forma indefinida. */
    uint32 guard = 400000u;
    if (!g_link_started) {
        return;
    }
    while (guard-- != 0u) {
        if ((LINK_I2C(MasterStatus)() & LINK_I2C(MSTAT_XFER_INP)) == 0u) {
            return;
        }
    }
    g_link_last_ok = 0u;
}

void psoc_link_put_array(const uint8 *buf, uint16 len)
{
    uint16 i;
    uint8  status;

    if ((buf == 0) || (len == 0u)) {
        return;
    }
    psoc_link_start();
    if (len > (uint16)sizeof(g_link_buf)) {
        len = (uint16)sizeof(g_link_buf);
    }

    psoc_link_wait_idle();
    for (i = 0u; i < len; i++) {
        g_link_buf[i] = buf[i];
    }
    (void)LINK_I2C(MasterClearStatus)();

    if (LINK_I2C(MasterWriteBuf)(PSOC_LINK_I2C_ADDR, g_link_buf, (uint8)len,
                                 LINK_I2C(MODE_COMPLETE_XFER)) != LINK_I2C(MSTR_NO_ERROR)) {
        g_link_last_ok = 0u;
        return;
    }
    psoc_link_wait_idle();
    status = LINK_I2C(MasterStatus)();
    g_link_last_ok = ((status & LINK_I2C(MSTAT_ERR_XFER)) == 0u) ? 1u : 0u;
}

uint8 psoc_link_last_ok(void)
{
    return g_link_last_ok;
}

/* ==========================================================================
 * Driver SD modo SPI (declaraciones en sd_spi.h — implementado acá para no
 * registrar un .c nuevo en el .cyprj). Verificado contra el netlist elaborado:
 * BSPIM ModeCPHA=0, ModePOL=0, ShiftDir=MSB-first, 8 bits — SPI modo 0, el que
 * exige la SD. SPI_IntClock nominal 2 MHz (bitrate 1 Mbps); para el init se
 * baja por divisor a ~375 kHz (187.5 kbps < 400 kHz de la spec) y se restaura.
 * ========================================================================== */

#define SD_CMD_RETRY        8u
#define SD_R1_IDLE          0x01u
#define SD_TOKEN_START      0xFEu
/* Divisor de SPI_IntClock para el init lento: 24 MHz / 64 = 375 kHz de clock
 * interno -> 187.5 kbps en el bus. */
#define SD_INIT_CLK_DIVIDER 64u

static uint8  g_sd_type          = SD_TYPE_NONE;
static uint8  g_sd_selftest_ok   = 0u;
static uint8  g_sd_spi_started   = 0u;
static uint16 g_sd_fast_divider  = 0u;
static uint32 g_sd_sector_count  = 0u;
static uint8  g_sd_scratch[SD_BLOCK_BYTES];
static uint8  g_sd_diag_stage    = 0u;
static uint8  g_sd_diag_last_r1  = 0xFFu;
static uint8  g_sd_diag_pins     = 0u;
static uint8  g_sd_diag_timeout  = 0u;

/* CS (P2.3) quedó ruteado al `ss` de hardware del BSPIM, que deassertea entre
 * bytes. Se lo desengancha del DSI con el bypass del puerto (patrón calcado de
 * tx1_gpio_detach_dsi) para manejarlo por software durante toda la transacción. */
/* En la placa nueva los cuatro pines de SPIp ya no caen en el mismo puerto,
 * así que el fitter dejó de emitir los macros agregados (SPIp__DR, SPIp__BYP,
 * SPIp__PRTDSI__*) y hay que ir por los del pin 0, que es CS. Se mantiene el
 * camino viejo por si un rediseño vuelve a juntarlos en un puerto. */
#if defined(SPIp__DR)
    #define SD_CS_DR            SPIp__DR
    #define SD_CS_BYP           SPIp__BYP
    #define SD_CS_OUT_SEL0      SPIp__PRTDSI__OUT_SEL0
    #define SD_CS_OUT_SEL1      SPIp__PRTDSI__OUT_SEL1
    #define SD_CS_OE_SEL0       SPIp__PRTDSI__OE_SEL0
    #define SD_CS_OE_SEL1       SPIp__PRTDSI__OE_SEL1
#else
    #define SD_CS_DR            SPIp__0__DR
    #define SD_CS_BYP           SPIp__0__BYP
    #define SD_CS_OUT_SEL0      SPIp__0__PRTDSI__OUT_SEL0
    #define SD_CS_OUT_SEL1      SPIp__0__PRTDSI__OUT_SEL1
    #define SD_CS_OE_SEL0       SPIp__0__PRTDSI__OE_SEL0
    #define SD_CS_OE_SEL1       SPIp__0__PRTDSI__OE_SEL1
#endif
#define SD_CS_MASK              SPIp__CS__MASK

static void sd_cs_detach_dsi(void)
{
    reg8 *outSel0 = (reg8 *)SD_CS_OUT_SEL0;
    reg8 *outSel1 = (reg8 *)SD_CS_OUT_SEL1;
    reg8 *oeSel0  = (reg8 *)SD_CS_OE_SEL0;
    reg8 *oeSel1  = (reg8 *)SD_CS_OE_SEL1;
    reg8 *byp     = (reg8 *)SD_CS_BYP;
    reg8 *dr      = (reg8 *)SD_CS_DR;
    uint8 saved   = CyEnterCriticalSection();

    /* DR alto ANTES del bypass para que el pin no glitchee a 0 al cambiar de
     * fuente (la SD interpreta CS bajo como selección). */
    *dr      |= SD_CS_MASK;
    *outSel0 &= (uint8)~SD_CS_MASK;
    *outSel1 &= (uint8)~SD_CS_MASK;
    *oeSel0  &= (uint8)~SD_CS_MASK;
    *oeSel1  &= (uint8)~SD_CS_MASK;
    *byp     |= SD_CS_MASK;
    CyExitCriticalSection(saved);
    CyPins_SetPinDriveMode(SPIp_CS, CY_PINS_DM_STRONG);
}

static void sd_cs_write(uint8 value)
{
    reg8 *dr = (reg8 *)SD_CS_DR;
    uint8 saved = CyEnterCriticalSection();
    if (value) {
        *dr |= SD_CS_MASK;
    } else {
        *dr &= (uint8)~SD_CS_MASK;
    }
    CyExitCriticalSection(saved);
}

/* Muestrea los PADS, no los data registers. Durante una transferencia el lazo
 * de espera corre muchas veces por bit y alcanza a observar ambos niveles de
 * SCK/MOSI; MISO revela enseguida una linea clavada o flotante. */
static void sd_diag_sample_pins(void)
{
    uint8 ps;
    ps = CY_GET_REG8((reg8 *)SPIp__MISO__PS);
    g_sd_diag_pins |= ((ps & SPIp__MISO__MASK) != 0u) ?
                      SD_DIAG_MISO_HIGH : SD_DIAG_MISO_LOW;
    ps = CY_GET_REG8((reg8 *)SPIp__SCK__PS);
    g_sd_diag_pins |= ((ps & SPIp__SCK__MASK) != 0u) ?
                      SD_DIAG_SCK_HIGH : SD_DIAG_SCK_LOW;
    ps = CY_GET_REG8((reg8 *)SPIp__MOSI__PS);
    g_sd_diag_pins |= ((ps & SPIp__MOSI__MASK) != 0u) ?
                      SD_DIAG_MOSI_HIGH : SD_DIAG_MOSI_LOW;
    ps = CY_GET_REG8((reg8 *)SPIp__CS__PS);
    g_sd_diag_pins |= ((ps & SPIp__CS__MASK) != 0u) ?
                      SD_DIAG_CS_HIGH : SD_DIAG_CS_LOW;
}

/* Un byte full-duplex. Lazo acotado: a 187.5 kbps un byte tarda ~43 us; el
 * guard de 60000 vueltas es órdenes de magnitud más que eso y evita colgar el
 * main loop si el clock/SPI quedara en un estado imposible. */
static uint8 sd_xfer(uint8 v)
{
    uint16 guard = 60000u;
    sd_diag_sample_pins();
    SPI_WriteTxData(v);
    while (0u == (SPI_ReadRxStatus() & SPI_STS_RX_FIFO_NOT_EMPTY)) {
        sd_diag_sample_pins();
        if (--guard == 0u) {
            g_sd_diag_timeout = 1u;
            return 0xFFu;
        }
    }
    sd_diag_sample_pins();
    return SPI_ReadRxData();
}

static void sd_clock_slow(void)
{
    g_sd_fast_divider = (uint16)(SPI_IntClock_GetDividerRegister() + 1u);
    SPI_IntClock_SetDividerValue(SD_INIT_CLK_DIVIDER);
}

static void sd_clock_fast(void)
{
    if (g_sd_fast_divider != 0u) {
        SPI_IntClock_SetDividerValue(g_sd_fast_divider);
    }
}

/* Espera a que la tarjeta suelte la línea de busy (responde 0xFF). */
static uint8 sd_wait_ready(uint32 max_bytes)
{
    while (max_bytes-- != 0u) {
        if (sd_xfer(0xFFu) == 0xFFu) {
            return 1u;
        }
    }
    return 0u;
}

static uint8 sd_cmd(uint8 cmd, uint32 arg, uint8 crc)
{
    uint8 r1 = 0xFFu;
    uint8 i;

    (void)sd_xfer(0xFFu);
    (void)sd_xfer((uint8)(0x40u | cmd));
    (void)sd_xfer((uint8)(arg >> 24u));
    (void)sd_xfer((uint8)(arg >> 16u));
    (void)sd_xfer((uint8)(arg >> 8u));
    (void)sd_xfer((uint8)arg);
    (void)sd_xfer(crc);
    /* R1 llega dentro de los próximos 8 bytes (NCR); margen extra por cards
     * lentas en init. */
    for (i = 0u; i < 32u; i++) {
        r1 = sd_xfer(0xFFu);
        if ((r1 & 0x80u) == 0u) {
            break;
        }
    }
    g_sd_diag_last_r1 = r1;
    return r1;
}

static uint8 sd_acmd(uint8 cmd, uint32 arg)
{
    uint8 r1 = sd_cmd(55u, 0u, 0xFFu);
    if (r1 > SD_R1_IDLE) {
        return r1;
    }
    return sd_cmd(cmd, arg, 0xFFu);
}

/* Lee uno de los registros de 16 bytes (CMD9=CSD). Debe llamarse con CS bajo. */
static uint8 sd_read_register(uint8 cmd, uint8 *dst)
{
    uint16 guard;
    uint8 token = 0xFFu;
    uint8 i;

    if (sd_cmd(cmd, 0u, 0xFFu) != 0x00u) {
        return 0u;
    }
    for (guard = 0u; guard < 15000u; guard++) {
        token = sd_xfer(0xFFu);
        if (token != 0xFFu) {
            break;
        }
    }
    if (token != SD_TOKEN_START) {
        return 0u;
    }
    for (i = 0u; i < 16u; i++) {
        dst[i] = sd_xfer(0xFFu);
    }
    (void)sd_xfer(0xFFu);
    (void)sd_xfer(0xFFu);
    return 1u;
}

static uint32 sd_sector_count_from_csd(const uint8 *csd)
{
    if ((csd[0] & 0xC0u) == 0x40u) {
        uint32 c_size = ((uint32)(csd[7] & 0x3Fu) << 16u) |
                        ((uint32)csd[8] << 8u) | (uint32)csd[9];
        return (c_size + 1u) * 1024u;
    } else {
        uint32 c_size = ((uint32)(csd[6] & 0x03u) << 10u) |
                        ((uint32)csd[7] << 2u) |
                        ((uint32)(csd[8] >> 6u) & 0x03u);
        uint8 c_mult = (uint8)(((csd[9] & 0x03u) << 1u) |
                               ((csd[10] >> 7u) & 0x01u));
        uint8 read_bl_len = (uint8)(csd[5] & 0x0Fu);
        uint64 bytes = ((uint64)c_size + 1u) << ((uint8)c_mult + 2u + read_bl_len);
        return (uint32)(bytes / (uint64)SD_BLOCK_BYTES);
    }
}

/* Dirección efectiva del bloque según el tipo (SDHC direcciona por bloque,
 * SDSC por byte). */
static uint32 sd_block_addr(uint32 lba)
{
    return (g_sd_type == SD_TYPE_SDHC) ? lba : (lba << 9u);
}

uint8 sd_spi_init(void)
{
    uint8 r1;
    uint8 i;
    uint8 ocr[4];
    uint16 tries;
    uint8 ok = 0u;

    g_sd_type = SD_TYPE_NONE;
    g_sd_selftest_ok = 0u;
    g_sd_sector_count = 0u;
    g_sd_diag_stage = 1u;
    g_sd_diag_last_r1 = 0xFFu;
    g_sd_diag_pins = 0u;
    g_sd_diag_timeout = 0u;

    if (!g_sd_spi_started) {
        sd_cs_detach_dsi();
        SPI_Start();
        g_sd_spi_started = 1u;
    }

    sd_clock_slow();
    SPI_ClearFIFO();
    sd_cs_write(1u);
    /* >=74 clocks con CS alto para entrar a modo SPI nativo. */
    for (i = 0u; i < 12u; i++) {
        (void)sd_xfer(0xFFu);
    }

    sd_cs_write(0u);
    g_sd_diag_stage = 2u;
    r1 = 0xFFu;
    for (i = 0u; i < SD_CMD_RETRY; i++) {
        r1 = sd_cmd(0u, 0u, 0x95u);
        if (r1 == SD_R1_IDLE) {
            break;
        }
    }
    if (r1 != SD_R1_IDLE) {
        goto done;   /* sin tarjeta (o no responde): present=0 */
    }

    g_sd_diag_stage = 3u;
    r1 = sd_cmd(8u, 0x000001AAu, 0x87u);
    if (r1 == SD_R1_IDLE) {
        /* SD v2: leer R7 y verificar echo del patrón. */
        for (i = 0u; i < 4u; i++) {
            ocr[i] = sd_xfer(0xFFu);
        }
        if ((ocr[2] != 0x01u) || (ocr[3] != 0xAAu)) {
            goto done;
        }
        g_sd_diag_stage = 4u;
        for (tries = 0u; tries < 1000u; tries++) {
            r1 = sd_acmd(41u, 0x40000000u);   /* HCS=1 */
            if (r1 == 0x00u) {
                break;
            }
            CyDelay(1u);
        }
        if (r1 != 0x00u) {
            goto done;
        }
        g_sd_diag_stage = 5u;
        r1 = sd_cmd(58u, 0u, 0xFFu);
        if (r1 != 0x00u) {
            goto done;
        }
        for (i = 0u; i < 4u; i++) {
            ocr[i] = sd_xfer(0xFFu);
        }
        g_sd_type = ((ocr[0] & 0x40u) != 0u) ? SD_TYPE_SDHC : SD_TYPE_SD2;
    } else {
        /* SD v1 (o MMC): ACMD41 sin HCS, fallback CMD1. */
        g_sd_diag_stage = 4u;
        for (tries = 0u; tries < 1000u; tries++) {
            r1 = sd_acmd(41u, 0u);
            if (r1 == 0x00u) {
                break;
            }
            if (r1 > SD_R1_IDLE) {
                r1 = sd_cmd(1u, 0u, 0xFFu);
                if (r1 == 0x00u) {
                    break;
                }
            }
            CyDelay(1u);
        }
        if (r1 != 0x00u) {
            goto done;
        }
        g_sd_type = SD_TYPE_SD1;
    }

    if (g_sd_type != SD_TYPE_SDHC) {
        g_sd_diag_stage = 6u;
        if (sd_cmd(16u, (uint32)SD_BLOCK_BYTES, 0xFFu) != 0x00u) {
            g_sd_type = SD_TYPE_NONE;
            goto done;
        }
    }
    g_sd_diag_stage = 7u;
    if (!sd_read_register(9u, g_sd_scratch)) {
        g_sd_type = SD_TYPE_NONE;
        goto done;
    }
    g_sd_diag_stage = 8u;
    g_sd_sector_count = sd_sector_count_from_csd(g_sd_scratch);
    if (g_sd_sector_count == 0u) {
        g_sd_type = SD_TYPE_NONE;
        goto done;
    }
    g_sd_diag_stage = 9u;
    ok = 1u;

done:
    sd_cs_write(1u);
    (void)sd_xfer(0xFFu);
    sd_clock_fast();
    return ok;
}

uint8 sd_spi_present(void)
{
    return (g_sd_type != SD_TYPE_NONE) ? 1u : 0u;
}

uint8 sd_spi_card_type(void)
{
    return g_sd_type;
}

uint32 sd_spi_sector_count(void)
{
    return g_sd_sector_count;
}

uint8 sd_spi_status_byte(void)
{
    uint8 st = 0u;
    if (g_sd_type != SD_TYPE_NONE) {
        st |= 0x01u;
        st |= (uint8)((g_sd_type & 0x03u) << 1u);
    }
    if (g_sd_selftest_ok) {
        st |= 0x08u;
    }
    return st;
}

uint8 sd_spi_diag_stage(void)
{
    return (uint8)(g_sd_diag_stage | (g_sd_diag_timeout ? 0x80u : 0u));
}

uint8 sd_spi_diag_last_r1(void)
{
    return g_sd_diag_last_r1;
}

uint8 sd_spi_diag_pin_flags(void)
{
    return g_sd_diag_pins;
}

uint8 sd_spi_read_block(uint32 lba, uint8 *dst)
{
    uint16 i;
    uint8 token = 0xFFu;
    uint32 guard;
    uint8 ok = 0u;

    if (g_sd_type == SD_TYPE_NONE) {
        return 0u;
    }
    sd_cs_write(0u);
    if (!sd_wait_ready(40000u)) {
        goto out;
    }
    if (sd_cmd(17u, sd_block_addr(lba), 0xFFu) != 0x00u) {
        goto out;
    }
    /* Token de datos: hasta ~100 ms a 1 Mbps. */
    for (guard = 0u; guard < 15000u; guard++) {
        token = sd_xfer(0xFFu);
        if (token != 0xFFu) {
            break;
        }
    }
    if (token != SD_TOKEN_START) {
        goto out;
    }
    for (i = 0u; i < SD_BLOCK_BYTES; i++) {
        dst[i] = sd_xfer(0xFFu);
    }
    (void)sd_xfer(0xFFu);   /* CRC16 descartado (CRC off en modo SPI) */
    (void)sd_xfer(0xFFu);
    ok = 1u;
out:
    sd_cs_write(1u);
    (void)sd_xfer(0xFFu);
    return ok;
}

uint8 sd_spi_write_block(uint32 lba, const uint8 *src)
{
    uint16 i;
    uint8 resp;
    uint8 ok = 0u;

    if (g_sd_type == SD_TYPE_NONE) {
        return 0u;
    }
    sd_cs_write(0u);
    if (!sd_wait_ready(40000u)) {
        goto out;
    }
    if (sd_cmd(24u, sd_block_addr(lba), 0xFFu) != 0x00u) {
        goto out;
    }
    (void)sd_xfer(0xFFu);
    (void)sd_xfer(SD_TOKEN_START);
    for (i = 0u; i < SD_BLOCK_BYTES; i++) {
        (void)sd_xfer(src[i]);
    }
    (void)sd_xfer(0xFFu);   /* CRC16 dummy */
    (void)sd_xfer(0xFFu);
    resp = sd_xfer(0xFFu);
    if ((resp & 0x1Fu) != 0x05u) {
        goto out;
    }
    /* Busy de programación: típicamente <10 ms, worst-case cientos de ms. */
    if (!sd_wait_ready(80000u)) {
        goto out;
    }
    ok = 1u;
out:
    sd_cs_write(1u);
    (void)sd_xfer(0xFFu);
    return ok;
}

uint8 sd_spi_self_test(void)
{
    uint16 i;

    g_sd_selftest_ok = 0u;
    if (g_sd_type == SD_TYPE_NONE || g_sd_sector_count == 0u) {
        return 0u;
    }
    /* Prueba de bajo nivel estrictamente no destructiva. La prueba de escritura
     * vive en FatFs y usa únicamente GEOTEST.BIN, que luego se elimina. */
    if (!sd_spi_read_block(0u, g_sd_scratch)) {
        return 0u;
    }
    for (i = 0u; i < 16u; i++) { (void)g_sd_scratch[i]; }
    g_sd_selftest_ok = 1u;
    return 1u;
}

void sd_spi_set_self_test_result(uint8 ok)
{
    g_sd_selftest_ok = ok ? 1u : 0u;
}
