#ifndef CALIBRATION_TABLES_GEO_ADDER_H
#define CALIBRATION_TABLES_GEO_ADDER_H

/* Parametros de calibracion para la etapa GEO_ADDER (VDAC_Ref_Adder,
 * AMux_ADC=2). Todo aca es independiente de las demas etapas (ver
 * HANDOFF_CALIBRATION.md, seccion "headers por VDAC" para la guia de ajuste
 * rapido vs. final). */

/* Guia de ajuste de parametros:
 * CAL_TARGET_COUNTS_*: valor ADC objetivo; para reposo diferencial GEO es 0.
 * CAL_DIRECTION_*: sentido inicial esperado; el firmware lo verifica con el probe.
 * CAL_DAC_CENTER_*: codigo VDAC inicial y centro del rango de busqueda.
 * CAL_DAC_MAX_CHANGE_*: semiancho del rango; mas chico acelera si el centro es bueno.
 * CAL_PROBE_STEP_*: primer salto para detectar pendiente; mas grande detecta mejor,
 *   pero puede tocar zonas no lineales si el rango util es chico.
 * CAL_MAX_ITER_*: tope de biseccion; con rango +-64, 7-8 suele alcanzar.
 * CAL_TOL_COUNTS_*: TOL formal. Si |medido-target| <= TOL, el punto es OK.
 * CAL_DEADBAND_COUNTS_*: zona de no-movimiento durante biseccion. Evita perseguir
 *   ruido/LSB cuando ya esta cerca. Si DEADBAND > TOL converge mas rapido pero
 *   verify puede marcar la etapa como fuera de tolerancia.
 * CAL_SETTLE_SAMPLES_*: muestras descartadas tras cambiar DAC/AMux antes de medir.
 * CAL_AVG_N_* y CAL_AVG_WINDOW_COUNT_*: piso de promediado = N*WINDOW.
 *   Bajarlos acelera y aumenta ruido; subirlos suaviza y tarda mas.
 * CAL_AVG_MAX_SAMPLES_*: techo de muestras por medicion de biseccion.
 * CAL_AVG_SETTLE_TOL_*: cambio permitido entre promedios para considerar estable.
 *   Mas grande corta antes; mas chico exige mas estabilidad.
 * CAL_AVG_STABLE_STREAK_*: cantidad de comparaciones estables consecutivas.
 * CAL_VERIFY_AVG_MAX_SAMPLES_*: techo de muestras en la confirmacion verify.
 * CAL_SAT_COUNTS_*: umbral de riel/saturacion real, no banda buena.
 * CAL_REALCHECK_ENABLE_*: 1 mide con entrada real y ajusta fino; 0 salta esa fase.
 * CAL_REALCHECK_TOL_COUNTS_*: TOL formal durante realcheck.
 * CAL_REALCHECK_NUDGE_STEP_*: paso fino en LSB VDAC.
 * CAL_REALCHECK_MAX_NUDGES_*: cantidad maxima de nudges; mas alto tarda mas.
 * CAL_REALCHECK_DISCARD_SAMPLES_*: descarte al entrar al realcheck.
 * CAL_REALCHECK_NUDGE_DISCARD_SAMPLES_*: descarte despues de cada nudge.
 * CAL_REALCHECK_AVG_*: promediado propio del realcheck; suele ser el mayor costo.
 * CAL_SERVO_*: parametros del mantenimiento lento en IDLE; hoy desactivado por
 *   CAL_SERVO_ENABLE_DEFAULT=0, no afectan la calibracion manual salvo que lo actives. */

/* ===== Identidad / objetivo =====
 * ADC_CFG1 diferencial signed 18 bit, 0 counts = 0V diferencial (señal del
 * geofono en reposo). direction=1: valor inicial del sentido
 * de busqueda (sube el codigo VDAC -> sube la medida); EVAL_PROBE mide el
 * signo real al arrancar cada corrida y lo guarda para la fase realcheck. */
#define CAL_TARGET_COUNTS_GEO_ADDER   0L
#define CAL_DIRECTION_GEO_ADDER       1

/* ===== Rango de busqueda (biseccion) =====
 * dac_center/max_change acotan el rango [center-max_change, center+max_change].
 * probe_step es el primer paso (antes de la biseccion) para detectar el signo
 * real de la pendiente. max_iter limita las iteraciones de biseccion.
 * tolerance_counts: 0.1V aprox = 52429 counts/V * 0.1V = 5243 counts. */
#define CAL_DAC_CENTER_GEO_ADDER      0x9Cu
#define CAL_DAC_MAX_CHANGE_GEO_ADDER  64u
#define CAL_PROBE_STEP_GEO_ADDER      32u
#define CAL_MAX_ITER_GEO_ADDER        16u
#define CAL_TOL_COUNTS_GEO_ADDER      5243L

/* TOL vs DEADBAND:
 * TOL define si el resultado es suficientemente bueno.
 * DEADBAND define cuando dejamos de mover el DAC aunque todavia podria haber
 * ruido alrededor. Igual a TOL = criterio conservador y simple. */
#define CAL_DEADBAND_COUNTS_GEO_ADDER 5243L

/* ===== Asentamiento: tiempo simulado 200 us, convertido a muestras con
 * ADC_DEFAULT_SRATE y CAL_SETTLE_FACTOR/DEN. */
