#ifndef CALIBRATION_TABLES_HAMMER_PGA_H
#define CALIBRATION_TABLES_HAMMER_PGA_H

/* HAMMER_PGA: VDAC_PGA, AMux_ADC=0. */

#ifndef CAL_TARGET_HAMMER_PGA_MV
#define CAL_TARGET_HAMMER_PGA_MV 1024L
#endif
#define CAL_TARGET_HAMMER_PGA_COUNTS (CAL_TARGET_HAMMER_PGA_MV * CAL_TARGET_1V_COUNTS / 1000L)

/* La ganancia efectiva se calcula en runtime como 1 - GainDirecta. */
#define CAL_DIRECTION_HAMMER_PGA 1

#ifndef CAL_ADELANTO_HAMMER_PGA_MV
#define CAL_ADELANTO_HAMMER_PGA_MV 1040L
#endif
/* Arranca en 0, que con polaridad es exactamente Vref: el punto natural
 * para empezar a anular un offset, y el unico desde el que se puede ir
 * para los dos lados. Antes era un adelanto fijo en mV sobre el LSB del
 * VDAC, que no tiene sentido en un IDAC con signo. */
#define CAL_DAC_CENTER_HAMMER_PGA 0

#define CAL_DAC_MAX_CHANGE_HAMMER_PGA CAL_IDAC_SIGNED_MAX

/* 0 = ganancia dinamica 1 - GainDirecta. */
/* Ganancia fisica referencia -> tap, medida en la placa el 2026-09-02
 * con el barrido de D2, dividida por el escalon real de 1875 uV. */
#define CAL_PI_GAIN_HAMMER_PGA_X1000 31L

/* 0 conserva la seleccion automatica a partir de la ganancia fisica. */
#ifndef CAL_PI_DEADBAND_HAMMER_PGA_DAC_CODES
#define CAL_PI_DEADBAND_HAMMER_PGA_DAC_CODES 1L
#endif

/* P=1e-3, I=300e-6. */
#define CAL_PI_KP_NUM_HAMMER_PGA 1L
#define CAL_PI_KP_DIV_HAMMER_PGA 1000L
#define CAL_PI_KI_NUM_HAMMER_PGA 3L
#define CAL_PI_KI_DIV_HAMMER_PGA 10000L

#ifndef CAL_PI_LOCK_SAMPLES_HAMMER_PGA
#define CAL_PI_LOCK_SAMPLES_HAMMER_PGA 512u
#endif

#ifndef CAL_PI_SETTLE_SAMPLES_HAMMER_PGA
#define CAL_PI_SETTLE_SAMPLES_HAMMER_PGA 512u
#endif

#ifndef CAL_PI_TIMEOUT_SAMPLES_HAMMER_PGA
#define CAL_PI_TIMEOUT_SAMPLES_HAMMER_PGA CAL_PI_DEFAULT_TIMEOUT_SAMPLES
#endif

#ifndef CAL_PI_REFINE_ENABLE_HAMMER_PGA
#define CAL_PI_REFINE_ENABLE_HAMMER_PGA 1u
#endif

#ifndef CAL_PI_REFINE_SETTLE_SAMPLES_HAMMER_PGA
#define CAL_PI_REFINE_SETTLE_SAMPLES_HAMMER_PGA 1024u
#endif

#endif
