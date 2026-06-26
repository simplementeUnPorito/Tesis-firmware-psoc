#ifndef CALIBRATION_TABLES_GEO_PGA_H
#define CALIBRATION_TABLES_GEO_PGA_H

/* Parametros de calibracion para la etapa GEO_PGA (VDAC_ref_PGA, AMux_ADC=0).
 * Todo aca es independiente de las demas etapas.
 *
 * Este archivo esta dividido en dos bloques que NO se mezclan:
 *   1) PI -- el UNICO algoritmo activo hoy (cal_pi_run_service,
 *      calibration.c). Puerto directo de Subsystem_step() en
 *      src/matlab/Simulaciones Controladores/Desacople/Subsystem_grt_rtw/
 *      Subsystem.c -- ver el banner en calibration_tables.h para la ley
 *      completa.
 *   2) BISECCION -- legado, desactivado (CAL_ALGO_BISECTION_ENABLE=0 en
 *      calibration.h). Se conserva tal cual a pedido del usuario; el PI no
 *      lee nada de ese bloque. */

/* ============================================================
 * PI (unico algoritmo activo) -- superficie de ajuste completa:
 * target_mv, adelanto_mv, direction, max_change, deadband_mv, Kp, Ki,
 * lock_samples. NADA MAS aca abajo es un parametro del PI.
 * ============================================================ */

/* ADC_CFG1 diferencial signed 18 bit, 0 counts = 0V diferencial (señal del
 * geofono en reposo). Voltajes de este bloque se cargan en mV (nunca en
 * hex/codigo VDAC) -- las constantes _COUNTS/_CENTER se derivan solas. */
#ifndef CAL_TARGET_GEO_PGA_MV
#define CAL_TARGET_GEO_PGA_MV 0L   /* reposo diferencial GEO = 0V */
#endif
#define CAL_TARGET_COUNTS_GEO_PGA (CAL_TARGET_GEO_PGA_MV * CAL_TARGET_1V_COUNTS / 1000L)

/* La ganancia fisica VDAC->PGA es la misma que HAMMER (1 - GainDirecta),
 * pero la polaridad diferencial del ADC GEO queda invertida respecto de ese
 * nodo: el controlador debe aplicar direction=-1 para que el error tienda a
 * 0 en cuentas diferenciales. */
#define CAL_DIRECTION_GEO_PGA (-1)

#ifndef CAL_VDAC8_MV_PER_LSB
#define CAL_VDAC8_MV_PER_LSB 16L   /* VDAC8 1x: 4.08V/256 codigos ~= 16mV/LSB */
#endif

/* Candidato INICIAL en mV donde arranca el PI (feedforward/dac_center) --
 * el valor de arranque se deja comun a las etapas GEO: ~1.5V. */
#ifndef CAL_ADELANTO_GEO_PGA_MV
#define CAL_ADELANTO_GEO_PGA_MV 1504L
#endif
#define CAL_DAC_CENTER_GEO_PGA ((uint8)(CAL_ADELANTO_GEO_PGA_MV / CAL_VDAC8_MV_PER_LSB))

/* Misma ventana que HAMMER_PGA: el PI arranca en adelanto, pero puede usar
 * todo el VDAC si el offset de la placa GEO lo exige. */
#define CAL_DAC_MAX_CHANGE_GEO_PGA 255u

/* Ganancia VDAC->medida dinamica: en calibration.c stage 0 usa
 * 1 - GainDirecta, con signo fisico incluido, igual que HAMMER_PGA. */
#ifndef CAL_PI_GAIN_GEO_PGA_X1000
#define CAL_PI_GAIN_GEO_PGA_X1000 0L
#endif

/* Kp=1e-3, Ki=300e-6 sobre error reescalado a codigos DAC. */
#define CAL_PI_KP_NUM_GEO_PGA 1L
#define CAL_PI_KP_DIV_GEO_PGA 1000L
#define CAL_PI_KI_NUM_GEO_PGA 3L
#define CAL_PI_KI_DIV_GEO_PGA 10000L

/* N de la ventana del detector de lock -- ver banner de calibration_tables.h
 * (mismo concepto que el N=128 del modelo de referencia). */
#ifndef CAL_PI_LOCK_SAMPLES_GEO_PGA
#define CAL_PI_LOCK_SAMPLES_GEO_PGA 512u
#endif

/* El PI no descarta muestras; el FIR de calibracion entrega la medicion DC. */
#define CAL_SETTLE_SAMPLES_GEO_PGA 0u

/* ============================================================
 * BISECCION (legado, desactivado -- CAL_ALGO_BISECTION_ENABLE=0). El PI NO
 * lee nada de aca abajo.
 * ============================================================ */
#define CAL_PROBE_STEP_GEO_PGA 32u
#define CAL_MAX_ITER_GEO_PGA   16u

#ifndef CAL_TOL_MV_GEO_PGA
#define CAL_TOL_MV_GEO_PGA 100L
#endif
#define CAL_TOL_COUNTS_GEO_PGA (CAL_TOL_MV_GEO_PGA * CAL_TARGET_1V_COUNTS / 1000L)
#define CAL_DEADBAND_COUNTS_GEO_PGA CAL_TOL_COUNTS_GEO_PGA

#define CAL_VERIFY_SETTLE_SAMPLES_GEO_PGA CAL_SETTLE_SAMPLES_FROM_MS(220u)

#define CAL_AVG_N_GEO_PGA              64u
#define CAL_AVG_WINDOW_COUNT_GEO_PGA   16u
#define CAL_AVG_MAX_SAMPLES_GEO_PGA    2048u
#define CAL_AVG_SETTLE_TOL_GEO_PGA     10L
#define CAL_AVG_STABLE_STREAK_GEO_PGA  3u
#define CAL_VERIFY_AVG_MAX_SAMPLES_GEO_PGA 4096u

#define CAL_SAT_COUNTS_GEO_PGA 120000L

#define CAL_REALCHECK_ENABLE_GEO_PGA            1u
#define CAL_REALCHECK_TOL_COUNTS_GEO_PGA        5243L
#define CAL_REALCHECK_NUDGE_STEP_GEO_PGA        1u
#define CAL_REALCHECK_MAX_NUDGES_GEO_PGA        3u
#define CAL_REALCHECK_DISCARD_SAMPLES_GEO_PGA       CAL_SETTLE_SAMPLES_FROM_MS(220u)
#define CAL_REALCHECK_NUDGE_DISCARD_SAMPLES_GEO_PGA CAL_SETTLE_SAMPLES_FROM_MS(220u)
#define CAL_REALCHECK_AVG_N_GEO_PGA              64u
#define CAL_REALCHECK_AVG_WINDOW_COUNT_GEO_PGA   64u
#define CAL_REALCHECK_AVG_MAX_SAMPLES_GEO_PGA    8192u
#define CAL_REALCHECK_AVG_SETTLE_TOL_GEO_PGA     10L
#define CAL_REALCHECK_AVG_STABLE_STREAK_GEO_PGA  3u

#define CAL_SERVO_KP_NUM_GEO_PGA         1L
#define CAL_SERVO_KI_NUM_GEO_PGA         1L
#define CAL_SERVO_KI_DIV_GEO_PGA         8L
#define CAL_SERVO_DEADBAND_GEO_PGA       5000L
#define CAL_SERVO_FINE_STEP_GEO_PGA      1u
#define CAL_SERVO_RECOVERY_STEP_GEO_PGA  1u
#define CAL_SERVO_SETTLE_SAMPLES_GEO_PGA CAL_SETTLE_SAMPLES_FROM_MS(220u)

#endif
