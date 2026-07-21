#ifndef CALIBRATION_TABLES_GEO_PGA_H
#define CALIBRATION_TABLES_GEO_PGA_H

/* GEO_PGA: VDAC_ref_PGA, AMux_ADC=0.
 * Ajustes activos del PI y del refinamiento de 1 LSB. */

#ifndef CAL_TARGET_GEO_PGA_MV
#define CAL_TARGET_GEO_PGA_MV 0L
#endif
#define CAL_TARGET_COUNTS_GEO_PGA (CAL_TARGET_GEO_PGA_MV * CAL_TARGET_1V_COUNTS / 1000L)

/* Para la topologia GEO, la ganancia DC VDAC->nodo PGA es +1 e independiente
 * de la ganancia diferencial programada. */
#define CAL_DIRECTION_GEO_PGA 1

#ifndef CAL_ADELANTO_GEO_PGA_MV
#define CAL_ADELANTO_GEO_PGA_MV 2500L
#endif
#define CAL_DAC_CENTER_GEO_PGA ((uint8)(CAL_ADELANTO_GEO_PGA_MV / CAL_VDAC8_MV_PER_LSB))

#define CAL_DAC_MAX_CHANGE_GEO_PGA 255u

#ifndef CAL_PI_GAIN_GEO_PGA_X1000
#define CAL_PI_GAIN_GEO_PGA_X1000 1000L
#endif

/* Delta_PGA del firmware actual. La corrida experimental documentada en el
 * manuscrito uso deliberadamente Delta_PGA=18. */
#ifndef CAL_PI_DEADBAND_GEO_PGA_DAC_CODES
#define CAL_PI_DEADBAND_GEO_PGA_DAC_CODES 3L
#endif

/* P=1e-3, I=300e-6. */
#define CAL_PI_KP_NUM_GEO_PGA 1L
#define CAL_PI_KP_DIV_GEO_PGA 1000L
#define CAL_PI_KI_NUM_GEO_PGA 3L
#define CAL_PI_KI_DIV_GEO_PGA 10000L

#ifndef CAL_PI_LOCK_SAMPLES_GEO_PGA
#define CAL_PI_LOCK_SAMPLES_GEO_PGA 512u
#endif

#ifndef CAL_PI_SETTLE_SAMPLES_GEO_PGA
#define CAL_PI_SETTLE_SAMPLES_GEO_PGA 512u
#endif

#ifndef CAL_PI_TIMEOUT_SAMPLES_GEO_PGA
#define CAL_PI_TIMEOUT_SAMPLES_GEO_PGA CAL_PI_DEFAULT_TIMEOUT_SAMPLES
#endif

#ifndef CAL_PI_REFINE_ENABLE_GEO_PGA
#define CAL_PI_REFINE_ENABLE_GEO_PGA 1u
#endif

#ifndef CAL_PI_REFINE_SETTLE_SAMPLES_GEO_PGA
#define CAL_PI_REFINE_SETTLE_SAMPLES_GEO_PGA 1024u
#endif

#endif
