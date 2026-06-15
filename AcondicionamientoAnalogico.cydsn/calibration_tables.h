#ifndef CALIBRATION_TABLES_H
#define CALIBRATION_TABLES_H

#include "calibration.h"

#ifndef CAL_INPUT_NORMAL_CHANNEL
#define CAL_INPUT_NORMAL_CHANNEL 0u
#endif

/* AMux_IN canal 1 = referencia/tierra virtual de 2.5 V. Durante calibracion
 * la entrada se fuerza ahi para que cada etapa quede centrada tambien en 2.5 V. */
#ifndef CAL_INPUT_REF_CHANNEL
#define CAL_INPUT_REF_CHANNEL 1u
#endif

#ifndef CAL_INPUT_GROUND_CHANNEL
#define CAL_INPUT_GROUND_CHANNEL CAL_INPUT_REF_CHANNEL
#endif

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

#ifndef CAL_TARGET_HAMMER_IN_COUNTS
#define CAL_TARGET_HAMMER_IN_COUNTS CAL_TARGET_1V_COUNTS
#endif

#ifndef CAL_TARGET_HAMMER_PGA_COUNTS
#define CAL_TARGET_HAMMER_PGA_COUNTS CAL_TARGET_1V5_COUNTS
#endif

#ifndef CAL_TARGET_HAMMER_LP_COUNTS
#define CAL_TARGET_HAMMER_LP_COUNTS CAL_TARGET_1V5_COUNTS
#endif

/* ADC_CFG1 es diferencial signed de 18 bits (0 = 0V diferencial). En modo
 * GEO cada etapa debe quedar con su salida DC igualada a AMux_IN=1
 * (referencia/tierra virtual de 2.5 V), es decir lectura diferencial = 0 counts. */
#ifndef CAL_TARGET_GEO_PGA_COUNTS
#define CAL_TARGET_GEO_PGA_COUNTS 0L
#endif

#ifndef CAL_TARGET_GEO_BP_COUNTS
#define CAL_TARGET_GEO_BP_COUNTS 0L
#endif

#ifndef CAL_TARGET_GEO_ADDER_COUNTS
#define CAL_TARGET_GEO_ADDER_COUNTS 0L
#endif

#ifndef CAL_TARGET_GEO_LP_COUNTS
#define CAL_TARGET_GEO_LP_COUNTS 0L
#endif

#define CAL_AVG_N      32u
#define CAL_MAX_ITER   16u
#define CAL_TOL_COUNTS 250L

/* Rango operativo absoluto: ADC_CFG1_COUNTS_PER_VOLT=52429 (ver HANDOFF
 * §4/§13), entonces 0.5V =~ 26214 counts. Se conserva como alarma de salud:
 * si una etapa queda fuera de esto, el log lo muestra como no OK, pero no se
 * abandona la cascada ni se vuelve al default. */
#define CAL_OPERATING_RANGE_COUNTS 26214L

/* Tolerancia fina GEO: 0.1V aprox. 52429 counts/V * 0.1V = 5243 counts.
 * Esto fuerza a la busqueda a acercarse mucho mas al cero antes de lockear.
 * Si no existe un codigo que llegue, igual queda escrito el mejor candidato. */
#define CAL_FINE_TOL_COUNTS_GEO 5243L
#define CAL_TOL_COUNTS_GEO_PGA   CAL_FINE_TOL_COUNTS_GEO
#define CAL_TOL_COUNTS_GEO_BP    CAL_FINE_TOL_COUNTS_GEO
#define CAL_TOL_COUNTS_GEO_ADDER CAL_FINE_TOL_COUNTS_GEO
#define CAL_TOL_COUNTS_GEO_LP    CAL_FINE_TOL_COUNTS_GEO

/* Espera en muestras ADC descartadas despues de tocar cada VDAC. CFG1 corre a
 * ~3 kSPS, asi que 300 muestras son aproximadamente 100 ms. Los nodos mas
 * abajo en la cascada esperan mas porque ven el asentamiento acumulado. */
#define CAL_SETTLE_SAMPLES_GEO_PGA   3000u
#define CAL_SETTLE_SAMPLES_GEO_BP   15000u
#define CAL_SETTLE_SAMPLES_GEO_ADDER 8000u
#define CAL_SETTLE_SAMPLES_GEO_LP    6000u

