/*
 * eeprom.h
 *
 *  Created on: 31 dic. 2017
 *      Author: alumno-lp1
 */

#ifndef EEPROM_H_
#define EEPROM_H_

#include <stdint.h>
    
#include "menu.h"
    
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
uint8_t escribeConfigEEPROM (struct configNodo *config);

/*
 * Función que lee la configuración de la EEPROM, lo mismo que el CRC guardado.
 * Esta función verifica el CRC.
 * Parámetros:
 * 	config ->  puntero a la posición donde se guarda la configuración.
 * 	crcDest -> puntero a la posición donde se guarda el CRC leído
 * Retorno:
 * 	0 si el CRC es correcto, si el CRC no es correcto retorna -1;
 */
uint8_t leeConfigEEPROM (struct configNodo *config, uint16_t *crcDest);

#endif /* EEPROM_H_ */
