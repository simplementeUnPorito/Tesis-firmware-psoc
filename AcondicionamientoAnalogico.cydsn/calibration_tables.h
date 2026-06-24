#ifndef CALIBRATION_TABLES_H
#define CALIBRATION_TABLES_H

#include "calibration.h"

/* ============================================================
 * Este archivo es el AGREGADOR de la configuracion de calibracion.
 * Los parametros POR ETAPA GEO (target, rango de busqueda, promediado
 * de biseccion/verify, saturacion, fase realcheck y ganancias del
 * servo PI) viven en headers independientes, uno por VDAC:
 *   - calibration_tables_geo_pga.h
 *   - calibration_tables_geo_bp.h
 *   - calibration_tables_geo_adder.h
 *   - calibration_tables_geo_lp.h
 * Cada uno es 100% autocontenido (sin alias a constantes compartidas) para
 * poder afinarlos por separado con el osciloscopio. Lo que queda aca son
 * los parametros realmente GLOBALES (canales AMux, "banda buena" operativa,
 * limites de buffers internos) y el bloque HAMMER (PSOC_HW_CLASS lo selecciona
 * cuando el TopDesign trae el componente PGA — confirmado vía
 * codegentemp/generated_files.txt: hoy es el front-end HAMMER el que está
 * colocado, no PGAgain/GEO; PGAgain.c/.h en Generated_Source son huérfanos).
 * ============================================================ */

/* Guia global:
 * CAL_AMUX_SETTLE_MS / CAL_DAC_SETTLE_MS: defaults legacy de espera fija.
 *   La calibracion GEO actual usa principalmente *_SETTLE_SAMPLES_* por etapa.
 * CAL_TARGET_1V_COUNTS / CAL_TARGET_1V5_COUNTS: objetivos auxiliares del
 *   bloque HAMMER, en cuentas ADC.
 * CAL_OPERATING_RANGE_COUNTS: banda buena absoluta. Si GEO_LP termina dentro
 *   de este rango, la calibracion final se considera usable para la GUI.
 * CAL_AVG_WINDOW_MAX: limite del buffer interno de promediado; debe ser >= al
 *   mayor *_AVG_WINDOW_COUNT_* usado.
 * CAL_DAC_INIT / CAL_DAC_CENTER: centro nominal del VDAC, 0x9C ~= 2.5 V.
 * CAL_FAIL_FAST_ON_STAGE_FAIL: en GEO queda 0 para completar todas las etapas
 *   y obtener logs aunque una etapa no cierre perfecto.
 * CAL_BEST_CANDIDATE_COUNT: cuantos mejores puntos guarda la biseccion.
 * CAL_VISIT_HISTORY_COUNT: memoria de DACs visitados para evitar loops.
 * CAL_WATCHDOG_TICKS: timeout global de la calibracion; tick = 10 ms.
 * CAL_SERVO_ENABLE_DEFAULT: habilita el servo lento en IDLE; hoy 0.
 * CAL_SERVO_* globales: timing/limites del servo, no de la corrida manual.
 * Bloque HAMMER: no es usado por el hardware GEO actual; queda compilable. */

#ifndef CAL_AMUX_SETTLE_MS
#define CAL_AMUX_SETTLE_MS 5u
#endif

#ifndef CAL_DAC_SETTLE_MS
#define CAL_DAC_SETTLE_MS 5u
#endif

#ifndef CAL_TARGET_1V_COUNTS
#define CAL_TARGET_1V_COUNTS 52429L
#endif

#ifndef CAL_TARGET_1V5_COUNTS
#define CAL_TARGET_1V5_COUNTS 78644L
#endif

/* Escalas fisicas usadas por el PI activo. El ADC se compara en el mismo
 * dominio que el VDAC: ADC 0..5V, 18 bits; VDAC8 0..4.080V, 8 bits. */
#ifndef CAL_ADC_SPAN_MV
#define CAL_ADC_SPAN_MV 5000L
#endif

#ifndef CAL_ADC_FULL_SCALE_COUNTS
#define CAL_ADC_FULL_SCALE_COUNTS 262144L
#endif

