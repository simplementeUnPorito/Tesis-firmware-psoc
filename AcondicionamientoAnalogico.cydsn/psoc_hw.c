#include "psoc_hw.h"
#include "sd_spi.h"

static uint8 g_psoc_hw_pga_code = PSOC_PGA_DEFAULT_CODE;

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

void psoc_hw_start_analog(uint8 pga_code, uint8 pgavdac_code)
{
#if PSOC_HW_CLASS == PSOC_HW_GEO
    OPAref_Start();
    PGAshield_Start();
    PGAp_Start();
    PGAn_Start();
    PGAp_SetGain(PGAp_GAIN_02);
    PGAn_SetGain(PGAn_GAIN_02);
    LPF_1_Start();
    //LPF_2_Start();
    PGAgain_Start();
    psoc_hw_set_pga(pga_code);
    OPAbp_Start();
    OPAadder_Start();
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
#endif
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

/* CS (P2.3) quedó ruteado al `ss` de hardware del BSPIM, que deassertea entre
 * bytes. Se lo desengancha del DSI con el bypass del puerto (patrón calcado de
 * tx1_gpio_detach_dsi) para manejarlo por software durante toda la transacción. */
static void sd_cs_detach_dsi(void)
{
    reg8 *outSel0 = (reg8 *)SPIp__PRTDSI__OUT_SEL0;
    reg8 *outSel1 = (reg8 *)SPIp__PRTDSI__OUT_SEL1;
    reg8 *oeSel0  = (reg8 *)SPIp__PRTDSI__OE_SEL0;
    reg8 *oeSel1  = (reg8 *)SPIp__PRTDSI__OE_SEL1;
    reg8 *byp     = (reg8 *)SPIp__BYP;
    reg8 *dr      = (reg8 *)SPIp__DR;
    uint8 saved   = CyEnterCriticalSection();

    /* DR alto ANTES del bypass para que el pin no glitchee a 0 al cambiar de
     * fuente (la SD interpreta CS bajo como selección). */
    *dr      |= SPIp__CS__MASK;
    *outSel0 &= (uint8)~SPIp__CS__MASK;
    *outSel1 &= (uint8)~SPIp__CS__MASK;
    *oeSel0  &= (uint8)~SPIp__CS__MASK;
    *oeSel1  &= (uint8)~SPIp__CS__MASK;
    *byp     |= SPIp__CS__MASK;
    CyExitCriticalSection(saved);
    CyPins_SetPinDriveMode(SPIp_CS, CY_PINS_DM_STRONG);
}

static void sd_cs_write(uint8 value)
{
    reg8 *dr = (reg8 *)SPIp__DR;
    uint8 saved = CyEnterCriticalSection();
    if (value) {
        *dr |= SPIp__CS__MASK;
    } else {
        *dr &= (uint8)~SPIp__CS__MASK;
    }
    CyExitCriticalSection(saved);
}

/* Un byte full-duplex. Lazo acotado: a 187.5 kbps un byte tarda ~43 us; el
 * guard de 60000 vueltas es órdenes de magnitud más que eso y evita colgar el
 * main loop si el clock/SPI quedara en un estado imposible. */
static uint8 sd_xfer(uint8 v)
{
    uint16 guard = 60000u;
    SPI_WriteTxData(v);
    while (0u == (SPI_ReadRxStatus() & SPI_STS_RX_FIFO_NOT_EMPTY)) {
        if (--guard == 0u) {
            return 0xFFu;
        }
    }
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

    r1 = sd_cmd(8u, 0x000001AAu, 0x87u);
    if (r1 == SD_R1_IDLE) {
        /* SD v2: leer R7 y verificar echo del patrón. */
        for (i = 0u; i < 4u; i++) {
            ocr[i] = sd_xfer(0xFFu);
        }
        if ((ocr[2] != 0x01u) || (ocr[3] != 0xAAu)) {
            goto done;
        }
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
        if (sd_cmd(16u, (uint32)SD_BLOCK_BYTES, 0xFFu) != 0x00u) {
            g_sd_type = SD_TYPE_NONE;
            goto done;
        }
    }
    if (!sd_read_register(9u, g_sd_scratch)) {
        g_sd_type = SD_TYPE_NONE;
        goto done;
    }
    g_sd_sector_count = sd_sector_count_from_csd(g_sd_scratch);
    if (g_sd_sector_count == 0u) {
        g_sd_type = SD_TYPE_NONE;
        goto done;
    }
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
