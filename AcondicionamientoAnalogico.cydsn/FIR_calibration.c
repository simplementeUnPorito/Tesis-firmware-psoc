#include "FIR_calibration.h"

/* Default = misma copia exacta del diseño ya horneado en el componente
 * Filter (Filter_Coefficients.c) que usa FIR_adquisition.c -- cargar esto es
 * un no-op verificable, punto de partida seguro. Reemplazar estos valores
 * (manteniendo FILTER_FIR_NTAPS elementos) por el diseño pensado para la
 * etapa de calibracion -- p.ej. el FIR de suavizado fuerte del modelo MATLAB
 * en src/matlab/Simulaciones Controladores/Desacople, una vez convertido a
 * Q1.23 con FILTER_FIR_NTAPS taps. */
const int32 g_fir_calibration_coeffs_q23[FILTER_FIR_NTAPS] = {
    2772, 3311, 3307, 2704, 1518, -146, -2076, -3961,
    -5423, -6074, -5597, -3834, -867, 2945, 6972, 10398,
    12359, 12123, 9286, 3925, -3319, -11277, -18406, -23051,
    -23788, -19766, -10988, 1543, 15875, 29359, 39095, 42492,
    37852, 24858, 4845, -19229, -43135, -62050, -71427, -67953,
    -50374, -20048, 18951, 60232, 95988, 118287, 120561, 99050,
    53920, -10165, -84186, -155673, -210294, -233921, -214878, -146013,
    -26248, 138664, 336372, 549137, 755910, 934976, 1066803, 1136669,
    1136669, 1066803, 934976, 755910, 549137, 336372, 138664, -26248,
    -146013, -214878, -233921, -210294, -155673, -84186, -10165, 53920,
    99050, 120561, 118287, 95988, 60232, 18951, -20048, -50374,
    -67953, -71427, -62050, -43135, -19229, 4845, 24858, 37852,
    42492, 39095, 29359, 15875, 1543, -10988, -19766, -23788,
    -23051, -18406, -11277, -3319, 3925, 9286, 12123, 12359,
    10398, 6972, 2945, -867, -3834, -5597, -6074, -5423,
    -3961, -2076, -146, 1518, 2704, 3307, 3311, 2772
};