#define CAL_SETTLE_SAMPLES_GEO_ADDER        CAL_SETTLE_SAMPLES_FROM_US(200u)
#define CAL_VERIFY_SETTLE_SAMPLES_GEO_ADDER CAL_SETTLE_SAMPLES_FROM_US(200u)

/* ===== Promediado/convergencia: biseccion =====
 * measured = promedio de las ultimas avg_n*window_count muestras (peso
 * constante, no se diluye). "Estable" = ese promedio varia
 * <= settle_tol_counts durante stable_streak comparaciones consecutivas, una
 * vez el buffer lleno (piso = avg_n*window_count muestras). Si se llega a
 * max_samples sin racha estable, se usa el promedio igual. */
#define CAL_AVG_N_GEO_ADDER              64u
#define CAL_AVG_WINDOW_COUNT_GEO_ADDER   16u   /* piso 1024 muestras (~341ms) */
#define CAL_AVG_MAX_SAMPLES_GEO_ADDER    2048u /* techo ~683ms */
#define CAL_AVG_SETTLE_TOL_GEO_ADDER     10L
#define CAL_AVG_STABLE_STREAK_GEO_ADDER  3u

/* ===== Promediado/convergencia: verify =====
 * Mismo avg_n/window_count/settle_tol/stable_streak que biseccion, solo
 * cambia el techo (mas tiempo para confirmar el resultado final). */
#define CAL_VERIFY_AVG_MAX_SAMPLES_GEO_ADDER 4096u /* ~1.4s */

/* ===== Anti-saturacion =====
 * Umbral de riel real (distinto de CAL_OPERATING_RANGE_COUNTS, que es la
 * "banda buena" +-0.5V). Full-scale ADC_CFG1 = +-131072 counts aprox; 120000
 * = ~91% FS = ~2.29V. Ajustar mirando PSOC_EVT_CAL_STAGE_MEAS32 (sin clamp):
 * si el valor queda fijo en ~131072 sin importar el DAC, esta saturado de
 * verdad; si varia aunque sea >26214, solo esta "fuera de banda". */
#define CAL_SAT_COUNTS_GEO_ADDER   120000L

/* ===== Etapa final con señal real (entrada fija al geofono) =====
 * Ajuste fino de +-nudge_step LSB sobre el resultado de verify, hasta
 * max_nudges veces; revierte si un nudge satura o empeora |error|.
 * enable=0 salta la etapa (queda el resultado de verify tal cual). */
#define CAL_REALCHECK_ENABLE_GEO_ADDER            1u
#define CAL_REALCHECK_TOL_COUNTS_GEO_ADDER        5243L
#define CAL_REALCHECK_NUDGE_STEP_GEO_ADDER        1u
#define CAL_REALCHECK_MAX_NUDGES_GEO_ADDER        3u
#define CAL_REALCHECK_DISCARD_SAMPLES_GEO_ADDER       CAL_SETTLE_SAMPLES_FROM_US(200u)
#define CAL_REALCHECK_NUDGE_DISCARD_SAMPLES_GEO_ADDER CAL_SETTLE_SAMPLES_FROM_US(200u)
#define CAL_REALCHECK_AVG_N_GEO_ADDER              64u
#define CAL_REALCHECK_AVG_WINDOW_COUNT_GEO_ADDER   64u   /* piso 4096 muestras (~1.4s) */
#define CAL_REALCHECK_AVG_MAX_SAMPLES_GEO_ADDER    8192u /* techo ~2.7s */
#define CAL_REALCHECK_AVG_SETTLE_TOL_GEO_ADDER     10L
#define CAL_REALCHECK_AVG_STABLE_STREAK_GEO_ADDER  3u

/* ===== Controlador PI de calibracion (unico camino activo, GEO incluido —
 * ver bloque "Controlador PI de calibracion" en calibration_tables.h para el
 * detalle de la ley de control). Sin datos de banco todavia: mismos
 * Kp/Ki de partida que HAMMER y lsb_counts estimado asumiendo ganancia ~1x
 * entre VDAC_Ref_Adder y el punto de medicion (AMux_ADC=2) — ajustar con
 * osciloscopio. */
#define CAL_PI_KP_NUM_GEO_ADDER        1L
#define CAL_PI_KP_DIV_GEO_ADDER        2000L
#define CAL_PI_KI_NUM_GEO_ADDER        1L
#define CAL_PI_KI_DIV_GEO_ADDER        16000L
#define CAL_PI_LSB_COUNTS_GEO_ADDER    839L
#define CAL_PI_MAX_SAMPLES_GEO_ADDER   6000u

/* ===== Servo PI (mantenimiento en IDLE, CAL_SERVO_ENABLE_DEFAULT=0) =====
 * control = err*KP_NUM + integral*KI_NUM/KI_DIV; el signo decide el siguiente
 * LSB. Reubicado aca para ajustarlo sin tocar calibration_tables.h.
 * DEADBAND mas chico que las otras etapas (valor original heredado). */
#define CAL_SERVO_KP_NUM_GEO_ADDER         1L
#define CAL_SERVO_KI_NUM_GEO_ADDER         1L
#define CAL_SERVO_KI_DIV_GEO_ADDER         8L
#define CAL_SERVO_DEADBAND_GEO_ADDER       2000L
#define CAL_SERVO_FINE_STEP_GEO_ADDER      1u
#define CAL_SERVO_RECOVERY_STEP_GEO_ADDER  1u
#define CAL_SERVO_SETTLE_SAMPLES_GEO_ADDER CAL_SETTLE_SAMPLES_FROM_US(200u)

#endif