#define CAL_VERIFY_SAMPLES_GEO_PGA    5000u
#define CAL_VERIFY_SAMPLES_GEO_BP    24000u
#define CAL_VERIFY_SAMPLES_GEO_ADDER 12000u
#define CAL_VERIFY_SAMPLES_GEO_LP     9000u

#define CAL_SETTLE_SAMPLES_HAMMER_IN   180u
#define CAL_SETTLE_SAMPLES_HAMMER_PGA  600u
#define CAL_SETTLE_SAMPLES_HAMMER_LP   900u

#define CAL_VERIFY_SAMPLES_HAMMER_IN   300u
#define CAL_VERIFY_SAMPLES_HAMMER_PGA  900u
#define CAL_VERIFY_SAMPLES_HAMMER_LP  1200u

/* Punto de partida obligatorio de la busqueda binaria: 0x9C = 156 ->
 * 156 * 16mV (VDAC8 1x) = 2.496V =~ 2.5V (centro de rango / "tierra
 * virtual" del front-end analogico). */
#define CAL_DAC_INIT   0x9Cu

/* Cada etapa busca alrededor de su codigo VDAC esperado, no sobre todo el
 * rango. Asi esto escala a otros front-ends: se ajusta CENTER/MAX_CHANGE por
 * operacional y la maquina de estados calcula center +- max_change. */
#define CAL_DAC_CENTER CAL_DAC_INIT
#define CAL_DAC_MAX_CHANGE_GEO 64u
#define CAL_DAC_MAX_CHANGE_HAMMER 255u

/* En GEO no cortar en la primera etapa: la etapa siguiente puede compensar
 * parte del residual, y necesitamos ver el diagnostico completo de las cuatro
 * etapas aunque una quede fuera de tolerancia. */
#define CAL_FAIL_FAST_ON_STAGE_FAIL 0u

#define CAL_DAC_CENTER_GEO_PGA   CAL_DAC_CENTER
#define CAL_DAC_CENTER_GEO_BP    CAL_DAC_CENTER
#define CAL_DAC_CENTER_GEO_ADDER CAL_DAC_CENTER
#define CAL_DAC_CENTER_GEO_LP    CAL_DAC_CENTER

#define CAL_DAC_MAX_CHANGE_GEO_PGA   CAL_DAC_MAX_CHANGE_GEO
#define CAL_DAC_MAX_CHANGE_GEO_BP    CAL_DAC_MAX_CHANGE_GEO
#define CAL_DAC_MAX_CHANGE_GEO_ADDER CAL_DAC_MAX_CHANGE_GEO
#define CAL_DAC_MAX_CHANGE_GEO_LP    CAL_DAC_MAX_CHANGE_GEO

#define CAL_DAC_CENTER_HAMMER_IN  CAL_DAC_CENTER
#define CAL_DAC_CENTER_HAMMER_PGA CAL_DAC_CENTER
#define CAL_DAC_CENTER_HAMMER_LP  CAL_DAC_CENTER

#define CAL_DAC_MAX_CHANGE_HAMMER_IN  CAL_DAC_MAX_CHANGE_HAMMER
#define CAL_DAC_MAX_CHANGE_HAMMER_PGA CAL_DAC_MAX_CHANGE_HAMMER
#define CAL_DAC_MAX_CHANGE_HAMMER_LP  CAL_DAC_MAX_CHANGE_HAMMER

#define CAL_BEST_CANDIDATE_COUNT 4u
#define CAL_VISIT_HISTORY_COUNT  8u

/* Primer paso (antes de la busqueda binaria) para detectar empiricamente si
 * la medida crece o decrece con el codigo DAC. */
#define CAL_PROBE_STEP 32u

/* La maquina de estados no bloqueante descarta settle_samples y luego sigue
 * promediando ACUMULATIVAMENTE (ver async_measure_service en calibration.c)
 * hasta que el promedio acumulado deje de moverse mas de
 * CAL_SETTLE_TOL_COUNTS de una ventana a la siguiente, o hasta
 * CAL_SETTLE_MAX_WINDOWS como limite de seguridad. CAL_SETTLE_TOL_COUNTS=10
 * (~+-10 counts, pedido del usuario 2026-06-14) es estricto porque se compara
 * contra el promedio ACUMULADO (cada vez mas estable), no contra una ventana
 * sola. CAL_SETTLE_MAX_WINDOWS=40 (vs 10 antes) => hasta 40*32=1280 muestras
 * por punto (~426 ms a ~3 kSPS); con ~60 puntos medidos en una corrida
 * completa eso agrega ~19 s en el peor caso, muy por debajo de
 * CAL_WATCHDOG_TICKS=200 s. */
