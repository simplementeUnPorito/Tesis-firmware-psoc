#ifndef FIR_CALIBRATION_H
#define FIR_CALIBRATION_H

#include "filter_coeffs.h"
#include "Filter.h"

/* Coeficientes del Filter (DFB) usados en modo "calibracion": el PI de
 * calibracion (cal_pi_run_service, calibration.c) lee fir_output desde la
 * salida del Filter de hardware, asi que el suavizado que reciba viene de
 * ESTE arreglo (se carga al empezar una corrida de calibracion y se
 * restaura el de adquisicion al terminar -- ver psoc_calibration_start_async
 * / cal_async_complete). Formato nativo (4 bytes/tap, ver filter_coeffs.h) --
 * mismo array que genera el customizer del componente Filter en PSoC
 * Creator, se puede pegar tal cual.
 * Para cambiar el filtro de calibracion: editar SOLO los valores de este
 * arreglo en FIR_calibration.c (debe seguir teniendo Filter_FIR_A_SIZE
 * elementos) -- no hace falta tocar psoc_filter_load_fir_coefficients() ni
 * el resto del mecanismo. */
extern const uint8 CYCODE g_fir_calibration_coeffs_q23[Filter_FIR_A_SIZE];

#endif
