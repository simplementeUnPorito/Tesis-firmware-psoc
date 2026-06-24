#ifndef CALIBRATION_TABLES_GEO_ADDER_H
#define CALIBRATION_TABLES_GEO_ADDER_H

/* Parametros de calibracion para la etapa GEO_ADDER (VDAC_Ref_Adder,
 * AMux_ADC=2). Todo aca es independiente de las demas etapas.
 *
 * Este archivo esta dividido en dos bloques que NO se mezclan: PI (unico
 * algoritmo activo) y BISECCION (legado, desactivado -- ver
 * calibration_tables_geo_pga.h para la explicacion completa de la
 * separacion). */

/* ============================================================
 * PI (unico algoritmo activo) -- superficie de ajuste completa:
 * target_mv, adelanto_mv, direction, max_change, deadband_mv, Kp, Ki,
 * lock_samples. NADA MAS aca abajo es un parametro del PI.
 * ============================================================ */

#ifndef CAL_TARGET_GEO_ADDER_MV
#define CAL_TARGET_GEO_ADDER_MV 0L   /* reposo diferencial GEO = 0V */
#endif
#define CAL_TARGET_COUNTS_GEO_ADDER (CAL_TARGET_GEO_ADDER_MV * CAL_TARGET_1V_COUNTS / 1000L)

#define CAL_DIRECTION_GEO_ADDER 1

#ifndef CAL_VDAC8_MV_PER_LSB
#define CAL_VDAC8_MV_PER_LSB 16L
#endif

#ifndef CAL_ADELANTO_GEO_ADDER_MV
#define CAL_ADELANTO_GEO_ADDER_MV 2496L
#endif
#define CAL_DAC_CENTER_GEO_ADDER ((uint8)(CAL_ADELANTO_GEO_ADDER_MV / CAL_VDAC8_MV_PER_LSB))

#define CAL_DAC_MAX_CHANGE_GEO_ADDER 64u

#ifndef CAL_PI_GAIN_GEO_ADDER_X1000
#define CAL_PI_GAIN_GEO_ADDER_X1000 1000L
#endif

#define CAL_PI_KP_NUM_GEO_ADDER 1L
#define CAL_PI_KP_DIV_GEO_ADDER 32L
#define CAL_PI_KI_NUM_GEO_ADDER 1L
#define CAL_PI_KI_DIV_GEO_ADDER 8192L

#ifndef CAL_PI_LOCK_SAMPLES_GEO_ADDER
#define CAL_PI_LOCK_SAMPLES_GEO_ADDER 128u
#endif

/* Asentamiento: tiempo simulado 200us, convertido a muestras. */
#define CAL_SETTLE_SAMPLES_GEO_ADDER CAL_SETTLE_SAMPLES_FROM_US(200u)

/* ============================================================
 * BISECCION (legado, desactivado -- CAL_ALGO_BISECTION_ENABLE=0). El PI NO
 * lee nada de aca abajo.
 * ============================================================ */
#define CAL_PROBE_STEP_GEO_ADDER 32u
#define CAL_MAX_ITER_GEO_ADDER   16u

#ifndef CAL_TOL_MV_GEO_ADDER
#define CAL_TOL_MV_GEO_ADDER 100L
#endif
#define CAL_TOL_COUNTS_GEO_ADDER (CAL_TOL_MV_GEO_ADDER * CAL_TARGET_1V_COUNTS / 1000L)
#define CAL_DEADBAND_COUNTS_GEO_ADDER CAL_TOL_COUNTS_GEO_ADDER

#define CAL_VERIFY_SETTLE_SAMPLES_GEO_ADDER CAL_SETTLE_SAMPLES_FROM_US(200u)

#define CAL_AVG_N_GEO_ADDER              64u
#define CAL_AVG_WINDOW_COUNT_GEO_ADDER   16u
#define CAL_AVG_MAX_SAMPLES_GEO_ADDER    2048u
#define CAL_AVG_SETTLE_TOL_GEO_ADDER     10L
#define CAL_AVG_STABLE_STREAK_GEO_ADDER  3u
#define CAL_VERIFY_AVG_MAX_SAMPLES_GEO_ADDER 4096u

#define CAL_SAT_COUNTS_GEO_ADDER 120000L

#define CAL_REALCHECK_ENABLE_GEO_ADDER            1u
#define CAL_REALCHECK_TOL_COUNTS_GEO_ADDER        5243L
#define CAL_REALCHECK_NUDGE_STEP_GEO_ADDER        1u
#define CAL_REALCHECK_MAX_NUDGES_GEO_ADDER        3u
#define CAL_REALCHECK_DISCARD_SAMPLES_GEO_ADDER       CAL_SETTLE_SAMPLES_FROM_US(200u)
#define CAL_REALCHECK_NUDGE_DISCARD_SAMPLES_GEO_ADDER CAL_SETTLE_SAMPLES_FROM_US(200u)
#define CAL_REALCHECK_AVG_N_GEO_ADDER              64u
#define CAL_REALCHECK_AVG_WINDOW_COUNT_GEO_ADDER   64u
#define CAL_REALCHECK_AVG_MAX_SAMPLES_GEO_ADDER    8192u
#define CAL_REALCHECK_AVG_SETTLE_TOL_GEO_ADDER     10L
#define CAL_REALCHECK_AVG_STABLE_STREAK_GEO_ADDER  3u

/* DEADBAND del servo mas chico que las otras etapas (valor original heredado). */
#define CAL_SERVO_KP_NUM_GEO_ADDER         1L
#define CAL_SERVO_KI_NUM_GEO_ADDER         1L
#define CAL_SERVO_KI_DIV_GEO_ADDER         8L
#define CAL_SERVO_DEADBAND_GEO_ADDER       2000L
#define CAL_SERVO_FINE_STEP_GEO_ADDER      1u
#define CAL_SERVO_RECOVERY_STEP_GEO_ADDER  1u
#define CAL_SERVO_SETTLE_SAMPLES_GEO_ADDER CAL_SETTLE_SAMPLES_FROM_US(200u)

#endif
