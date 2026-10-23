#include "psoc_hw.h"

static uint8 g_psoc_hw_pga_code    = PSOC_PGA_DEFAULT_CODE;
static uint8 g_psoc_hw_pgaout_code = PSOC_PGAOUT_DEFAULT_CODE;

/* Parámetros de la red IDAC->tensión de la placa. Globales (no macros) para
 * poder ajustarlas si cambia la resistencia sin recompilar las tablas. */
/* Espejo del Control Register: bit i = etapa i en modo sumidero. Se lleva en
 * RAM porque el registro es de solo escritura desde el punto de vista del
 * firmware y hay que poder tocar un bit sin pisar los otros tres. */
static uint8 g_psoc_idac_polarity = 0u;

int32 psoc_idac_code_to_uv_signed(int16 code)
{
    int32 magnitude = (code < 0) ? -(int32)code : (int32)code;
    int32 uv = (int32)(((int64)magnitude * (int64)g_psoc_idac_fullscale_na
                        * (int64)g_psoc_idac_rset_ohm)
                       / ((int64)PSOC_IDAC_CODE_MAX * 1000LL));

    return (code < 0) ? -uv : uv;
}

int32 psoc_idac_stage_code_to_uv_signed(uint8 stage, int16 code)
{
    int32 magnitude = (code < 0) ? -(int32)code : (int32)code;
    uint32 fullscale_na = (stage == 2u)
                        ? PSOC_IDAC_OPASUM_FULLSCALE_NA
                        : g_psoc_idac_fullscale_na;
    uint32 rset_ohm;
    switch (stage) {
        case 0u: rset_ohm = PSOC_IDAC_PGA_RSET_OHM; break;
        case 1u: rset_ohm = PSOC_IDAC_BP_RSET_OHM; break;
        case 2u: rset_ohm = PSOC_IDAC_OPASUM_RSET_OHM; break;
        case 3u: rset_ohm = PSOC_IDAC_LP_RSET_OHM; break;
        default: rset_ohm = g_psoc_idac_rset_ohm; break;
    }
    int32 uv = (int32)(((int64)magnitude * (int64)fullscale_na
                        * (int64)rset_ohm)
                       / ((int64)PSOC_IDAC_CODE_MAX * 1000LL));
    return (code < 0) ? -uv : uv;
}

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
        /* El bloque FF recibe 3 MHz. El divisor generado (2) daba 1 MHz y
         * dejaba muy poco margen de subida en la placa real; SCL llego a no
         * bajar de forma fiable. Divisor 7 => 3 MHz/(7+1) = 375 kHz. Mantiene
         * el mismo protocolo/direccion y dio ACK estable en el banco. */
        I2C_CLKDIV1_REG = 7u;
        I2C_CLKDIV2_REG = 0u;
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