#ifndef CAL_VDAC_SPAN_MV
#define CAL_VDAC_SPAN_MV 4080L
#endif

#ifndef CAL_VDAC_CODE_MAX
#define CAL_VDAC_CODE_MAX 255L
#endif

#ifndef CAL_VDAC8_MV_PER_LSB
#define CAL_VDAC8_MV_PER_LSB 16L
#endif

#ifndef CAL_PI_DEADBAND_MARGIN_NUM
#define CAL_PI_DEADBAND_MARGIN_NUM 6L
#endif

#ifndef CAL_PI_DEADBAND_MARGIN_DEN
#define CAL_PI_DEADBAND_MARGIN_DEN 5L
#endif

#ifndef CAL_PI_DEADBAND_MIN_DAC_CODES
#define CAL_PI_DEADBAND_MIN_DAC_CODES 1L
#endif

#ifndef CAL_PI_MAX_DAC_STEP_PER_SAMPLE
#define CAL_PI_MAX_DAC_STEP_PER_SAMPLE 1u
#endif

/* Parametros POR ETAPA HAMMER (target, adelanto/rango de busqueda, PI,
 * asentamiento) viven en headers independientes, uno por VDAC -- mismo
 * patron que los headers GEO de mas abajo:
 *   - calibration_tables_hammer_pga.h
 *   - calibration_tables_hammer_lp.h
 * Incluidos aca arriba (no junto a los de GEO) porque el bloque HAMMER de
 * abajo en este archivo todavia los referencia para armar g_psoc_cal_stages. */
#include "calibration_tables_hammer_pga.h"
#include "calibration_tables_hammer_lp.h"

/* Rango operativo absoluto: ADC_CFG1_COUNTS_PER_VOLT=52429 (ver HANDOFF
 * §4/§13), entonces 0.5V =~ 26214 counts. Se conserva como alarma de salud:
 * si una etapa queda fuera de esto, el log lo muestra como no OK, pero no se
 * abandona la cascada ni se vuelve al default. Distinto de
 * CAL_SAT_COUNTS_GEO_* (por etapa, en cada header) que es el umbral de riel
 * real. */
#define CAL_OPERATING_RANGE_COUNTS 26214L

/* Tamaño del buffer circular de sumas-de-ventana usado por
 * async_measure_service (calibration.c) para el promedio deslizante. Debe
 * ser >= el window_count mas grande entre TODAS las etapas/fases (hoy:
 * CAL_REALCHECK_AVG_WINDOW_COUNT_GEO_* = 64). Una sola instancia porque las
 * etapas se procesan secuencialmente. */
#define CAL_AVG_WINDOW_MAX 64u

/* Descartes por asentamiento calculados desde el sample rate real del ADC.
 * CAL_SETTLE_FACTOR esta en calibration.h. Con factor 3x y
 * ADC_DEFAULT_SRATE=3000:
 *   PGA 220ms -> 1980 muestras, BP 100us -> 1, Adder 200us -> 2,
 *   LP 6ms -> 54. */
#ifndef ADC_DEFAULT_SRATE
#define ADC_DEFAULT_SRATE 3000u
#endif

#define CAL_SETTLE_SAMPLES_FROM_US(us) \
    ((uint16)((((uint32)(us) * (uint32)ADC_DEFAULT_SRATE * (uint32)CAL_SETTLE_FACTOR) + \
               ((uint32)CAL_SETTLE_FACTOR_DEN * 1000000UL) - 1UL) / \
              ((uint32)CAL_SETTLE_FACTOR_DEN * 1000000UL)))

#define CAL_SETTLE_SAMPLES_FROM_MS(ms) \
    CAL_SETTLE_SAMPLES_FROM_US(((uint32)(ms) * 1000UL))

/* Punto de partida obligatorio de la busqueda binaria: 0x9C = 156 ->
 * 156 * 16mV (VDAC8 1x) = 2.496V =~ 2.5V (centro de rango / "tierra
 * virtual" del front-end analogico). Las etapas GEO ya usan el literal
 * 0x9Cu directamente en sus headers; estas quedan para el bloque HAMMER. */
