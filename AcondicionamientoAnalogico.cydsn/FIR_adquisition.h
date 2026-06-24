#ifndef FIR_ADQUISITION_H
#define FIR_ADQUISITION_H

#include "filter_coeffs.h"
#include "Filter.h"

/* Coeficientes del Filter (DFB) usados en modo "adquisicion": el stream
 * filtrado que se manda al ESP/web cuando PSOC_CMD_SELECT_STREAM pide
 * filtrado (g_stream_mode=1, ver main.c). Formato nativo (4 bytes/tap, ver
 * filter_coeffs.h) -- mismo array que genera el customizer del componente
 * Filter en PSoC Creator, se puede pegar tal cual.
 * Para cambiar el filtro de adquisicion: editar SOLO los valores de este
 * arreglo en FIR_adquisition.c (debe seguir teniendo Filter_FIR_A_SIZE
 * elementos) -- no hace falta tocar psoc_filter_load_fir_coefficients() ni
 * el resto del mecanismo. */
extern const uint8 CYCODE g_fir_adquisition_coeffs_q23[Filter_FIR_A_SIZE];

#endif
