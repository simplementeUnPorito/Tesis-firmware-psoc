#ifndef FILTER_COEFFS_H
#define FILTER_COEFFS_H

#include "project.h"

/* Cantidad de taps del FIR cargado en el componente Filter (DFB), Canal A.
 * DEBE coincidir exactamente con el "Number of Taps" configurado en el
 * customizer del componente Filter en TopDesign (PSoC Creator) -- ver el
 * #error mas abajo, que lo verifica contra Filter_FIR_A_SIZE (generado por
 * PSoC Creator a partir de esa misma configuracion). Si en el futuro se
 * reconfigura el Filter a otro N de taps (p.ej. para usar un diseño FIR
 * generado desde MATLAB, ver src/matlab/Simulaciones Controladores/Desacople),
 * hay que actualizar este valor Y regenerar el coeficiente por defecto de
 * abajo -- el sistema de generacion de coeficientes vive aparte de esto. */
#define FILTER_FIR_NTAPS 128u

/* Formato nativo de coeficiente del DFB: 4 bytes por tap, little-endian,
 * Q1.23 en los 3 bytes bajos + 0x00 de relleno en el byte alto (exactamente
 * el layout que usa Filter_ChannelAFirCoefficients en el
 * Filter_Coefficients.c generado por PSoC Creator -- ver Filter_Init() en
 * Filter.c, que hace memcpy(Filter_DB_RAM, Filter_data_b, ...) directo,
 * sin convertir nada). [0..3]=tap mas viejo. Los DOS juegos de valores
 * (adquisicion vs. calibracion) viven en FIR_adquisition.h/.c y
 * FIR_calibration.h/.c, declarados `const uint8 CYCODE [...][Filter_FIR_A_SIZE]`
 * -- para cargar un diseño nuevo, pegar el array que genera el customizer del
 * componente Filter (PSoC Creator) tal cual ahi; esta funcion es la unica
 * pieza de mecanismo que hace falta tocar para que el cambio tenga efecto. */

/* Reemplaza en caliente los coeficientes del Canal A del Filter (DFB) sin
 * pasar por PSoC Creator. Requiere que el componente ya este configurado
 * para FIR con exactamente ntaps taps (ver advertencia arriba) -- si ntaps no
 * coincide con FILTER_FIR_NTAPS no se escribe nada y devuelve 0, para no
 * corromper la RAM de coeficientes con un tamaño que no es el que el
 * microcodigo del DFB espera. coeffs_bytes debe tener ntaps*4 elementos
 * (mismo layout que Filter_ChannelAFirCoefficients). Devuelve 1 si cargo OK. */
uint8 psoc_filter_load_fir_coefficients(const uint8 *coeffs_bytes, uint16 ntaps);

/* Borra solo la linea de retardo/memoria de muestras del FIR manteniendo los
 * coeficientes actuales. Se usa al cambiar de etapa de calibracion para que
 * el DC de una etapa no arrastre historia de la etapa anterior. */
void psoc_filter_reset_history(void);

#endif