#define CAL_SETTLE_MAX_WINDOWS 40u
#define CAL_SETTLE_TOL_COUNTS  10L

/* Servo lento de mantenimiento en IDLE. No reemplaza al comando manual de
 * calibracion; corrige deriva de a poco, por turnos, con AMux_IN=referencia. */
#ifndef CAL_SERVO_ENABLE_DEFAULT
#define CAL_SERVO_ENABLE_DEFAULT 0u
#endif

#define CAL_SERVO_DEADBAND_COUNTS       5000L
#define CAL_SERVO_WORSE_HYST_COUNTS      250L
#define CAL_SERVO_INTEGRAL_LIMIT      419424L
#define CAL_SERVO_FINE_STEP                1u
#define CAL_SERVO_RECOVERY_STEP            1u
#define CAL_SERVO_AVG_N                   32u
#define CAL_SERVO_SETTLE_MAX_WINDOWS       8u
#define CAL_SERVO_SETTLE_TOL_COUNTS       50L
#define CAL_SERVO_PERIOD_TICKS            25u

#define CAL_SERVO_SETTLE_SAMPLES_GEO_PGA    1200u
#define CAL_SERVO_SETTLE_SAMPLES_GEO_BP     6000u
#define CAL_SERVO_SETTLE_SAMPLES_GEO_ADDER  3000u
#define CAL_SERVO_SETTLE_SAMPLES_GEO_LP     3000u

#define CAL_SERVO_SETTLE_SAMPLES_HAMMER_IN   180u
#define CAL_SERVO_SETTLE_SAMPLES_HAMMER_PGA  600u
#define CAL_SERVO_SETTLE_SAMPLES_HAMMER_LP   900u

/* PI tuneable por etapa. Estan iguales a proposito para arrancar; en banco se
 * ajustan por separado sin tocar el codigo del servo. control =
 * err*KP_NUM + integral*KI_NUM/KI_DIV, y el signo decide el siguiente LSB. */
#define CAL_SERVO_KP_NUM_GEO_PGA       1L
#define CAL_SERVO_KI_NUM_GEO_PGA       1L
#define CAL_SERVO_KI_DIV_GEO_PGA       8L
#define CAL_SERVO_DEADBAND_GEO_PGA     CAL_SERVO_DEADBAND_COUNTS
#define CAL_SERVO_FINE_STEP_GEO_PGA    CAL_SERVO_FINE_STEP
#define CAL_SERVO_RECOVERY_STEP_GEO_PGA CAL_SERVO_RECOVERY_STEP

#define CAL_SERVO_KP_NUM_GEO_BP        1L
#define CAL_SERVO_KI_NUM_GEO_BP        1L
#define CAL_SERVO_KI_DIV_GEO_BP        8L
#define CAL_SERVO_DEADBAND_GEO_BP      CAL_SERVO_DEADBAND_COUNTS
#define CAL_SERVO_FINE_STEP_GEO_BP     CAL_SERVO_FINE_STEP
#define CAL_SERVO_RECOVERY_STEP_GEO_BP CAL_SERVO_RECOVERY_STEP

#define CAL_SERVO_KP_NUM_GEO_ADDER     1L
#define CAL_SERVO_KI_NUM_GEO_ADDER     1L
#define CAL_SERVO_KI_DIV_GEO_ADDER     8L
#define CAL_SERVO_DEADBAND_GEO_ADDER   2000L
#define CAL_SERVO_FINE_STEP_GEO_ADDER  CAL_SERVO_FINE_STEP
#define CAL_SERVO_RECOVERY_STEP_GEO_ADDER CAL_SERVO_RECOVERY_STEP

