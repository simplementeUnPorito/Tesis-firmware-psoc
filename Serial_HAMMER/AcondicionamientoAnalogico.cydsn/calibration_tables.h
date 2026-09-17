#ifndef CALIBRATION_TABLES_H
#define CALIBRATION_TABLES_H

#include "calibration.h"

/* Agregador de calibracion activo.
 *
 * La calibracion vigente es PI-only: el PI lee el DC desde el Filter de
 * hardware cargado con FIR_calibration.h, y al cerrar cada etapa puede probar
 * un unico LSB de VDAC para quedarse con el lado que mejore el error. Los
 * parametros ajustables viven por etapa en:
 *   - calibration_tables_geo_pga.h
 *   - calibration_tables_geo_bp.h
 *   - calibration_tables_geo_sum.h
 *   - calibration_tables_geo_lp.h
 *   - calibration_tables_hammer_pga.h
 *   - calibration_tables_hammer_lp.h
 *
 * La busqueda binaria historica se saco de los headers activos; queda guardada
 * en calibration_tables_legacy_bisection.h, sin incluirse desde aca. */

#ifndef CAL_TARGET_1V_COUNTS
#define CAL_TARGET_1V_COUNTS 52429L
#endif

#ifndef CAL_TARGET_1V5_COUNTS
#define CAL_TARGET_1V5_COUNTS 78644L
#endif

/* Escalas de cuantizacion usadas por el PI activo. N_*_LEVELS es la cantidad
 * de niveles (2^B), no el codigo maximo (2^B - 1). El VDAC8 tiene 256 niveles
 * separados 16 mV: span de cuantizacion 4.096 V, codigos 0..255 y salida
 * maxima realizable 4.080 V. */
#ifndef CAL_ADC_SPAN_MV
#define CAL_ADC_SPAN_MV 5000L
#endif

#ifndef CAL_ADC_LEVELS
#define CAL_ADC_LEVELS 262144L
#endif

/* Salida diferencial signed de 18 bits: -2^17 .. 2^17-1. */
#ifndef CAL_ADC_SIGNED_MIN_COUNTS
#define CAL_ADC_SIGNED_MIN_COUNTS (-(CAL_ADC_LEVELS / 2L))
#endif

#ifndef CAL_ADC_SIGNED_MAX_COUNTS
#define CAL_ADC_SIGNED_MAX_COUNTS ((CAL_ADC_LEVELS / 2L) - 1L)
#endif

#ifndef CAL_VDAC_QUANT_SPAN_MV
#define CAL_VDAC_QUANT_SPAN_MV 4096L
#endif

#ifndef CAL_VDAC_LEVELS
#define CAL_VDAC_LEVELS 256L
#endif

#ifndef CAL_VDAC8_MV_PER_LSB
#define CAL_VDAC8_MV_PER_LSB 16L
#endif

/* Definir esta macro (o pasarla por flag de compilador) fuerza Delta_i=1 en
 * todas las etapas, saltandose el margen de banda muerta de abajo. Uso: test
 * A1 de la revision (distinguir banda-muerta vs velocidad de convergencia).
 * Dejar comentado en builds normales. */
/* #define CAL_PI_FORCE_MIN_DEADBAND */

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

#ifndef CAL_PI_INTEGRAL_LIMIT
#define CAL_PI_INTEGRAL_LIMIT 8000000L
#endif

#ifndef CAL_PI_LOCK_N_MAX
#define CAL_PI_LOCK_N_MAX 4096u
#endif

#ifndef CAL_PI_DEFAULT_TIMEOUT_SAMPLES
#define CAL_PI_DEFAULT_TIMEOUT_SAMPLES 30000u
#endif

/* 10 ms/tick; se deja alto para que el timeout real lo maneje el PI por etapa
 * y no el watchdog global del firmware. */
#ifndef CAL_WATCHDOG_TICKS
#define CAL_WATCHDOG_TICKS 40000UL
#endif

#include "calibration_tables_hammer_pga.h"
#include "calibration_tables_hammer_lp.h"
#include "calibration_tables_geo_pga.h"
#include "calibration_tables_geo_bp.h"
#include "calibration_tables_geo_sum.h"
#include "calibration_tables_geo_lp.h"

#if PSOC_HW_CLASS == PSOC_HW_GEO

