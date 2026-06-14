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

/* Punto de partida obligatorio de la busqueda binaria: 0x9C = 156 ->
 * 156 * 16mV (VDAC8 1x) = 2.496V =~ 2.5V (centro de rango / "tierra
 * virtual" del front-end analogico). */
#define CAL_DAC_INIT   0x9Cu

/* Primer paso (antes de la busqueda binaria) para detectar empiricamente si
 * la medida crece o decrece con el codigo DAC. */
#define CAL_PROBE_STEP 32u

/* "Estabilizacion": stable_avg() repite avg_counts() hasta que dos ventanas
 * consecutivas difieran <= CAL_SETTLE_TOL_COUNTS (ruido de cuantizacion),
 * o hasta CAL_SETTLE_MAX_WINDOWS intentos (usa la ultima ventana). */
#define CAL_SETTLE_MAX_WINDOWS 10u
#define CAL_SETTLE_TOL_COUNTS  100L

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
    { "GEO_PGA",   0u, CAL_TARGET_GEO_PGA_COUNTS,   CAL_AVG_N, CAL_MAX_ITER, CAL_TOL_COUNTS, 1, cal_vdac_geo_pga },
    { "GEO_BP",    1u, CAL_TARGET_GEO_BP_COUNTS,    CAL_AVG_N, CAL_MAX_ITER, CAL_TOL_COUNTS, 1, cal_vdac_geo_bp },
    { "GEO_ADDER", 2u, CAL_TARGET_GEO_ADDER_COUNTS, CAL_AVG_N, CAL_MAX_ITER, CAL_TOL_COUNTS, 1, cal_vdac_geo_adder },
    { "GEO_LP",    3u, CAL_TARGET_GEO_LP_COUNTS,    CAL_AVG_N, CAL_MAX_ITER, CAL_TOL_COUNTS, 1, cal_vdac_geo_lp },
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
    { "HAMMER_IN",  0u, CAL_TARGET_HAMMER_IN_COUNTS,  CAL_AVG_N, CAL_MAX_ITER, CAL_TOL_COUNTS, 1, cal_vdac_hammer_in },
    { "HAMMER_PGA", 1u, CAL_TARGET_HAMMER_PGA_COUNTS, CAL_AVG_N, CAL_MAX_ITER, CAL_TOL_COUNTS, 1, cal_vdac_hammer_pga },
    { "HAMMER_LP",  2u, CAL_TARGET_HAMMER_LP_COUNTS,  CAL_AVG_N, CAL_MAX_ITER, CAL_TOL_COUNTS, 1, cal_vdac_hammer_lp },
};

#define PSOC_CAL_STAGE_COUNT ((uint8)(sizeof(g_psoc_cal_stages) / sizeof(g_psoc_cal_stages[0])))

#endif

#endif
