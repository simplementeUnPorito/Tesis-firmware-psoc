#ifndef FIR_CALIBRATION_H
#define FIR_CALIBRATION_H

#include "filter_coeffs.h"

/* Coeficientes del Filter (DFB) usados en modo "calibracion": el PI de
 * calibracion (cal_pi_run_service, calibration.c) lee fir_output desde la
 * salida del Filter de hardware, asi que el suavizado que reciba viene de
 * ESTE arreglo (se carga al empezar una corrida de calibracion y se
 * restaura el de adquisicion al terminar -- ver psoc_calibration_start_async
 * / cal_async_complete). Formato Q1.23, ver filter_coeffs.h.
 * Para cambiar el filtro de calibracion: editar SOLO los valores de este
 * arreglo en FIR_calibration.c (debe seguir teniendo FILTER_FIR_NTAPS
 * elementos) -- no hace falta tocar psoc_filter_load_fir_coefficients() ni
 * el resto del mecanismo. */
extern const int32 g_fir_calibration_coeffs_q23[FILTER_FIR_NTAPS];

#endif
