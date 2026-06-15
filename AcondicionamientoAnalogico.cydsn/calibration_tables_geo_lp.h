#ifndef CALIBRATION_TABLES_GEO_LP_H
#define CALIBRATION_TABLES_GEO_LP_H

/* Parametros de calibracion para la etapa GEO_LP (VDAC_ref_LP, AMux_ADC=3,
 * tambien el canal de captura final). Todo aca es independiente de las demas
 * etapas (ver HANDOFF_CALIBRATION.md, seccion "headers por VDAC" para la guia
 * de ajuste rapido vs. final). */

/* ===== Identidad / objetivo =====
 * ADC_CFG1 diferencial signed 18 bit, 0 counts = 0V diferencial (señal del
 * geofono en reposo). direction=1: valor inicial del sentido
 * de busqueda (sube el codigo VDAC -> sube la medida); EVAL_PROBE mide el
 * signo real al arrancar cada corrida y lo guarda para la fase realcheck. */
#define CAL_TARGET_COUNTS_GEO_LP   0L
#define CAL_DIRECTION_GEO_LP       1

/* ===== Rango de busqueda (biseccion) =====
 * dac_center/max_change acotan el rango [center-max_change, center+max_change].
 * probe_step es el primer paso (antes de la biseccion) para detectar el signo
 * real de la pendiente. max_iter limita las iteraciones de biseccion.
 * tolerance_counts: 0.1V aprox = 52429 counts/V * 0.1V = 5243 counts. */
#define CAL_DAC_CENTER_GEO_LP      0x9Cu
#define CAL_DAC_MAX_CHANGE_GEO_LP  64u
#define CAL_PROBE_STEP_GEO_LP      32u
#define CAL_MAX_ITER_GEO_LP        16u
#define CAL_TOL_COUNTS_GEO_LP      5243L

/* deadband_counts: igual a tolerance_counts -> CAL_ASYNC_PLAN_ITER corta la
 * busqueda en cuanto el punto actual (no solo el mejor historico) cae dentro
 * de tolerancia, evitando oscilar cerca del objetivo (histeresis). */
#define CAL_DEADBAND_COUNTS_GEO_LP 5243L

/* ===== Asentamiento: muestras descartadas tras escribir el VDAC, antes de
 * empezar a promediar (a ~3 kSPS). No cambia con el rediseño de promediado. */
#define CAL_SETTLE_SAMPLES_GEO_LP        6000u
#define CAL_VERIFY_SETTLE_SAMPLES_GEO_LP 9000u

/* ===== Promediado/convergencia: biseccion =====
 * measured = promedio de las ultimas avg_n*window_count muestras (peso
 * constante, no se diluye). "Estable" = ese promedio varia
 * <= settle_tol_counts durante stable_streak comparaciones consecutivas, una
 * vez el buffer lleno (piso = avg_n*window_count muestras). Si se llega a
 * max_samples sin racha estable, se usa el promedio igual.
 *
 * NOTA GEO_LP: esta es la etapa con saltos sub-LSB mas grandes (16mV/LSB del
 * VDAC sobre la ultima ganancia de la cascada). La biseccion puede terminar
 * "atrapada" entre dos codigos consecutivos sin poder llegar exactamente a
 * tolerance_counts; eso es esperado y lo resuelve la fase realcheck (mide la
 * señal real del geofono y hace el ajuste fino de +-1 LSB). */
#define CAL_AVG_N_GEO_LP              64u
#define CAL_AVG_WINDOW_COUNT_GEO_LP   16u   /* piso 1024 muestras (~341ms) */
#define CAL_AVG_MAX_SAMPLES_GEO_LP    2048u /* techo ~683ms */
#define CAL_AVG_SETTLE_TOL_GEO_LP     10L
#define CAL_AVG_STABLE_STREAK_GEO_LP  3u

/* ===== Promediado/convergencia: verify =====
 * Mismo avg_n/window_count/settle_tol/stable_streak que biseccion, solo
 * cambia el techo (mas tiempo para confirmar el resultado final). */
#define CAL_VERIFY_AVG_MAX_SAMPLES_GEO_LP 4096u /* ~1.4s */

/* ===== Anti-saturacion =====
 * Umbral de riel real (distinto de CAL_OPERATING_RANGE_COUNTS, que es la
 * "banda buena" +-0.5V). Full-scale ADC_CFG1 = +-131072 counts aprox; 120000
 * = ~91% FS = ~2.29V. Ajustar mirando PSOC_EVT_CAL_STAGE_MEAS32 (sin clamp):
 * si el valor queda fijo en ~131072 sin importar el DAC, esta saturado de
 * verdad; si varia aunque sea >26214, solo esta "fuera de banda". */
#define CAL_SAT_COUNTS_GEO_LP   120000L

/* ===== Etapa final con señal real (entrada fija al geofono) =====
 * Ajuste fino de +-nudge_step LSB sobre el resultado de verify, hasta
 * max_nudges veces; revierte si un nudge satura o empeora |error|.
 * enable=0 salta la etapa (queda el resultado de verify tal cual). Esta es la
 * etapa donde el ajuste de +-1 LSB se nota mas (ultima de la cascada y la de
 * mayor salto sub-LSB). */
#define CAL_REALCHECK_ENABLE_GEO_LP            1u
#define CAL_REALCHECK_TOL_COUNTS_GEO_LP        5243L
#define CAL_REALCHECK_NUDGE_STEP_GEO_LP        1u
#define CAL_REALCHECK_MAX_NUDGES_GEO_LP        3u
#define CAL_REALCHECK_DISCARD_SAMPLES_GEO_LP       1500u /* settle al cambiar AMux_ADC */
#define CAL_REALCHECK_NUDGE_DISCARD_SAMPLES_GEO_LP  200u /* settle tras un nudge de +-1 LSB */
#define CAL_REALCHECK_AVG_N_GEO_LP              64u
#define CAL_REALCHECK_AVG_WINDOW_COUNT_GEO_LP   64u   /* piso 4096 muestras (~1.4s) */
#define CAL_REALCHECK_AVG_MAX_SAMPLES_GEO_LP    8192u /* techo ~2.7s */
#define CAL_REALCHECK_AVG_SETTLE_TOL_GEO_LP     10L
#define CAL_REALCHECK_AVG_STABLE_STREAK_GEO_LP  3u

/* ===== Servo PI (mantenimiento en IDLE, CAL_SERVO_ENABLE_DEFAULT=0) =====
 * control = err*KP_NUM + integral*KI_NUM/KI_DIV; el signo decide el siguiente
 * LSB. Reubicado aca para ajustarlo sin tocar calibration_tables.h. */
#define CAL_SERVO_KP_NUM_GEO_LP         1L
#define CAL_SERVO_KI_NUM_GEO_LP         1L
#define CAL_SERVO_KI_DIV_GEO_LP         8L
#define CAL_SERVO_DEADBAND_GEO_LP       5000L
#define CAL_SERVO_FINE_STEP_GEO_LP      1u
#define CAL_SERVO_RECOVERY_STEP_GEO_LP  1u
#define CAL_SERVO_SETTLE_SAMPLES_GEO_LP 3000u

#endif
