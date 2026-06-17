#include "psoc_nv.h"
#include "crc.h"

/* EEPROM row numbers */
#define NV_ROW_DATA 0u
#define NV_ROW_CRC  1u

uint8 psoc_nv_save(const uint8 *cal_dac, uint8 cal_count)
{
    uint8 row0[PSOC_NV_ROW_SIZE];
    uint8 row1[PSOC_NV_ROW_SIZE];
    uint16 crc;
    uint8 i;

    for (i = 0u; i < PSOC_NV_ROW_SIZE; i++) { row0[i] = 0u; row1[i] = 0u; }

    row0[0] = PSOC_NV_MAGIC;
    for (i = 0u; i < cal_count && i < PSOC_NV_CAL_STAGES; i++) {
        row0[1u + i] = cal_dac[i];
    }

    crc = crc16(row0, (uint16)PSOC_NV_ROW_SIZE);
    row1[0] = (uint8)(crc & 0xFFu);
    row1[1] = (uint8)((crc >> 8u) & 0xFFu);

    EEPROM_UpdateTemperature();
    if (EEPROM_Write(row0, NV_ROW_DATA) != CYRET_SUCCESS) { return 0u; }
    if (EEPROM_Write(row1, NV_ROW_CRC)  != CYRET_SUCCESS) { return 0u; }
    return 1u;
}

uint8 psoc_nv_load(uint8 *cal_dac, uint8 cal_count)
{
    uint8 row0[PSOC_NV_ROW_SIZE];
    uint16 crc_stored;
    uint16 crc_calc;
    uint8 i;

    for (i = 0u; i < PSOC_NV_ROW_SIZE; i++) {
        row0[i] = EEPROM_ReadByte((uint16)i);
    }

    if (row0[0] != PSOC_NV_MAGIC) { return 0u; }

    crc_stored = (uint16)EEPROM_ReadByte((uint16)PSOC_NV_ROW_SIZE) |
                 ((uint16)EEPROM_ReadByte((uint16)(PSOC_NV_ROW_SIZE + 1u)) << 8u);
    crc_calc   = crc16(row0, (uint16)PSOC_NV_ROW_SIZE);
    if (crc_calc != crc_stored) { return 0u; }

    for (i = 0u; i < cal_count && i < PSOC_NV_CAL_STAGES; i++) {
        cal_dac[i] = row0[1u + i];
    }
    return 1u;
}
