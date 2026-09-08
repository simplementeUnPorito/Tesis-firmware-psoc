#ifndef CALIBRATION_TABLES_GEO_LP_H
#define CALIBRATION_TABLES_GEO_LP_H

/* GEO_LP: VDAC_ref_LP, AMux_ADC=3, canal de captura final. */

#ifndef CAL_TARGET_GEO_LP_MV
#define CAL_TARGET_GEO_LP_MV 0L
#endif
#define CAL_TARGET_COUNTS_GEO_LP (CAL_TARGET_GEO_LP_MV * CAL_TARGET_1V_COUNTS / 1000L)

#define CAL_DIRECTION_GEO_LP 1

#ifndef CAL_ADELANTO_GEO_LP_MV
#define CAL_ADELANTO_GEO_LP_MV 2500L
#endif
/* Arranca en 0, que con polaridad es exactamente Vref: el punto natural
 * para empezar a anular un offset, y el unico desde el que se puede ir
 * para los dos lados. Antes era un adelanto fijo en mV sobre el LSB del
 * VDAC, que no tiene sentido en un IDAC con signo. */
#define CAL_DAC_CENTER_GEO_LP 0

/* La PC validó semillas hasta +120. Se deja margen acotado hasta 160, no el
 * fondo de escala heredado, para impedir saltos grandes ante ruido de 50 Hz. */
#define CAL_DAC_MAX_CHANGE_GEO_LP 160
/* Simétrica dentro del límite de seguridad de software. */
#define CAL_DAC_MAX_CHANGE_NEG_GEO_LP 160

#ifndef CAL_PI_GAIN_GEO_LP_X1000
/* Ganancia fisica referencia -> tap, medida en la placa el 2026-09-02
 * con el barrido de D2, dividida por el escalon real de 1875 uV. */
#define CAL_PI_GAIN_GEO_LP_X1000 295L
#endif

#ifndef CAL_PI_DEADBAND_GEO_LP_COUNTS
#define CAL_PI_DEADBAND_GEO_LP_COUNTS 39L
#endif

/* Kp y Ki de la simulacion Monte Carlo del lazo
 * (calculos_modelados/python/calibracion_pi). El barrido de robustez
 * contra tau no discrimina esta etapa porque su banda muerta es ancha y
 * arranca adentro, asi que manda la grilla con perturbacion real, donde
 * este par converge el 100 % con p95 de 122 a 1089 muestras segun la
 * etapa. Igual quedo marcado como robusto para todos los tau probados. */
#define CAL_PI_KP_NUM_GEO_LP 1L
#define CAL_PI_KP_DIV_GEO_LP 1L
#define CAL_PI_KI_NUM_GEO_LP 0L
#define CAL_PI_KI_DIV_GEO_LP 1L

#ifndef CAL_PI_LOCK_SAMPLES_GEO_LP
#define CAL_PI_LOCK_SAMPLES_GEO_LP 3u
#endif

#ifndef CAL_PI_SETTLE_SAMPLES_GEO_LP
#define CAL_PI_SETTLE_SAMPLES_GEO_LP CAL_PI_FIR_SETTLE_SAMPLES
#endif

/* Espera de la planta para esta etapa. Ver el bloque de
 * CAL_PI_PLANT_SETTLE_* en calibration_tables.h: es un concepto distinto
 * del vaciado del FIR de arriba, y hoy vale cero a proposito. */
#ifndef CAL_PI_PLANT_SETTLE_SAMPLES_GEO_LP
#define CAL_PI_PLANT_SETTLE_SAMPLES_GEO_LP CAL_PI_PLANT_SETTLE_SAMPLES_DEFAULT
#endif

#ifndef CAL_PI_TIMEOUT_SAMPLES_GEO_LP
/* Al LP le alcanza con pocos pasos: llega despues del ADDER, o sea con el tap ya
 * cerca, y su trabajo es el ajuste fino. Su autoridad sobre ch3 son 525 uV por
 * codigo por 255 codigos = 134 mV, asi que tampoco podria hacer mas: si al
 * llegar su turno todavia faltan volts, el problema es del ADDER y darle mas
 * tiempo al LP solo alarga la calibracion sin arreglar nada.
 *
 * 400.000 muestras a 2604 Hz son 154 s: cinco pasos de 1 tau mas margen. */
#define CAL_PI_TIMEOUT_SAMPLES_GEO_LP 400000u
#endif

#ifndef CAL_PI_REFINE_ENABLE_GEO_LP
#define CAL_PI_REFINE_ENABLE_GEO_LP 1u
#endif

#ifndef CAL_PI_REFINE_SETTLE_SAMPLES_GEO_LP
#define CAL_PI_REFINE_SETTLE_SAMPLES_GEO_LP CAL_PI_FIR_SETTLE_SAMPLES
#endif

#endif