#define CAL_DAC_INIT   0x9Cu
#define CAL_DAC_CENTER CAL_DAC_INIT
#define CAL_DAC_MAX_CHANGE_HAMMER 255u

/* En GEO no cortar en la primera etapa: la etapa siguiente puede compensar
 * parte del residual, y necesitamos ver el diagnostico completo de las cuatro
 * etapas aunque una quede fuera de tolerancia. */
#define CAL_FAIL_FAST_ON_STAGE_FAIL 0u

/* El adelanto (feedforward) y el rango de busqueda de HAMMER ya NO viven
 * aca -- son individuales por etapa, ver CAL_ADELANTO_HAMMER_PGA_MV /
 * CAL_DAC_CENTER_HAMMER_PGA / CAL_DAC_MAX_CHANGE_HAMMER_PGA (y su _LP) en
 * calibration_tables_hammer_pga.h / _lp.h, incluidos mas arriba. */

#define CAL_BEST_CANDIDATE_COUNT 4u
#define CAL_VISIT_HISTORY_COUNT  8u

/* 10 ms/tick. La fase realcheck puede sumar decenas de segundos, asi que el
 * watchdog PSoC queda en 400 s y el timeout del ESP debe ser mayor. */
#define CAL_WATCHDOG_TICKS 40000UL

/* ============================================================
 * Servo lento de mantenimiento en IDLE (CAL_SERVO_ENABLE_DEFAULT=0).
 * No reemplaza al comando manual/boot de calibracion; corrige deriva de a
 * poco, por turnos. Estas son las constantes del LOOP en si (timing/limites,
 * no dependen de la etapa); las ganancias PI por etapa GEO viven en cada
 * calibration_tables_geo_*.h. */
#ifndef CAL_SERVO_ENABLE_DEFAULT
#define CAL_SERVO_ENABLE_DEFAULT 0u
#endif

#define CAL_SERVO_WORSE_HYST_COUNTS      250L
#define CAL_SERVO_INTEGRAL_LIMIT      419424L
#define CAL_SERVO_AVG_N                   32u
#define CAL_SERVO_SETTLE_MAX_WINDOWS       8u
#define CAL_SERVO_SETTLE_TOL_COUNTS       50L
#define CAL_SERVO_PERIOD_TICKS            25u

/* Defaults compartidos solo por el bloque HAMMER (las etapas GEO ya tienen
 * sus propios CAL_SERVO_DEADBAND/FINE_STEP/RECOVERY_STEP_GEO_* literales). */
#define CAL_SERVO_DEADBAND_COUNTS       5000L
#define CAL_SERVO_FINE_STEP                1u
#define CAL_SERVO_RECOVERY_STEP            1u

/* ============================================================
 * BLOQUE HAMMER: hoy es el front-end activo (ver nota arriba). Usa un solo
 * juego de parametros de biseccion (anti-saturacion/promediado/realcheck)
 * compartido entre sus 3 etapas, con la fase realcheck deshabilitada
 * (CAL_REALCHECK_ENABLE_HAMMER=0) — esa parte queda como código histórico,
 * reemplazada en la práctica por el controlador PI de mas abajo
 * (CAL_PI_*_HAMMER_*), que es el que corre hoy para HAMMER
 * (psoc_calibration_service_async, ver calibration.c).
 * ============================================================ */
#define CAL_AVG_N      32u
#define CAL_MAX_ITER   16u
/* HAMMER se esta cerrando con lectura directa del ADC crudo (sin el capacitor
 * de AMux, porque con capacitor la ruta queda saturada/congelada en banco).
 * El ruido observado por ESP ronda decenas de mV, asi que 250 counts (~5 mV)
 * hacia fallar etapas ya centradas. 6000 counts son ~114 mV con
 * CAL_TARGET_1V_COUNTS=52429 counts/V. */