#ifndef CAL_ADC_CAPTURE_CHANNEL
#define CAL_ADC_CAPTURE_CHANNEL 3u
#endif

#if !defined(VDAC_ref_PGA_DEFAULT_DATA)
    #error "AnalogGeo requiere el componente VDAC_ref_PGA."
#endif
#if !defined(VDAC_Ref_Sum_DEFAULT_DATA)
    #error "AnalogGeo requiere el componente VDAC_Ref_Sum."
#endif
#if !defined(VDAC_ref_LP_DEFAULT_DATA)
    #error "AnalogGeo requiere el componente VDAC_ref_LP."
#endif

static void cal_vdac_geo_pga(uint8 value)   { VDAC_ref_PGA_SetValue(value); }
#if defined(VDAC_ref_BP_DEFAULT_DATA) || defined(CY_DVDAC_VDAC_ref_BP_H)
static void cal_vdac_geo_bp(uint8 value)    { VDAC_ref_BP_SetValue(value); }
#endif
static void cal_vdac_geo_sum(uint8 value)   { VDAC_Ref_Sum_SetValue(value); }
static void cal_vdac_geo_lp(uint8 value)    { VDAC_ref_LP_SetValue(value); }

static const PsocCalStage g_psoc_cal_stages[] = {
    { "GEO_PGA",   0u, CAL_TARGET_COUNTS_GEO_PGA,   CAL_DIRECTION_GEO_PGA,   CAL_DAC_CENTER_GEO_PGA,   CAL_DAC_MAX_CHANGE_GEO_PGA,   cal_vdac_geo_pga },
#if defined(VDAC_ref_BP_DEFAULT_DATA) || defined(CY_DVDAC_VDAC_ref_BP_H)
    { "GEO_BP",    1u, CAL_TARGET_COUNTS_GEO_BP,    CAL_DIRECTION_GEO_BP,    CAL_DAC_CENTER_GEO_BP,    CAL_DAC_MAX_CHANGE_GEO_BP,    cal_vdac_geo_bp },
#endif
    { "GEO_SUM",   2u, CAL_TARGET_COUNTS_GEO_SUM,   CAL_DIRECTION_GEO_SUM,   CAL_DAC_CENTER_GEO_SUM,   CAL_DAC_MAX_CHANGE_GEO_SUM,   cal_vdac_geo_sum },
    { "GEO_LP",    3u, CAL_TARGET_COUNTS_GEO_LP,    CAL_DIRECTION_GEO_LP,    CAL_DAC_CENTER_GEO_LP,    CAL_DAC_MAX_CHANGE_GEO_LP,    cal_vdac_geo_lp },
};

#define PSOC_CAL_STAGE_COUNT ((uint8)(sizeof(g_psoc_cal_stages) / sizeof(g_psoc_cal_stages[0])))

#else

#ifndef CAL_ADC_CAPTURE_CHANNEL
#define CAL_ADC_CAPTURE_CHANNEL 1u
#endif

#if !defined(VDAC_PGA_DEFAULT_DATA)
    #error "AnalogHammer requiere el componente VDAC_PGA."
#endif
#if !defined(VDAC_LP_DEFAULT_DATA)
    #error "AnalogHammer requiere el componente VDAC_LP."
#endif

static void cal_vdac_hammer_pga(uint8 value) { VDAC_PGA_SetValue(value); }
static void cal_vdac_hammer_lp(uint8 value)  { VDAC_LP_SetValue(value); }

static const PsocCalStage g_psoc_cal_stages[] = {
    { "HAMMER_PGA", 0u, CAL_TARGET_HAMMER_PGA_COUNTS, CAL_DIRECTION_HAMMER_PGA, CAL_DAC_CENTER_HAMMER_PGA, CAL_DAC_MAX_CHANGE_HAMMER_PGA, cal_vdac_hammer_pga },
    { "HAMMER_LP",  1u, CAL_TARGET_HAMMER_LP_COUNTS,  CAL_DIRECTION_HAMMER_LP,  CAL_DAC_CENTER_HAMMER_LP,  CAL_DAC_MAX_CHANGE_HAMMER_LP,  cal_vdac_hammer_lp },
};

#define PSOC_CAL_STAGE_COUNT ((uint8)(sizeof(g_psoc_cal_stages) / sizeof(g_psoc_cal_stages[0])))

#endif

#endif
