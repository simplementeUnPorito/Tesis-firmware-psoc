#ifndef CALIBRATION_TABLES_GEO_SUM_H
#define CALIBRATION_TABLES_GEO_SUM_H

/* GEO_SUM: VDAC_Ref_Sum, AMux_ADC=2. */

#ifndef CAL_TARGET_GEO_SUM_MV
#define CAL_TARGET_GEO_SUM_MV 0L
#endif
#define CAL_TARGET_COUNTS_GEO_SUM (CAL_TARGET_GEO_SUM_MV * CAL_TARGET_1V_COUNTS / 1000L)

#define CAL_DIRECTION_GEO_SUM 1

#ifndef CAL_ADELANTO_GEO_SUM_MV
#define CAL_ADELANTO_GEO_SUM_MV 2500L
#endif
/* Arranca en 0, que con polaridad es exactamente Vref: el punto natural
 * para empezar a anular un offset, y el unico desde el que se puede ir
 * para los dos lados. Antes era un adelanto fijo en mV sobre el LSB del
 * VDAC, que no tiene sentido en un IDAC con signo. */
#define CAL_DAC_CENTER_GEO_SUM 0

#define CAL_DAC_MAX_CHANGE_GEO_SUM CAL_IDAC_SIGNED_MAX

#ifndef CAL_PI_GAIN_GEO_SUM_X1000
/* Ganancia fisica referencia -> tap, medida en la placa el 2026-09-02
 * con el barrido de D2, dividida por el escalon real de 1875 uV. */
#define CAL_PI_GAIN_GEO_SUM_X1000 411L
#endif

#ifndef CAL_PI_DEADBAND_GEO_SUM_DAC_CODES
#define CAL_PI_DEADBAND_GEO_SUM_DAC_CODES 1L
#endif

/* Kp y Ki de la simulacion Monte Carlo del lazo
 * (calculos_modelados/python/calibracion_pi). El barrido de robustez
 * contra tau no discrimina esta etapa porque su banda muerta es ancha y
 * arranca adentro, asi que manda la grilla con perturbacion real, donde
 * este par converge el 100 % con p95 de 122 a 1089 muestras segun la
 * etapa. Igual quedo marcado como robusto para todos los tau probados. */
#define CAL_PI_KP_NUM_GEO_SUM 2L
#define CAL_PI_KP_DIV_GEO_SUM 1L
#define CAL_PI_KI_NUM_GEO_SUM 1L
#define CAL_PI_KI_DIV_GEO_SUM 1L

#ifndef CAL_PI_LOCK_SAMPLES_GEO_SUM
#define CAL_PI_LOCK_SAMPLES_GEO_SUM 512u
#endif

#ifndef CAL_PI_SETTLE_SAMPLES_GEO_SUM
#define CAL_PI_SETTLE_SAMPLES_GEO_SUM 512u
#endif

#ifndef CAL_PI_TIMEOUT_SAMPLES_GEO_SUM
#define CAL_PI_TIMEOUT_SAMPLES_GEO_SUM CAL_PI_DEFAULT_TIMEOUT_SAMPLES
#endif

#ifndef CAL_PI_REFINE_ENABLE_GEO_SUM
#define CAL_PI_REFINE_ENABLE_GEO_SUM 1u
#endif

#ifndef CAL_PI_REFINE_SETTLE_SAMPLES_GEO_SUM
#define CAL_PI_REFINE_SETTLE_SAMPLES_GEO_SUM 1024u
#endif

#endif