/* CAL_TOL_COUNTS/DEADBAND/PROBE_STEP/SETTLE_SAMPLES/SAT_COUNTS por etapa HAMMER
 * (antes globales aca) ya viven en calibration_tables_hammer_pga.h / _lp.h,
 * incluidos mas arriba -- CAL_TOL_COUNTS_HAMMER_PGA, CAL_TOL_COUNTS_HAMMER_LP,
 * etc. Lo que queda aca es la configuracion de promediado/realcheck de la
 * biseccion vieja (compartida, sin uso por el PI -- ver #if 0 en
 * calibration.c). */

#define CAL_AVG_N_HAMMER              32u
#define CAL_AVG_WINDOW_COUNT_HAMMER    8u   /* piso 256 muestras */
#define CAL_AVG_MAX_SAMPLES_HAMMER    512u
#define CAL_AVG_SETTLE_TOL_HAMMER      10L
#define CAL_AVG_STABLE_STREAK_HAMMER    2u
#define CAL_VERIFY_AVG_MAX_SAMPLES_HAMMER 1024u

#define CAL_REALCHECK_ENABLE_HAMMER              0u
#define CAL_REALCHECK_TOL_COUNTS_HAMMER          6000L
#define CAL_REALCHECK_NUDGE_STEP_HAMMER          1u
#define CAL_REALCHECK_MAX_NUDGES_HAMMER          0u
#define CAL_REALCHECK_DISCARD_SAMPLES_HAMMER     0u
#define CAL_REALCHECK_NUDGE_DISCARD_SAMPLES_HAMMER 0u

#define CAL_HAMMER_AVG_CFG \
    { CAL_AVG_N_HAMMER, CAL_AVG_WINDOW_COUNT_HAMMER, CAL_AVG_MAX_SAMPLES_HAMMER, CAL_AVG_SETTLE_TOL_HAMMER, CAL_AVG_STABLE_STREAK_HAMMER }
#define CAL_HAMMER_VERIFY_AVG_CFG \
    { CAL_AVG_N_HAMMER, CAL_AVG_WINDOW_COUNT_HAMMER, CAL_VERIFY_AVG_MAX_SAMPLES_HAMMER, CAL_AVG_SETTLE_TOL_HAMMER, CAL_AVG_STABLE_STREAK_HAMMER }
#define CAL_HAMMER_REALCHECK_CFG \
    { CAL_REALCHECK_ENABLE_HAMMER, CAL_REALCHECK_TOL_COUNTS_HAMMER, CAL_REALCHECK_NUDGE_STEP_HAMMER, CAL_REALCHECK_MAX_NUDGES_HAMMER, \
      CAL_REALCHECK_DISCARD_SAMPLES_HAMMER, CAL_REALCHECK_NUDGE_DISCARD_SAMPLES_HAMMER, \
      { CAL_AVG_N_HAMMER, CAL_AVG_WINDOW_COUNT_HAMMER, CAL_AVG_MAX_SAMPLES_HAMMER, CAL_AVG_SETTLE_TOL_HAMMER, CAL_AVG_STABLE_STREAK_HAMMER } }

/* CAL_SERVO_*_HAMMER_PGA/_LP (servo lento, legado sin uso) y CAL_PI_KP/KI/
 * LSB_COUNTS/MAX_SAMPLES_HAMMER_PGA/_LP (ganancias del PI activo) ya viven
 * en calibration_tables_hammer_pga.h / _lp.h, incluidos mas arriba. */

