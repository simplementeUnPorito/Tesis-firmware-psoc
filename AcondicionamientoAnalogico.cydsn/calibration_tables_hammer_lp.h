#ifndef CALIBRATION_TABLES_HAMMER_LP_H
#define CALIBRATION_TABLES_HAMMER_LP_H

/* Parametros de calibracion para la etapa HAMMER_LP (VDAC_LP, AMux_ADC=1).
 * Ver calibration_tables_hammer_pga.h para la guia completa (misma forma,
 * misma separacion PI/BISECCION) -- independiente de HAMMER_PGA para poder
 * afinarlos por separado. */

/* ============================================================
 * PI (unico algoritmo activo) -- superficie de ajuste completa:
 * target_mv, adelanto_mv, direction, max_change, ganancia, Kp, Ki,
 * lock_samples. La deadband se deriva de la ganancia fija de esta etapa.
 * ============================================================ */

#ifndef CAL_TARGET_HAMMER_LP_MV
#define CAL_TARGET_HAMMER_LP_MV 2800L
#endif
#define CAL_TARGET_HAMMER_LP_COUNTS (CAL_TARGET_HAMMER_LP_MV * CAL_TARGET_1V_COUNTS / 1000L)

#define CAL_DIRECTION_HAMMER_LP (1)

#ifndef CAL_ADELANTO_HAMMER_LP_MV
#define CAL_ADELANTO_HAMMER_LP_MV 912L  /* 57 codigos VDAC8 * 16mV */
#endif
#define CAL_DAC_CENTER_HAMMER_LP ((uint8)(CAL_ADELANTO_HAMMER_LP_MV / CAL_VDAC8_MV_PER_LSB))

#define CAL_DAC_MAX_CHANGE_HAMMER_LP 255u

/* Ganancia absoluta fija VDAC->medida para la etapa LP. Sobrescribir desde
 * tabla/frontend cuando se tenga la ganancia de hardware medida. */
#ifndef CAL_PI_GAIN_HAMMER_LP_X1000
#define CAL_PI_GAIN_HAMMER_LP_X1000 1000L
#endif

#define CAL_PI_KP_NUM_HAMMER_LP 1L
#define CAL_PI_KP_DIV_HAMMER_LP 64L
#define CAL_PI_KI_NUM_HAMMER_LP 1L
#define CAL_PI_KI_DIV_HAMMER_LP 2048L

#ifndef CAL_PI_LOCK_SAMPLES_HAMMER_LP
#define CAL_PI_LOCK_SAMPLES_HAMMER_LP 128u
#endif

#define CAL_SETTLE_SAMPLES_HAMMER_LP 0u

/* ============================================================
 * BISECCION (legado, desactivado -- CAL_ALGO_BISECTION_ENABLE=0). El PI NO
 * lee nada de aca abajo.
 * ============================================================ */
#define CAL_PROBE_STEP_HAMMER_LP 32u
#define CAL_MAX_ITER_HAMMER_LP   16u
#define CAL_TOL_COUNTS_HAMMER_LP      6000L
#define CAL_DEADBAND_COUNTS_HAMMER_LP CAL_TOL_COUNTS_HAMMER_LP
#define CAL_VERIFY_SETTLE_SAMPLES_HAMMER_LP 1200u
#define CAL_SAT_COUNTS_HAMMER_LP 120000L

#define CAL_SERVO_KP_NUM_HAMMER_LP        1L
#define CAL_SERVO_KI_NUM_HAMMER_LP        1L
#define CAL_SERVO_KI_DIV_HAMMER_LP        8L
#define CAL_SERVO_DEADBAND_HAMMER_LP      5000L
#define CAL_SERVO_FINE_STEP_HAMMER_LP     1u
#define CAL_SERVO_RECOVERY_STEP_HAMMER_LP 1u
#define CAL_SERVO_SETTLE_SAMPLES_HAMMER_LP 900u

#endif
