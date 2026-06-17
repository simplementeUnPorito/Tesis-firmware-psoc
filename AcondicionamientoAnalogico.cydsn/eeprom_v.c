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
#include "project.h"

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
int escribirEEPROM (uint8_t *p, uint16_t tam, uint16_t crc) {
    uint16_t i, veces;
    cystatus ret;
    
    if (tam > CYDEV_EE_SIZE) {
        // no hay espacio suficiente en la EEPROM
        return CYRET_UNKNOWN;
    }
    
    if ((tam % CYDEV_EEPROM_ROW_SIZE) == 0){
        // es múltiplo exacto
        veces = tam / CYDEV_EEPROM_ROW_SIZE;
    } else {
        veces = (tam / CYDEV_EEPROM_ROW_SIZE) +1;
    }
    
    for (i = 0; i < veces; i++, p += CYDEV_EEPROM_ROW_SIZE) {
        if ((ret = EEPROM_Write(p, i)) != CYRET_SUCCESS) 
            return ret;
    }
    return EEPROM_Write((uint8_t *)&crc, i);
}

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
void leerEEPROM (uint8_t *p, uint16_t tam, uint16_t *crc) {
    uint16_t i, fila;
    uint8_t v1, v2;
    
    for (i = 0; i < tam; i++) {
        *p++ = EEPROM_ReadByte(i); 
    }
    
    // calculamos la cantidad de filas usadas
    if ((tam % CYDEV_EEPROM_ROW_SIZE) == 0){
        // es múltiplo exacto
        fila = tam / CYDEV_EEPROM_ROW_SIZE;
    } else {
        fila = (tam / CYDEV_EEPROM_ROW_SIZE) +1;
    }
    // leemos el CRC
    v1 = EEPROM_ReadByte (fila * CYDEV_EEPROM_ROW_SIZE);
    v2 = EEPROM_ReadByte ((fila * CYDEV_EEPROM_ROW_SIZE) +1);
    *crc = v2 << 8 | v1;
}

/* [] END OF FILE */