/* ============================================================
 * Controlador PI de calibracion: UNICO algoritmo activo, GEO y HAMMER por
 * igual. Usa el FIR de hardware como unica fuente de suavizado (nunca
 * promediado de software). La biseccion vieja y el servo lento
 * (calibration.c, CAL_ALGO_BISECTION_ENABLE) quedaron comentados a pedido
 * del usuario, no se borraron.
 *
 * Ley de control por muestra (a 3kHz, sobre fir_output = salida del Filter
 * de hardware vía DMA_Filter_RAM, ver cal_pi_run_service en calibration.c):
 *   error_counts = target_counts - fir_output
 *   error_dac    = error_counts reescalado a ADC 18b 0..5V vs DAC 8b 0..4.080V
 *   effort       = adelanto + direction*(P(error_dac/gain) + I(error_dac/gain))
 *   integral    += error_dac (solo fuera de deadband y sin saturacion)
 *   dac    = clamp(effort), se escribe siempre
 * dac_center hace de feedforward/adelanto: el PI solo compensa el residuo.
 *
 * Deadband = ceil(ganancia_absoluta * CAL_PI_DEADBAND_MARGIN_NUM /
 * CAL_PI_DEADBAND_MARGIN_DEN) codigos DAC, con minimo
 * CAL_PI_DEADBAND_MIN_DAC_CODES. Para HAMMER_PGA la ganancia es dinamica:
 * abs(PGA_GAIN - 1) porque el VDAC entra por el terminal negativo. El lock
 * cierra la etapa si durante lock_samples (= M) el error queda en la misma
 * celda cuantizada; bucket 0 significa "dentro de deadband".
 *
 * Superficie de ajuste por etapa, a proposito limitada a esto (y nada mas):
 * Kp (num/div), Ki (num/div), ganancia_x1000 y samples para lock. Sin
 * tolerancia aparte, sin lsb_counts aparte, sin paso de refine/verify
 * promediado al cerrar la etapa.
 * ============================================================ */
#define CAL_PI_LOCK_N_MAX 128u   /* capacidad del buffer; lock_samples (por etapa) puede ser <= esto */

#ifndef CAL_PI_TIMEOUT_SAMPLES
#define CAL_PI_TIMEOUT_SAMPLES 20000u   /* tope de seguridad GLOBAL, no por etapa: si nunca lockea, se rinde */
#endif

/* Kp/Ki/gain/lock_samples por etapa HAMMER ya viven en
 * calibration_tables_hammer_pga.h / _lp.h. */

/* ============================================================
 * Headers por VDAC (etapas GEO). Incluidos siempre (son #define puros, sin
 * dependencias de hardware) para que esten disponibles aunque se compile en
 * modo HAMMER.
 * ============================================================ */
#include "calibration_tables_geo_pga.h"
#include "calibration_tables_geo_bp.h"
#include "calibration_tables_geo_adder.h"
#include "calibration_tables_geo_lp.h"

#if PSOC_HW_CLASS == PSOC_HW_GEO

#ifndef CAL_ADC_CAPTURE_CHANNEL
#define CAL_ADC_CAPTURE_CHANNEL 3u
#endif

#if !defined(VDAC_ref_PGA_DEFAULT_DATA)
    #error "AnalogGeo requiere el componente VDAC_ref_PGA."
#endif
/* VDAC_ref_BP es opcional: si no esta en TopDesign, GEO_BP se omite. */
#if !defined(VDAC_Ref_Adder_DEFAULT_DATA)
    #error "AnalogGeo requiere el componente VDAC_Ref_Adder."
#endif
#if !defined(VDAC_ref_LP_DEFAULT_DATA)
    #error "AnalogGeo requiere el componente VDAC_ref_LP."
#endif

static void cal_vdac_geo_pga(uint8 value)   { VDAC_ref_PGA_SetValue(value); }
#if defined(VDAC_ref_BP_DEFAULT_DATA) || defined(CY_DVDAC_VDAC_ref_BP_H)
static void cal_vdac_geo_bp(uint8 value)    { VDAC_ref_BP_SetValue(value); }
#endif
static void cal_vdac_geo_adder(uint8 value) { VDAC_Ref_Adder_SetValue(value); }
static void cal_vdac_geo_lp(uint8 value)    { VDAC_ref_LP_SetValue(value); }

