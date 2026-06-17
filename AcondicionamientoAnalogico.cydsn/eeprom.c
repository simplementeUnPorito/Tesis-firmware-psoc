/*
 * eeprom.c
 *
 *  Created on: 31 dic. 2017
 *      Author: alumno-lp1
 */

#include <stdint.h>

#include "project.h"
#include "crc16.h"
#include "menu.h"

#define TAM_BLOQUE_EEPROM 16
#define N_BLOQUES_CONFIG_EEPROM (sizeof(struct configNodo)/TAM_BLOQUE_EEPROM)
#define BLOQUE_ADICIONAL_CONFIG_EEPROM ((sizeof(struct configNodo) - (N_BLOQUES_CONFIG_EEPROM * TAM_BLOQUE_EEPROM)) ? 1 : 0)
#define N_BLOQUES_CONFIG_EEPROM_TOTAL (N_BLOQUES_CONFIG_EEPROM + BLOQUE_ADICIONAL_CONFIG_EEPROM)

/*
 * Variable que contiene el CRC calculado para las funciones de este archivo
 */
uint16_t crc;

/*
 * Función que escribe la configuración en la EEPROM, se escribe por filas de 16 bytes cada una.
 * En total se escriben un número entero de filas, que puede ser mayor o igual a las necesarias.
 * El CRC se escribe en la fila siguiente, en los dos primeros bytes.
 * Esta función calcula el CRC antes de escribir la EEPROM.
 * Parámetros:
 * 	config ->  puntero a la posición donde se encuentra la configuración.
 * Retorno:
 * 	0 si no hubo error, si ocurrió algún error retorno -1;
 */
uint8_t escribeConfigEEPROM (struct configNodo *config){
	uint8_t i;
	uint8_t *punt = (uint8_t *) config;

	EEPROM_CONFIGURACION_UpdateTemperature();

	// calculamos CRC
	crc = crc16((uint8_t *) config, sizeof(struct configNodo));
	// escribimos la configuración
	for (i = 0; i < N_BLOQUES_CONFIG_EEPROM_TOTAL; i++){
		// escribimos en bloques de 16 bytes
		if (EEPROM_CONFIGURACION_Write(punt + (i * TAM_BLOQUE_EEPROM), i) != CYRET_SUCCESS){
			return -1;
		}
	}
	// escribimos el CRC
	if (EEPROM_CONFIGURACION_ByteWritePos((uint8_t)(crc & 0xFF), i, 0) != CYRET_SUCCESS){			// primera posición de la siguiente fila de la EEPROM
		return -1;
	}
	if (EEPROM_CONFIGURACION_ByteWritePos((uint8_t)((crc >> 8) & 0xFF), i, 1)!= CYRET_SUCCESS){	// segunda posición de la siguiente fila de la EEPROM
		return -1;
	}

	return 0;
}


/*
 * Función que lee la configuración de la EEPROM, lo mismo que el CRC guardado.
 * Esta función verifica el CRC.
 * Parámetros:
 * 	config ->  puntero a la posición donde se guarda la configuración.
 * 	crcDest -> puntero a la posición donde se guarda el CRC leído
 * Retorno:
 * 	0 si el CRC es correcto, si el CRC no es correcto retorna -1;
 */
uint8_t leeConfigEEPROM (struct configNodo *config, uint16_t *crcDest){
	uint32_t i;
	uint8_t *punt = (uint8_t *)config;

	// leemos la configuración
	for (i = 0; i < sizeof(struct configNodo); i++, punt++){
		// leemos por bytes
		*punt = EEPROM_CONFIGURACION_ReadByte(i);
	}

	// leemos el CRC de la EEPROM
	*crcDest = EEPROM_CONFIGURACION_ReadByte(N_BLOQUES_CONFIG_EEPROM_TOTAL * TAM_BLOQUE_EEPROM) |
			EEPROM_CONFIGURACION_ReadByte((N_BLOQUES_CONFIG_EEPROM_TOTAL * TAM_BLOQUE_EEPROM) + 1) << 8;

	// calculamos CRC
	crc = crc16((uint8_t *) config, sizeof(struct configNodo));

	if (crc == *crcDest) {	// Comparamos el CRC calculado con el leído
		return 0;
	} else {
		return -1;
	}
}