#define CAL_SERVO_KP_NUM_GEO_LP        1L
#define CAL_SERVO_KI_NUM_GEO_LP        1L
#define CAL_SERVO_KI_DIV_GEO_LP        8L
#define CAL_SERVO_DEADBAND_GEO_LP      CAL_SERVO_DEADBAND_COUNTS
#define CAL_SERVO_FINE_STEP_GEO_LP     CAL_SERVO_FINE_STEP
#define CAL_SERVO_RECOVERY_STEP_GEO_LP CAL_SERVO_RECOVERY_STEP

#define CAL_SERVO_KP_NUM_HAMMER_IN       1L
#define CAL_SERVO_KI_NUM_HAMMER_IN       1L
#define CAL_SERVO_KI_DIV_HAMMER_IN       8L
#define CAL_SERVO_DEADBAND_HAMMER_IN     CAL_SERVO_DEADBAND_COUNTS
#define CAL_SERVO_FINE_STEP_HAMMER_IN    CAL_SERVO_FINE_STEP
#define CAL_SERVO_RECOVERY_STEP_HAMMER_IN CAL_SERVO_RECOVERY_STEP

#define CAL_SERVO_KP_NUM_HAMMER_PGA       1L
#define CAL_SERVO_KI_NUM_HAMMER_PGA       1L
#define CAL_SERVO_KI_DIV_HAMMER_PGA       8L
#define CAL_SERVO_DEADBAND_HAMMER_PGA     CAL_SERVO_DEADBAND_COUNTS
#define CAL_SERVO_FINE_STEP_HAMMER_PGA    CAL_SERVO_FINE_STEP
#define CAL_SERVO_RECOVERY_STEP_HAMMER_PGA CAL_SERVO_RECOVERY_STEP

#define CAL_SERVO_KP_NUM_HAMMER_LP       1L
#define CAL_SERVO_KI_NUM_HAMMER_LP       1L
#define CAL_SERVO_KI_DIV_HAMMER_LP       8L
#define CAL_SERVO_DEADBAND_HAMMER_LP     CAL_SERVO_DEADBAND_COUNTS
#define CAL_SERVO_FINE_STEP_HAMMER_LP    CAL_SERVO_FINE_STEP
#define CAL_SERVO_RECOVERY_STEP_HAMMER_LP CAL_SERVO_RECOVERY_STEP

#if PSOC_HW_CLASS == PSOC_HW_GEO

#ifndef CAL_ADC_CAPTURE_CHANNEL
#define CAL_ADC_CAPTURE_CHANNEL 3u
#endif

#if !defined(VDAC_ref_PGA_DEFAULT_DATA)
    #error "AnalogGeo requiere el componente VDAC_ref_PGA."
#endif
#if !defined(VDAC_ref_BP_DEFAULT_DATA)
    #error "AnalogGeo requiere el componente VDAC_ref_BP."
#endif
#if !defined(VDAC_Ref_Adder_DEFAULT_DATA)
    #error "AnalogGeo requiere el componente VDAC_Ref_Adder."
#endif
#if !defined(VDAC_ref_LP_DEFAULT_DATA)
    #error "AnalogGeo requiere el componente VDAC_ref_LP."
#endif

static void cal_vdac_geo_pga(uint8 value)   { VDAC_ref_PGA_SetValue(value); }
static void cal_vdac_geo_bp(uint8 value)    { VDAC_ref_BP_SetValue(value); }
static void cal_vdac_geo_adder(uint8 value) { VDAC_Ref_Adder_SetValue(value); }
static void cal_vdac_geo_lp(uint8 value)    { VDAC_ref_LP_SetValue(value); }