static const PsocCalStage g_psoc_cal_stages[] = {
    {
        "GEO_PGA", 0u, CAL_TARGET_COUNTS_GEO_PGA, CAL_DIRECTION_GEO_PGA,
        CAL_DAC_CENTER_GEO_PGA, CAL_DAC_MAX_CHANGE_GEO_PGA, CAL_PROBE_STEP_GEO_PGA,
        CAL_MAX_ITER_GEO_PGA, CAL_TOL_COUNTS_GEO_PGA, CAL_DEADBAND_COUNTS_GEO_PGA, CAL_SAT_COUNTS_GEO_PGA,
        CAL_SETTLE_SAMPLES_GEO_PGA, CAL_VERIFY_SETTLE_SAMPLES_GEO_PGA,
        { CAL_AVG_N_GEO_PGA, CAL_AVG_WINDOW_COUNT_GEO_PGA, CAL_AVG_MAX_SAMPLES_GEO_PGA, CAL_AVG_SETTLE_TOL_GEO_PGA, CAL_AVG_STABLE_STREAK_GEO_PGA },
        { CAL_AVG_N_GEO_PGA, CAL_AVG_WINDOW_COUNT_GEO_PGA, CAL_VERIFY_AVG_MAX_SAMPLES_GEO_PGA, CAL_AVG_SETTLE_TOL_GEO_PGA, CAL_AVG_STABLE_STREAK_GEO_PGA },
        {
            CAL_REALCHECK_ENABLE_GEO_PGA, CAL_REALCHECK_TOL_COUNTS_GEO_PGA, CAL_REALCHECK_NUDGE_STEP_GEO_PGA, CAL_REALCHECK_MAX_NUDGES_GEO_PGA,
            CAL_REALCHECK_DISCARD_SAMPLES_GEO_PGA, CAL_REALCHECK_NUDGE_DISCARD_SAMPLES_GEO_PGA,
            { CAL_REALCHECK_AVG_N_GEO_PGA, CAL_REALCHECK_AVG_WINDOW_COUNT_GEO_PGA, CAL_REALCHECK_AVG_MAX_SAMPLES_GEO_PGA, CAL_REALCHECK_AVG_SETTLE_TOL_GEO_PGA, CAL_REALCHECK_AVG_STABLE_STREAK_GEO_PGA }
        },
        cal_vdac_geo_pga
    },
#if defined(VDAC_ref_BP_DEFAULT_DATA) || defined(CY_DVDAC_VDAC_ref_BP_H)
    {
        "GEO_BP", 1u, CAL_TARGET_COUNTS_GEO_BP, CAL_DIRECTION_GEO_BP,
        CAL_DAC_CENTER_GEO_BP, CAL_DAC_MAX_CHANGE_GEO_BP, CAL_PROBE_STEP_GEO_BP,
        CAL_MAX_ITER_GEO_BP, CAL_TOL_COUNTS_GEO_BP, CAL_DEADBAND_COUNTS_GEO_BP, CAL_SAT_COUNTS_GEO_BP,
        CAL_SETTLE_SAMPLES_GEO_BP, CAL_VERIFY_SETTLE_SAMPLES_GEO_BP,
        { CAL_AVG_N_GEO_BP, CAL_AVG_WINDOW_COUNT_GEO_BP, CAL_AVG_MAX_SAMPLES_GEO_BP, CAL_AVG_SETTLE_TOL_GEO_BP, CAL_AVG_STABLE_STREAK_GEO_BP },
        { CAL_AVG_N_GEO_BP, CAL_AVG_WINDOW_COUNT_GEO_BP, CAL_VERIFY_AVG_MAX_SAMPLES_GEO_BP, CAL_AVG_SETTLE_TOL_GEO_BP, CAL_AVG_STABLE_STREAK_GEO_BP },
        {
            CAL_REALCHECK_ENABLE_GEO_BP, CAL_REALCHECK_TOL_COUNTS_GEO_BP, CAL_REALCHECK_NUDGE_STEP_GEO_BP, CAL_REALCHECK_MAX_NUDGES_GEO_BP,
            CAL_REALCHECK_DISCARD_SAMPLES_GEO_BP, CAL_REALCHECK_NUDGE_DISCARD_SAMPLES_GEO_BP,
            { CAL_REALCHECK_AVG_N_GEO_BP, CAL_REALCHECK_AVG_WINDOW_COUNT_GEO_BP, CAL_REALCHECK_AVG_MAX_SAMPLES_GEO_BP, CAL_REALCHECK_AVG_SETTLE_TOL_GEO_BP, CAL_REALCHECK_AVG_STABLE_STREAK_GEO_BP }
        },
        cal_vdac_geo_bp
    },
#endif
    {
        "GEO_ADDER", 2u, CAL_TARGET_COUNTS_GEO_ADDER, CAL_DIRECTION_GEO_ADDER,
        CAL_DAC_CENTER_GEO_ADDER, CAL_DAC_MAX_CHANGE_GEO_ADDER, CAL_PROBE_STEP_GEO_ADDER,
        CAL_MAX_ITER_GEO_ADDER, CAL_TOL_COUNTS_GEO_ADDER, CAL_DEADBAND_COUNTS_GEO_ADDER, CAL_SAT_COUNTS_GEO_ADDER,
        CAL_SETTLE_SAMPLES_GEO_ADDER, CAL_VERIFY_SETTLE_SAMPLES_GEO_ADDER,
        { CAL_AVG_N_GEO_ADDER, CAL_AVG_WINDOW_COUNT_GEO_ADDER, CAL_AVG_MAX_SAMPLES_GEO_ADDER, CAL_AVG_SETTLE_TOL_GEO_ADDER, CAL_AVG_STABLE_STREAK_GEO_ADDER },
        { CAL_AVG_N_GEO_ADDER, CAL_AVG_WINDOW_COUNT_GEO_ADDER, CAL_VERIFY_AVG_MAX_SAMPLES_GEO_ADDER, CAL_AVG_SETTLE_TOL_GEO_ADDER, CAL_AVG_STABLE_STREAK_GEO_ADDER },
        {
            CAL_REALCHECK_ENABLE_GEO_ADDER, CAL_REALCHECK_TOL_COUNTS_GEO_ADDER, CAL_REALCHECK_NUDGE_STEP_GEO_ADDER, CAL_REALCHECK_MAX_NUDGES_GEO_ADDER,
            CAL_REALCHECK_DISCARD_SAMPLES_GEO_ADDER, CAL_REALCHECK_NUDGE_DISCARD_SAMPLES_GEO_ADDER,
            { CAL_REALCHECK_AVG_N_GEO_ADDER, CAL_REALCHECK_AVG_WINDOW_COUNT_GEO_ADDER, CAL_REALCHECK_AVG_MAX_SAMPLES_GEO_ADDER, CAL_REALCHECK_AVG_SETTLE_TOL_GEO_ADDER, CAL_REALCHECK_AVG_STABLE_STREAK_GEO_ADDER }
        },
        cal_vdac_geo_adder
    },
    {
        "GEO_LP", 3u, CAL_TARGET_COUNTS_GEO_LP, CAL_DIRECTION_GEO_LP,
        CAL_DAC_CENTER_GEO_LP, CAL_DAC_MAX_CHANGE_GEO_LP, CAL_PROBE_STEP_GEO_LP,
        CAL_MAX_ITER_GEO_LP, CAL_TOL_COUNTS_GEO_LP, CAL_DEADBAND_COUNTS_GEO_LP, CAL_SAT_COUNTS_GEO_LP,
        CAL_SETTLE_SAMPLES_GEO_LP, CAL_VERIFY_SETTLE_SAMPLES_GEO_LP,
        { CAL_AVG_N_GEO_LP, CAL_AVG_WINDOW_COUNT_GEO_LP, CAL_AVG_MAX_SAMPLES_GEO_LP, CAL_AVG_SETTLE_TOL_GEO_LP, CAL_AVG_STABLE_STREAK_GEO_LP },
        { CAL_AVG_N_GEO_LP, CAL_AVG_WINDOW_COUNT_GEO_LP, CAL_VERIFY_AVG_MAX_SAMPLES_GEO_LP, CAL_AVG_SETTLE_TOL_GEO_LP, CAL_AVG_STABLE_STREAK_GEO_LP },
        {
            CAL_REALCHECK_ENABLE_GEO_LP, CAL_REALCHECK_TOL_COUNTS_GEO_LP, CAL_REALCHECK_NUDGE_STEP_GEO_LP, CAL_REALCHECK_MAX_NUDGES_GEO_LP,
            CAL_REALCHECK_DISCARD_SAMPLES_GEO_LP, CAL_REALCHECK_NUDGE_DISCARD_SAMPLES_GEO_LP,
            { CAL_REALCHECK_AVG_N_GEO_LP, CAL_REALCHECK_AVG_WINDOW_COUNT_GEO_LP, CAL_REALCHECK_AVG_MAX_SAMPLES_GEO_LP, CAL_REALCHECK_AVG_SETTLE_TOL_GEO_LP, CAL_REALCHECK_AVG_STABLE_STREAK_GEO_LP }
        },
        cal_vdac_geo_lp
    },
};

