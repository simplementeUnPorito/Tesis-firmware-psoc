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

/* Kp y Ki de la simulacion Monte Carlo del lazo
 * (calculos_modelados/python/calibracion_pi). 500 corridas por punto sobre
 * 120 pares Kp-Ki, con la planta medida de esta placa, el offset de entrada
 * de los operacionales sorteado en +-2 mV (maximo del datasheet del
 * CY8C58LP) y deriva termica. Verificado ademas contra la constante de
 * tiempo del filtro, que es la suposicion mas debil del modelo: converge el
 * 100 % de las corridas con tau entre 10 y 300 muestras, y es la sintonia
 * mas rapida en el peor tau (mediana 577-648 muestras). */
#define CAL_PI_KP_NUM_HAMMER_PGA 4L
#define CAL_PI_KP_DIV_HAMMER_PGA 1L
#define CAL_PI_KI_NUM_HAMMER_PGA 1L
#define CAL_PI_KI_DIV_HAMMER_PGA 2L

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
