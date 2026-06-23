#ifndef FIR_ADQUISITION_H
#define FIR_ADQUISITION_H

#include "filter_coeffs.h"

/* Coeficientes del Filter (DFB) usados en modo "adquisicion": el stream
 * filtrado que se manda al ESP/web cuando PSOC_CMD_SELECT_STREAM pide
 * filtrado (g_stream_mode=1, ver main.c). Formato Q1.23, ver filter_coeffs.h.
 * Para cambiar el filtro de adquisicion: editar SOLO los valores de este
 * arreglo en FIR_adquisition.c (debe seguir teniendo FILTER_FIR_NTAPS
 * elementos) -- no hace falta tocar psoc_filter_load_fir_coefficients() ni
 * el resto del mecanismo. */
extern const int32 g_fir_adquisition_coeffs_q23[FILTER_FIR_NTAPS];

#endif
