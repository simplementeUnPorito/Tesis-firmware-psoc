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
#define CAL_MAX_ITER   12u
#define CAL_TOL_COUNTS 250L

#define CAL_TOL_COUNTS_GEO_PGA   250L
#define CAL_TOL_COUNTS_GEO_BP    250L
#define CAL_TOL_COUNTS_GEO_ADDER 250L
#define CAL_TOL_COUNTS_GEO_LP    250L

/* Rango operativo absoluto: ADC_CFG1_COUNTS_PER_VOLT=52429 (ver HANDOFF
 * §4/§13), entonces 0.5V =~ 26214 counts. Una etapa cuya mejor medicion
 * (best_measured) quede mas lejos de 0 que esto NO se deja calibrada con ese
 * DAC aunque "ok" hubiera sido 1: se prefiere volver a CAL_DAC_INIT
 * (~2.5V, sin calibrar pero en rango) a dejar el operacional fuera de rango
 * (riesgo de saturacion en cascada). Prioridad maxima: la ultima etapa
 * (GEO_LP, la que alimenta al canal de captura del ADC) SIEMPRE debe quedar
 * dentro de este rango, o la captura es inutil (ver PSOC_EVT_CAL_LP_BAD). */
#define CAL_OPERATING_RANGE_COUNTS 26214L

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
    { "GEO_PGA",   0u, CAL_TARGET_GEO_PGA_COUNTS,   CAL_AVG_N, CAL_SETTLE_SAMPLES_GEO_PGA,   CAL_VERIFY_SAMPLES_GEO_PGA,   CAL_MAX_ITER, CAL_TOL_COUNTS_GEO_PGA,   1, cal_vdac_geo_pga },
    { "GEO_BP",    1u, CAL_TARGET_GEO_BP_COUNTS,    CAL_AVG_N, CAL_SETTLE_SAMPLES_GEO_BP,    CAL_VERIFY_SAMPLES_GEO_BP,    CAL_MAX_ITER, CAL_TOL_COUNTS_GEO_BP,    1, cal_vdac_geo_bp },
    { "GEO_ADDER", 2u, CAL_TARGET_GEO_ADDER_COUNTS, CAL_AVG_N, CAL_SETTLE_SAMPLES_GEO_ADDER, CAL_VERIFY_SAMPLES_GEO_ADDER, CAL_MAX_ITER, CAL_TOL_COUNTS_GEO_ADDER, 1, cal_vdac_geo_adder },
    { "GEO_LP",    3u, CAL_TARGET_GEO_LP_COUNTS,    CAL_AVG_N, CAL_SETTLE_SAMPLES_GEO_LP,    CAL_VERIFY_SAMPLES_GEO_LP,    CAL_MAX_ITER, CAL_TOL_COUNTS_GEO_LP,    1, cal_vdac_geo_lp },
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
    { "HAMMER_IN",  0u, CAL_TARGET_HAMMER_IN_COUNTS,  CAL_AVG_N, CAL_SETTLE_SAMPLES_HAMMER_IN,  CAL_VERIFY_SAMPLES_HAMMER_IN,  CAL_MAX_ITER, CAL_TOL_COUNTS, 1, cal_vdac_hammer_in },
    { "HAMMER_PGA", 1u, CAL_TARGET_HAMMER_PGA_COUNTS, CAL_AVG_N, CAL_SETTLE_SAMPLES_HAMMER_PGA, CAL_VERIFY_SAMPLES_HAMMER_PGA, CAL_MAX_ITER, CAL_TOL_COUNTS, 1, cal_vdac_hammer_pga },
    { "HAMMER_LP",  2u, CAL_TARGET_HAMMER_LP_COUNTS,  CAL_AVG_N, CAL_SETTLE_SAMPLES_HAMMER_LP,  CAL_VERIFY_SAMPLES_HAMMER_LP,  CAL_MAX_ITER, CAL_TOL_COUNTS, 1, cal_vdac_hammer_lp },
};

#define PSOC_CAL_STAGE_COUNT ((uint8)(sizeof(g_psoc_cal_stages) / sizeof(g_psoc_cal_stages[0])))

#endif

#endif
