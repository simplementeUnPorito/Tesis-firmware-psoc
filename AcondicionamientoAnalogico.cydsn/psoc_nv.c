#include "psoc_nv.h"
#include "crc.h"
#include "psoc_hw.h"

#define NV_ROW_FOR_GAIN(gain) ((uint8)(gain))
#define NV_CRC_OFFSET 14u
/* Byte que estaba sin usar en la fila; guarda el signo de las cuatro etapas,
 * un bit por etapa, 1 = referencia por debajo de Vref. Queda dentro del CRC. */
#define NV_SIGN_OFFSET 10u

static uint8 nv_gain_valid(uint8 pga_code)
{
    return (pga_code < PSOC_NV_GAIN_SLOTS) ? 1u : 0u;
}

static uint16 nv_row_base(uint8 row)
{
    return ((uint16)row * (uint16)PSOC_NV_ROW_SIZE);
}

uint8 psoc_nv_save(const int16 *cal_dac, uint8 cal_count)
{
    return psoc_nv_save_for_gain(psoc_hw_get_pga_code(), cal_dac, cal_count);
}

uint8 psoc_nv_save_for_gain(uint8 pga_code, const int16 *cal_dac, uint8 cal_count)
{
    uint8 row0[PSOC_NV_ROW_SIZE];
    uint16 crc;
    uint8 i;
    uint8 valid_mask = 0u;

    if (!nv_gain_valid(pga_code) || cal_dac == (const int16 *)0) { return 0u; }
    if (cal_count == 0u || cal_count > PSOC_NV_CAL_STAGES) { return 0u; }

    for (i = 0u; i < PSOC_NV_ROW_SIZE; i++) { row0[i] = 0u; }
    row0[0] = PSOC_NV_MAGIC;
    row0[1] = PSOC_NV_VERSION;
    row0[2] = PSOC_HW_CLASS;
    row0[3] = pga_code;
    row0[4] = cal_count;
    {
        uint8 sign_mask = 0u;
        for (i = 0u; i < cal_count && i < PSOC_NV_CAL_STAGES; i++) {
            int16 code = psoc_hw_idac_clamp_signed(cal_dac[i]);
            valid_mask |= (uint8)(1u << i);
            if (code < 0) {
                sign_mask |= (uint8)(1u << i);
                code = (int16)(-code);
            }
            row0[6u + i] = (uint8)code;
        }
        row0[NV_SIGN_OFFSET] = sign_mask;
    }
    row0[5] = valid_mask;

    crc = crc16(row0, (uint16)NV_CRC_OFFSET);
    row0[NV_CRC_OFFSET] = (uint8)(crc & 0xFFu);
    row0[NV_CRC_OFFSET + 1u] = (uint8)((crc >> 8u) & 0xFFu);

    EEPROM_UpdateTemperature();
    if (EEPROM_Write(row0, NV_ROW_FOR_GAIN(pga_code)) != CYRET_SUCCESS) { return 0u; }
    return 1u;
}

uint8 psoc_nv_load(int16 *cal_dac, uint8 cal_count)
{
    return psoc_nv_load_for_gain(psoc_hw_get_pga_code(), cal_dac, cal_count);
}

uint8 psoc_nv_load_for_gain(uint8 pga_code, int16 *cal_dac, uint8 cal_count)
{
    uint8 row0[PSOC_NV_ROW_SIZE];
    uint16 crc_stored;
    uint16 crc_calc;
    uint8 i;
    uint16 base;
    uint8 valid_mask;

    if (!nv_gain_valid(pga_code) || cal_dac == (int16 *)0) { return 0u; }
    if (cal_count == 0u || cal_count > PSOC_NV_CAL_STAGES) { return 0u; }

    base = nv_row_base(NV_ROW_FOR_GAIN(pga_code));
    for (i = 0u; i < PSOC_NV_ROW_SIZE; i++) {
        row0[i] = EEPROM_ReadByte((uint16)(base + i));
    }

    if (row0[0] != PSOC_NV_MAGIC) { return 0u; }
    if (row0[1] != PSOC_NV_VERSION) { return 0u; }
    if (row0[2] != PSOC_HW_CLASS) { return 0u; }
    if (row0[3] != pga_code) { return 0u; }
    if (row0[4] != cal_count) { return 0u; }

    crc_stored = (uint16)row0[NV_CRC_OFFSET] |
                 ((uint16)row0[NV_CRC_OFFSET + 1u] << 8u);
    crc_calc   = crc16(row0, (uint16)NV_CRC_OFFSET);
    if (crc_calc != crc_stored) { return 0u; }

    valid_mask = row0[5];
    if ((valid_mask & (uint8)((1u << cal_count) - 1u)) !=
        (uint8)((1u << cal_count) - 1u)) {
        return 0u;
    }

    {
        uint8 sign_mask = row0[NV_SIGN_OFFSET];
        for (i = 0u; i < cal_count && i < PSOC_NV_CAL_STAGES; i++) {
            int16 code = (int16)row0[6u + i];
            if (sign_mask & (uint8)(1u << i)) {
                code = (int16)(-code);
            }
            cal_dac[i] = code;
        }
    }
    return 1u;
}
