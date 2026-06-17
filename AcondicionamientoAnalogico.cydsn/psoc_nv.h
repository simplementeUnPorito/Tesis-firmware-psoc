#ifndef PSOC_NV_H
#define PSOC_NV_H

/*
 * psoc_nv.h — Almacenamiento no volátil en EEPROM PSoC.
 *
 * Guarda los 4 valores DAC de calibración (uno por etapa GEO) con CRC-16.
 * Al arrancar, si el CRC es válido, se cargan como punto de partida de la
 * calibración para converger más rápido que desde el default 0x9C.
 *
 * Layout EEPROM (2 filas de 16 bytes cada una):
 *   Row 0 [0..15]: magic(1) + cal_dac[4] + pad[11]
 *   Row 1 [16..31]: crc_lo(1) + crc_hi(1) + pad[14]
 */

#include "project.h"

#define PSOC_NV_MAGIC        0xCAu
#define PSOC_NV_CAL_STAGES   4u
#define PSOC_NV_ROW_SIZE     16u

/*
 * Guarda los cal_count valores DAC en EEPROM.
 * Retorna 1 si éxito, 0 si error de escritura.
 * Llama EEPROM_UpdateTemperature() internamente.
 */
uint8 psoc_nv_save(const uint8 *cal_dac, uint8 cal_count);

/*
 * Lee y verifica el CRC de la EEPROM.
 * Si magic y CRC son válidos, llena cal_dac[cal_count] y retorna 1.
 * Retorna 0 si EEPROM en blanco, CRC inválido o magic incorrecto.
 */
uint8 psoc_nv_load(uint8 *cal_dac, uint8 cal_count);

#endif /* PSOC_NV_H */
