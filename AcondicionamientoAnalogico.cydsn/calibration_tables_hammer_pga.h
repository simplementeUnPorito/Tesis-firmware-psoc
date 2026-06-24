#ifndef CALIBRATION_TABLES_HAMMER_PGA_H
#define CALIBRATION_TABLES_HAMMER_PGA_H

/* Parametros de calibracion para la etapa HAMMER_PGA (VDAC_PGA, AMux_ADC=0).
 * Mismo formato/orden que calibration_tables_geo_pga.h (etapas GEO) -- todo
 * aca es independiente de HAMMER_LP, para poder afinarlos por separado.
 *
 * Este archivo esta dividido en dos bloques que NO se mezclan:
 *   1) PI -- el UNICO algoritmo activo hoy (cal_pi_run_service,
 *      calibration.c). Puerto directo de Subsystem_step() en
 *      src/matlab/Simulaciones Controladores/Desacople/Subsystem_grt_rtw/
 *      Subsystem.c -- ver el banner en calibration_tables.h para la ley
 *      completa. Lee el DC ya extraido por el FIR de hardware (Filter,
 *      FIR_calibration.h) -- nunca promedia en software.
 *   2) BISECCION -- legado, desactivado (CAL_ALGO_BISECTION_ENABLE=0 en
 *      calibration.h). Se conserva tal cual a pedido del usuario, pero estos
 *      campos NO los toca ni los usa el PI. Si algun dia se reactiva la
 *      bisección, son estos los que hay que mirar/afinar -- hasta entonces,
 *      ignorarlos. */

/* ============================================================
 * PI (unico algoritmo activo) -- superficie de ajuste completa:
 * target_mv, adelanto_mv, direction, max_change, Kp, Ki, lock_samples.
 * La deadband se deriva en runtime desde la ganancia efectiva del VDAC.
 * ============================================================ */

#ifndef CAL_TARGET_HAMMER_PGA_MV
#define CAL_TARGET_HAMMER_PGA_MV 1024L   /* Opa_ref_1V medido por ADC */
#endif
#define CAL_TARGET_HAMMER_PGA_COUNTS (CAL_TARGET_HAMMER_PGA_MV * CAL_TARGET_1V_COUNTS / 1000L)

/* Signo del lazo VDAC->medida; si la etapa diverge en vez de converger,
 * este es el primer sospechoso (invertir el signo). */
#define CAL_DIRECTION_HAMMER_PGA (-1)

/* Candidato INICIAL en mV (no el target) donde arranca el PI -- hace de
 * feedforward fijo (dac_center). Hardcodear lo que la prueba en banco diga
 * que converge mas rapido; no tiene que ser una fraccion exacta del target. */
#ifndef CAL_ADELANTO_HAMMER_PGA_MV
#define CAL_ADELANTO_HAMMER_PGA_MV 1040L  /* 65 codigos VDAC8 * 16mV */
#endif
#define CAL_DAC_CENTER_HAMMER_PGA ((uint8)(CAL_ADELANTO_HAMMER_PGA_MV / CAL_VDAC8_MV_PER_LSB))

/* Semiancho de busqueda alrededor del adelanto, en codigos VDAC8 (255 = no acotar). */
#define CAL_DAC_MAX_CHANGE_HAMMER_PGA 255u

/* La deadband de HAMMER_PGA se calcula con ganancia dinamica:
 * abs(PGA_GAIN - 1) porque el VDAC entra por el terminal negativo. */
#define CAL_PI_GAIN_HAMMER_PGA_X1000 0L

/* Kp/Ki trabajan sobre error reescalado a codigos DAC y el controlador divide
 * por la ganancia efectiva de la etapa. */
#define CAL_PI_KP_NUM_HAMMER_PGA 1L
#define CAL_PI_KP_DIV_HAMMER_PGA 64L
#define CAL_PI_KI_NUM_HAMMER_PGA 1L
#define CAL_PI_KI_DIV_HAMMER_PGA 2048L

/* M muestras consecutivas dentro de la misma celda de error cuantizado. */
#ifndef CAL_PI_LOCK_SAMPLES_HAMMER_PGA
#define CAL_PI_LOCK_SAMPLES_HAMMER_PGA 128u
#endif

/* El PI no descarta muestras; el FIR de calibracion entrega la medicion DC. */
#define CAL_SETTLE_SAMPLES_HAMMER_PGA 0u

/* ============================================================
 * BISECCION (legado, desactivado -- CAL_ALGO_BISECTION_ENABLE=0). El PI NO
 * lee nada de aca abajo.
 * ============================================================ */
#define CAL_PROBE_STEP_HAMMER_PGA 32u
#define CAL_MAX_ITER_HAMMER_PGA   16u
#define CAL_TOL_COUNTS_HAMMER_PGA      6000L
#define CAL_DEADBAND_COUNTS_HAMMER_PGA CAL_TOL_COUNTS_HAMMER_PGA
#define CAL_VERIFY_SETTLE_SAMPLES_HAMMER_PGA 900u
#define CAL_SAT_COUNTS_HAMMER_PGA 120000L

#define CAL_SERVO_KP_NUM_HAMMER_PGA        1L
#define CAL_SERVO_KI_NUM_HAMMER_PGA        1L
#define CAL_SERVO_KI_DIV_HAMMER_PGA        8L
#define CAL_SERVO_DEADBAND_HAMMER_PGA      5000L
#define CAL_SERVO_FINE_STEP_HAMMER_PGA     1u
#define CAL_SERVO_RECOVERY_STEP_HAMMER_PGA 1u
#define CAL_SERVO_SETTLE_SAMPLES_HAMMER_PGA 600u

#endif