#define PSOC_CAL_STAGE_COUNT ((uint8)(sizeof(g_psoc_cal_stages) / sizeof(g_psoc_cal_stages[0])))

#else

#ifndef CAL_ADC_CAPTURE_CHANNEL
#define CAL_ADC_CAPTURE_CHANNEL 1u
#endif

/* "Precambios" (commit 990f49f9) simplificó AnalogHammer: ya no hay etapa de
 * referencia de entrada separada (VDAC_ref_IN/Opa_ref_IN/Vref/LPF_ref
 * desaparecieron de generated_files.txt) — quedan solo 2 puntos de
 * calibración (PGA, LP), igual que muestra el AMux_ADC de 2 canales
 * (0=LPo, 1=LP) en el esquemático actual. */
#if !defined(VDAC_PGA_DEFAULT_DATA)
    #error "AnalogHammer requiere el componente VDAC_PGA."
#endif
#if !defined(VDAC_LP_DEFAULT_DATA)
    #error "AnalogHammer requiere el componente VDAC_LP."
#endif

static void cal_vdac_hammer_pga(uint8 value) { VDAC_PGA_SetValue(value); }
static void cal_vdac_hammer_lp(uint8 value)  { VDAC_LP_SetValue(value); }

/* CAL_DIRECTION_HAMMER_PGA/_LP ya viven en calibration_tables_hammer_pga.h /
 * _lp.h (sentido VDAC->medida; si una etapa diverge en vez de converger, ese
 * es el primer sospechoso -- invertir el signo ahi). */

