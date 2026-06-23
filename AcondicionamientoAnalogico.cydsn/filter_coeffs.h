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

/* Formato nativo de coeficiente del DFB: Q1.23 (1 signo + 23 fraccionarios),
 * rango [-1, 1), 1 elemento por tap, [0]=tap mas viejo (igual orden que
 * Filter_data_b/Filter_ChannelAFirCoefficients). Los DOS juegos de valores
 * (adquisicion vs. calibracion) viven en FIR_adquisition.h/.c y
 * FIR_calibration.h/.c -- editar los valores ahí; esta funcion es la unica
 * pieza de mecanismo que hace falta tocar para que el cambio tenga efecto. */

/* Reemplaza en caliente los coeficientes del Canal A del Filter (DFB) sin
 * pasar por PSoC Creator. Requiere que el componente ya este configurado
 * para FIR con exactamente ntaps taps (ver advertencia arriba) -- si ntaps no
 * coincide con FILTER_FIR_NTAPS no se escribe nada y devuelve 0, para no
 * corromper la RAM de coeficientes con un tamaño que no es el que el
 * microcodigo del DFB espera. Devuelve 1 si cargo OK. */
uint8 psoc_filter_load_fir_coefficients(const int32 *coeffs_q23, uint16 ntaps);

#endif
