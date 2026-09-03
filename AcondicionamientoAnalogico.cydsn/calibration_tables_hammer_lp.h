#ifndef CALIBRATION_TABLES_HAMMER_LP_H
#define CALIBRATION_TABLES_HAMMER_LP_H

/* HAMMER_LP: VDAC_LP, AMux_ADC=1. */

#ifndef CAL_TARGET_HAMMER_LP_MV
#define CAL_TARGET_HAMMER_LP_MV 3500L
#endif
#define CAL_TARGET_HAMMER_LP_COUNTS (CAL_TARGET_HAMMER_LP_MV * CAL_TARGET_1V_COUNTS / 1000L)

#define CAL_DIRECTION_HAMMER_LP 1

#ifndef CAL_ADELANTO_HAMMER_LP_MV
#define CAL_ADELANTO_HAMMER_LP_MV 1280L
#endif
/* Arranca en 0, que con polaridad es exactamente Vref: el punto natural
 * para empezar a anular un offset, y el unico desde el que se puede ir
 * para los dos lados. Antes era un adelanto fijo en mV sobre el LSB del
 * VDAC, que no tiene sentido en un IDAC con signo. */
#define CAL_DAC_CENTER_HAMMER_LP 0

#define CAL_DAC_MAX_CHANGE_HAMMER_LP CAL_IDAC_SIGNED_MAX

#ifndef CAL_PI_GAIN_HAMMER_LP_X1000
/* Ganancia fisica referencia -> tap, medida en la placa el 2026-09-02
 * con el barrido de D2, dividida por el escalon real de 1875 uV. */
#define CAL_PI_GAIN_HAMMER_LP_X1000 651L
#endif

/* 0 conserva la seleccion automatica a partir de la ganancia fisica. */
#ifndef CAL_PI_DEADBAND_HAMMER_LP_DAC_CODES
#define CAL_PI_DEADBAND_HAMMER_LP_DAC_CODES 1L
#endif

/* Kp y Ki de la simulacion Monte Carlo del lazo
 * (calculos_modelados/python/calibracion_pi). El barrido de robustez
 * contra tau no discrimina esta etapa porque su banda muerta es ancha y
 * arranca adentro, asi que manda la grilla con perturbacion real, donde
 * este par converge el 100 % con p95 de 122 a 1089 muestras segun la
 * etapa. Igual quedo marcado como robusto para todos los tau probados. */
#define CAL_PI_KP_NUM_HAMMER_LP 2L
#define CAL_PI_KP_DIV_HAMMER_LP 1L
#define CAL_PI_KI_NUM_HAMMER_LP 1L
#define CAL_PI_KI_DIV_HAMMER_LP 1L

#ifndef CAL_PI_LOCK_SAMPLES_HAMMER_LP
#define CAL_PI_LOCK_SAMPLES_HAMMER_LP 1024u
#endif

#ifndef CAL_PI_SETTLE_SAMPLES_HAMMER_LP
#define CAL_PI_SETTLE_SAMPLES_HAMMER_LP 512u
#endif

#ifndef CAL_PI_TIMEOUT_SAMPLES_HAMMER_LP
#define CAL_PI_TIMEOUT_SAMPLES_HAMMER_LP 45000u
#endif

#ifndef CAL_PI_REFINE_ENABLE_HAMMER_LP
#define CAL_PI_REFINE_ENABLE_HAMMER_LP 1u
#endif

#ifndef CAL_PI_REFINE_SETTLE_SAMPLES_HAMMER_LP
#define CAL_PI_REFINE_SETTLE_SAMPLES_HAMMER_LP 2048u
#endif

#endif
