#ifndef CALIBRATION_TABLES_GEO_BP_H
#define CALIBRATION_TABLES_GEO_BP_H

/* Parametros de calibracion para la etapa GEO_BP (VDAC_ref_BP, AMux_ADC=1).
 * Todo aca es independiente de las demas etapas.
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

#ifndef CAL_TARGET_GEO_BP_MV
#define CAL_TARGET_GEO_BP_MV 0L   /* reposo diferencial GEO = 0V */
#endif
#define CAL_TARGET_COUNTS_GEO_BP (CAL_TARGET_GEO_BP_MV * CAL_TARGET_1V_COUNTS / 1000L)

/* La etapa entra por positivo: VDAC_ref_BP aumenta la medicion con ganancia
 * fisica +1 respecto al nodo diferencial que se calibra. */
#define CAL_DIRECTION_GEO_BP 1

#ifndef CAL_VDAC8_MV_PER_LSB
#define CAL_VDAC8_MV_PER_LSB 16L
#endif

#ifndef CAL_ADELANTO_GEO_BP_MV
#define CAL_ADELANTO_GEO_BP_MV 1504L
#endif
#ifndef CAL_DAC_CENTER_GEO_BP
#define CAL_DAC_CENTER_GEO_BP ((uint8)(CAL_ADELANTO_GEO_BP_MV / CAL_VDAC8_MV_PER_LSB))
#endif

/* Copiado del perfil LP probado: adelanto como arranque, rango completo. */
#define CAL_DAC_MAX_CHANGE_GEO_BP 255u

#ifndef CAL_PI_GAIN_GEO_BP_X1000
#define CAL_PI_GAIN_GEO_BP_X1000 1000L
#endif

#define CAL_PI_KP_NUM_GEO_BP 1L
#define CAL_PI_KP_DIV_GEO_BP 10000L
#define CAL_PI_KI_NUM_GEO_BP 1L
#define CAL_PI_KI_DIV_GEO_BP 2000L

#ifndef CAL_PI_LOCK_SAMPLES_GEO_BP
#define CAL_PI_LOCK_SAMPLES_GEO_BP 512u
#endif

/* El PI no descarta muestras; el FIR de calibracion entrega la medicion DC. */
#define CAL_SETTLE_SAMPLES_GEO_BP 0u

/* ============================================================
 * BISECCION (legado, desactivado -- CAL_ALGO_BISECTION_ENABLE=0). El PI NO
 * lee nada de aca abajo.
 * ============================================================ */
#define CAL_PROBE_STEP_GEO_BP 32u
#define CAL_MAX_ITER_GEO_BP   16u

#ifndef CAL_TOL_MV_GEO_BP
#define CAL_TOL_MV_GEO_BP 100L
#endif
#define CAL_TOL_COUNTS_GEO_BP (CAL_TOL_MV_GEO_BP * CAL_TARGET_1V_COUNTS / 1000L)
#define CAL_DEADBAND_COUNTS_GEO_BP CAL_TOL_COUNTS_GEO_BP

#define CAL_VERIFY_SETTLE_SAMPLES_GEO_BP CAL_SETTLE_SAMPLES_FROM_US(100u)

#define CAL_AVG_N_GEO_BP              64u
#define CAL_AVG_WINDOW_COUNT_GEO_BP   16u
#define CAL_AVG_MAX_SAMPLES_GEO_BP    2048u
#define CAL_AVG_SETTLE_TOL_GEO_BP     10L
#define CAL_AVG_STABLE_STREAK_GEO_BP  3u
#define CAL_VERIFY_AVG_MAX_SAMPLES_GEO_BP 4096u

#define CAL_SAT_COUNTS_GEO_BP 120000L

#define CAL_REALCHECK_ENABLE_GEO_BP            1u
#define CAL_REALCHECK_TOL_COUNTS_GEO_BP        5243L
#define CAL_REALCHECK_NUDGE_STEP_GEO_BP        1u
#define CAL_REALCHECK_MAX_NUDGES_GEO_BP        3u
#define CAL_REALCHECK_DISCARD_SAMPLES_GEO_BP       CAL_SETTLE_SAMPLES_FROM_US(100u)
#define CAL_REALCHECK_NUDGE_DISCARD_SAMPLES_GEO_BP CAL_SETTLE_SAMPLES_FROM_US(100u)
#define CAL_REALCHECK_AVG_N_GEO_BP              64u
#define CAL_REALCHECK_AVG_WINDOW_COUNT_GEO_BP   64u
#define CAL_REALCHECK_AVG_MAX_SAMPLES_GEO_BP    8192u
#define CAL_REALCHECK_AVG_SETTLE_TOL_GEO_BP     10L
#define CAL_REALCHECK_AVG_STABLE_STREAK_GEO_BP  3u

#define CAL_SERVO_KP_NUM_GEO_BP         1L
#define CAL_SERVO_KI_NUM_GEO_BP         1L
#define CAL_SERVO_KI_DIV_GEO_BP         8L
#define CAL_SERVO_DEADBAND_GEO_BP       5000L
#define CAL_SERVO_FINE_STEP_GEO_BP      1u
#define CAL_SERVO_RECOVERY_STEP_GEO_BP  1u
#define CAL_SERVO_SETTLE_SAMPLES_GEO_BP CAL_SETTLE_SAMPLES_FROM_US(100u)

/* ===== MODO SKIP (bypass de busqueda binaria, BISECCION solamente) =====
 * Para deshabilitar la calibracion de GEO_BP y dejar VDAC_ref_BP fijo:
 *   En PSoC Creator: Project > Build Settings > Compiler > Preprocessor Definitions
 *   Agregar:  CAL_GEO_BP_SKIP=1u
 *   Agregar:  CAL_DAC_CENTER_GEO_BP=0x94u  (o el codigo que funcione, default 0x9C)
 * Con SKIP activo: la etapa escribe el valor fijo, mide una vez (tolerancia
 * maxima = siempre pasa), y reporta ok=1 sin ejecutar biseccion ni realcheck.
 * El VDAC_ref_BP sigue siendo iniciado por psoc_calibration_start_references(). */
#ifndef CAL_GEO_BP_SKIP
#define CAL_GEO_BP_SKIP 0u
#endif

#if CAL_GEO_BP_SKIP
  #undef  CAL_DAC_MAX_CHANGE_GEO_BP
  #define CAL_DAC_MAX_CHANGE_GEO_BP   0u
  #undef  CAL_PROBE_STEP_GEO_BP
  #define CAL_PROBE_STEP_GEO_BP       1u
  #undef  CAL_MAX_ITER_GEO_BP
  #define CAL_MAX_ITER_GEO_BP         1u
  #undef  CAL_TOL_COUNTS_GEO_BP
  #define CAL_TOL_COUNTS_GEO_BP       131071L
  #undef  CAL_DEADBAND_COUNTS_GEO_BP
  #define CAL_DEADBAND_COUNTS_GEO_BP  131071L
  #undef  CAL_REALCHECK_ENABLE_GEO_BP
  #define CAL_REALCHECK_ENABLE_GEO_BP 0u
  #undef  CAL_REALCHECK_MAX_NUDGES_GEO_BP
  #define CAL_REALCHECK_MAX_NUDGES_GEO_BP 0u
#endif

#endif