static const PsocCalStage g_psoc_cal_stages[] = {
    { "GEO_PGA",   0u, CAL_TARGET_GEO_PGA_COUNTS,   CAL_AVG_N, CAL_SETTLE_SAMPLES_GEO_PGA,   CAL_VERIFY_SAMPLES_GEO_PGA,   CAL_MAX_ITER, CAL_TOL_COUNTS_GEO_PGA,   1, CAL_DAC_CENTER_GEO_PGA,   CAL_DAC_MAX_CHANGE_GEO_PGA,   cal_vdac_geo_pga },
    { "GEO_BP",    1u, CAL_TARGET_GEO_BP_COUNTS,    CAL_AVG_N, CAL_SETTLE_SAMPLES_GEO_BP,    CAL_VERIFY_SAMPLES_GEO_BP,    CAL_MAX_ITER, CAL_TOL_COUNTS_GEO_BP,    1, CAL_DAC_CENTER_GEO_BP,    CAL_DAC_MAX_CHANGE_GEO_BP,    cal_vdac_geo_bp },
    { "GEO_ADDER", 2u, CAL_TARGET_GEO_ADDER_COUNTS, CAL_AVG_N, CAL_SETTLE_SAMPLES_GEO_ADDER, CAL_VERIFY_SAMPLES_GEO_ADDER, CAL_MAX_ITER, CAL_TOL_COUNTS_GEO_ADDER, 1, CAL_DAC_CENTER_GEO_ADDER, CAL_DAC_MAX_CHANGE_GEO_ADDER, cal_vdac_geo_adder },
    { "GEO_LP",    3u, CAL_TARGET_GEO_LP_COUNTS,    CAL_AVG_N, CAL_SETTLE_SAMPLES_GEO_LP,    CAL_VERIFY_SAMPLES_GEO_LP,    CAL_MAX_ITER, CAL_TOL_COUNTS_GEO_LP,    1, CAL_DAC_CENTER_GEO_LP,    CAL_DAC_MAX_CHANGE_GEO_LP,    cal_vdac_geo_lp },
};

#define PSOC_CAL_STAGE_COUNT ((uint8)(sizeof(g_psoc_cal_stages) / sizeof(g_psoc_cal_stages[0])))

#else

#ifndef CAL_ADC_CAPTURE_CHANNEL
#define CAL_ADC_CAPTURE_CHANNEL 2u
#endif

#if !defined(VDAC_ref_IN_DEFAULT_DATA)
    #error "AnalogHammer requiere el componente VDAC_ref_IN."
#endif
#if !defined(VDAC_PGA_DEFAULT_DATA)
    #error "AnalogHammer requiere el componente VDAC_PGA."
#endif
#if !defined(VDAC_LP_DEFAULT_DATA)
    #error "AnalogHammer requiere el componente VDAC_LP."
#endif

#define HAMMER_VDAC_IN_START()     VDAC_ref_IN_Start()
#define HAMMER_VDAC_IN_WRITE(v)    VDAC_ref_IN_SetValue(v)

static void cal_vdac_hammer_in(uint8 value)  { HAMMER_VDAC_IN_WRITE(value); }
static void cal_vdac_hammer_pga(uint8 value) { VDAC_PGA_SetValue(value); }
static void cal_vdac_hammer_lp(uint8 value)  { VDAC_LP_SetValue(value); }

static const PsocCalStage g_psoc_cal_stages[] = {
    { "HAMMER_IN",  0u, CAL_TARGET_HAMMER_IN_COUNTS,  CAL_AVG_N, CAL_SETTLE_SAMPLES_HAMMER_IN,  CAL_VERIFY_SAMPLES_HAMMER_IN,  CAL_MAX_ITER, CAL_TOL_COUNTS, 1, CAL_DAC_CENTER_HAMMER_IN,  CAL_DAC_MAX_CHANGE_HAMMER_IN,  cal_vdac_hammer_in },
    { "HAMMER_PGA", 1u, CAL_TARGET_HAMMER_PGA_COUNTS, CAL_AVG_N, CAL_SETTLE_SAMPLES_HAMMER_PGA, CAL_VERIFY_SAMPLES_HAMMER_PGA, CAL_MAX_ITER, CAL_TOL_COUNTS, 1, CAL_DAC_CENTER_HAMMER_PGA, CAL_DAC_MAX_CHANGE_HAMMER_PGA, cal_vdac_hammer_pga },
    { "HAMMER_LP",  2u, CAL_TARGET_HAMMER_LP_COUNTS,  CAL_AVG_N, CAL_SETTLE_SAMPLES_HAMMER_LP,  CAL_VERIFY_SAMPLES_HAMMER_LP,  CAL_MAX_ITER, CAL_TOL_COUNTS, 1, CAL_DAC_CENTER_HAMMER_LP,  CAL_DAC_MAX_CHANGE_HAMMER_LP,  cal_vdac_hammer_lp },
};

#define PSOC_CAL_STAGE_COUNT ((uint8)(sizeof(g_psoc_cal_stages) / sizeof(g_psoc_cal_stages[0])))

#endif

#endif
