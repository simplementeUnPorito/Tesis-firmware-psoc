#ifndef PSOC_NV_H
#define PSOC_NV_H

/*
 * psoc_nv.h — Almacenamiento no volátil en EEPROM PSoC.
 *
 * Guarda los valores DAC de calibración por código de ganancia PGA con CRC-16.
 * Al arrancar o cambiar PGA, si existe un slot válido para esa ganancia, se
 * carga como punto de partida de la calibración.
 *
 * Layout EEPROM v2 (1 fila de 16 bytes por ganancia PGA 0..8):
 *   [0] magic, [1] version, [2] hw_class, [3] pga_code, [4] stage_count,
 *   [5] valid_mask, [6..9] cal_dac[4], [10..13] reserved, [14..15] crc_le.
 */

#include "project.h"

#define PSOC_NV_MAGIC        0xCAu
#define PSOC_NV_VERSION      0x02u
#define PSOC_NV_CAL_STAGES   4u
#define PSOC_NV_ROW_SIZE     16u
#define PSOC_NV_GAIN_SLOTS   9u

/*
 * Guarda los cal_count valores DAC en EEPROM.
 * Retorna 1 si éxito, 0 si error de escritura.
 * Llama EEPROM_UpdateTemperature() internamente.
 */
uint8 psoc_nv_save(const uint8 *cal_dac, uint8 cal_count);
uint8 psoc_nv_save_for_gain(uint8 pga_code, const uint8 *cal_dac, uint8 cal_count);

/*
 * Lee y verifica el CRC de la EEPROM.
 * Si magic y CRC son válidos, llena cal_dac[cal_count] y retorna 1.
 * Retorna 0 si EEPROM en blanco, CRC inválido o magic incorrecto.
 */
uint8 psoc_nv_load(uint8 *cal_dac, uint8 cal_count);
uint8 psoc_nv_load_for_gain(uint8 pga_code, uint8 *cal_dac, uint8 cal_count);

#endif /* PSOC_NV_H */
