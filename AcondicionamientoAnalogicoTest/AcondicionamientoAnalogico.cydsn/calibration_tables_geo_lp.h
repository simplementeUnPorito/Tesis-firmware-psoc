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

#define CAL_DAC_MAX_CHANGE_GEO_LP CAL_IDAC_SIGNED_MAX

#ifndef CAL_PI_GAIN_GEO_LP_X1000
/* Ganancia fisica referencia -> tap, medida en la placa el 2026-09-02
 * con el barrido de D2, dividida por el escalon real de 1875 uV. */
#define CAL_PI_GAIN_GEO_LP_X1000 651L
#endif

#ifndef CAL_PI_DEADBAND_GEO_LP_DAC_CODES
#define CAL_PI_DEADBAND_GEO_LP_DAC_CODES 1L
#endif

/* P=100e-6, I=500e-6. */
#define CAL_PI_KP_NUM_GEO_LP 1L
#define CAL_PI_KP_DIV_GEO_LP 10000L
#define CAL_PI_KI_NUM_GEO_LP 1L
#define CAL_PI_KI_DIV_GEO_LP 2000L

#ifndef CAL_PI_LOCK_SAMPLES_GEO_LP
#define CAL_PI_LOCK_SAMPLES_GEO_LP 1024u
#endif

#ifndef CAL_PI_SETTLE_SAMPLES_GEO_LP
#define CAL_PI_SETTLE_SAMPLES_GEO_LP 512u
#endif

#ifndef CAL_PI_TIMEOUT_SAMPLES_GEO_LP
#define CAL_PI_TIMEOUT_SAMPLES_GEO_LP 45000u
#endif

#ifndef CAL_PI_REFINE_ENABLE_GEO_LP
#define CAL_PI_REFINE_ENABLE_GEO_LP 1u
#endif

#ifndef CAL_PI_REFINE_SETTLE_SAMPLES_GEO_LP
#define CAL_PI_REFINE_SETTLE_SAMPLES_GEO_LP 2048u
#endif

#endif