static const PsocCalStage g_psoc_cal_stages[] = {
    { "HAMMER_PGA", 0u, CAL_TARGET_HAMMER_PGA_COUNTS, CAL_DIRECTION_HAMMER_PGA, CAL_DAC_CENTER_HAMMER_PGA, CAL_DAC_MAX_CHANGE_HAMMER_PGA, CAL_PROBE_STEP_HAMMER_PGA, CAL_MAX_ITER_HAMMER_PGA, CAL_TOL_COUNTS_HAMMER_PGA, CAL_DEADBAND_COUNTS_HAMMER_PGA, CAL_SAT_COUNTS_HAMMER_PGA, CAL_SETTLE_SAMPLES_HAMMER_PGA, CAL_VERIFY_SETTLE_SAMPLES_HAMMER_PGA, CAL_HAMMER_AVG_CFG, CAL_HAMMER_VERIFY_AVG_CFG, CAL_HAMMER_REALCHECK_CFG, cal_vdac_hammer_pga },
    { "HAMMER_LP",  1u, CAL_TARGET_HAMMER_LP_COUNTS,  CAL_DIRECTION_HAMMER_LP,  CAL_DAC_CENTER_HAMMER_LP,  CAL_DAC_MAX_CHANGE_HAMMER_LP,  CAL_PROBE_STEP_HAMMER_LP,  CAL_MAX_ITER_HAMMER_LP,  CAL_TOL_COUNTS_HAMMER_LP,  CAL_DEADBAND_COUNTS_HAMMER_LP,  CAL_SAT_COUNTS_HAMMER_LP,  CAL_SETTLE_SAMPLES_HAMMER_LP,  CAL_VERIFY_SETTLE_SAMPLES_HAMMER_LP,  CAL_HAMMER_AVG_CFG, CAL_HAMMER_VERIFY_AVG_CFG, CAL_HAMMER_REALCHECK_CFG, cal_vdac_hammer_lp },
};

#define PSOC_CAL_STAGE_COUNT ((uint8)(sizeof(g_psoc_cal_stages) / sizeof(g_psoc_cal_stages[0])))

#endif

#endif
