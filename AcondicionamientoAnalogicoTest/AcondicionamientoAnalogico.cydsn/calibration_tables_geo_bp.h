#ifndef CALIBRATION_TABLES_GEO_BP_H
#define CALIBRATION_TABLES_GEO_BP_H

/* GEO_BP: VDAC_ref_BP, AMux_ADC=1. Se calibra siempre si el componente existe. */

#ifndef CAL_TARGET_GEO_BP_MV
#define CAL_TARGET_GEO_BP_MV 0L
#endif
#define CAL_TARGET_COUNTS_GEO_BP (CAL_TARGET_GEO_BP_MV * CAL_TARGET_1V_COUNTS / 1000L)

#define CAL_DIRECTION_GEO_BP 1

#ifndef CAL_ADELANTO_GEO_BP_MV
#define CAL_ADELANTO_GEO_BP_MV 2500L
#endif
/* Arranca en 0, que con polaridad es exactamente Vref: el punto natural
 * para empezar a anular un offset, y el unico desde el que se puede ir
 * para los dos lados. Antes era un adelanto fijo en mV sobre el LSB del
 * VDAC, que no tiene sentido en un IDAC con signo. */
#define CAL_DAC_CENTER_GEO_BP 0

#define CAL_DAC_MAX_CHANGE_GEO_BP CAL_IDAC_SIGNED_MAX

#ifndef CAL_PI_GAIN_GEO_BP_X1000
/* Ganancia fisica referencia -> tap, medida en la placa el 2026-09-02
 * con el barrido de D2, dividida por el escalon real de 1875 uV. */
#define CAL_PI_GAIN_GEO_BP_X1000 101L
#endif

/* Delta_BP usado: tres codigos, con holgura sobre el piso de un codigo. */
#ifndef CAL_PI_DEADBAND_GEO_BP_DAC_CODES
#define CAL_PI_DEADBAND_GEO_BP_DAC_CODES 1L
#endif

/* Kp y Ki de la simulacion Monte Carlo del lazo
 * (calculos_modelados/python/calibracion_pi). El barrido de robustez
 * contra tau no discrimina esta etapa porque su banda muerta es ancha y
 * arranca adentro, asi que manda la grilla con perturbacion real, donde
 * este par converge el 100 % con p95 de 122 a 1089 muestras segun la
 * etapa. Igual quedo marcado como robusto para todos los tau probados. */
#define CAL_PI_KP_NUM_GEO_BP 2L
#define CAL_PI_KP_DIV_GEO_BP 1L
#define CAL_PI_KI_NUM_GEO_BP 1L
#define CAL_PI_KI_DIV_GEO_BP 1L

#ifndef CAL_PI_LOCK_SAMPLES_GEO_BP
#define CAL_PI_LOCK_SAMPLES_GEO_BP 1024u
#endif

#ifndef CAL_PI_SETTLE_SAMPLES_GEO_BP
#define CAL_PI_SETTLE_SAMPLES_GEO_BP 512u
#endif

#ifndef CAL_PI_TIMEOUT_SAMPLES_GEO_BP
#define CAL_PI_TIMEOUT_SAMPLES_GEO_BP 45000u
#endif

#ifndef CAL_PI_REFINE_ENABLE_GEO_BP
#define CAL_PI_REFINE_ENABLE_GEO_BP 1u
#endif

#ifndef CAL_PI_REFINE_SETTLE_SAMPLES_GEO_BP
#define CAL_PI_REFINE_SETTLE_SAMPLES_GEO_BP 1024u
#endif

#endif
