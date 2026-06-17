/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include <stdint.h>

/*
Función que escribe una estructura de datos en la EEPROM. 
Se escriben tantas filas como sean necesarias para almacenar la estructura.
Se utiliza una fila adicional para almacenar el crc.
Parámetros:
    p -> puntero a la posición inicial de la estructura
    tam -> cantidad de bytes que tiene la estructura
    crc -> valor del CRC calculado de la estructura que guardamos
Retorno:
    El valor que retorna la función EEPROM_Write de Cypress
*/
int escribirEEPROM (uint8_t *p, uint16_t tam, uint16_t crc);

/*
Función que lee una estructura de datos de la EEPROM. 
Se lee el CRC que se encuentra en la siguiente fila
Parámetros:
    p -> puntero a la posición inicial de la estructura
    tam -> cantidad de bytes que tiene la estructura
    crc -> puntero valor del CRC almacenado de la estructura
Retorno:
    Ninguno
*/
void leerEEPROM (uint8_t *p, uint16_t tam, uint16_t *crc);
/* [] END OF FILE */
