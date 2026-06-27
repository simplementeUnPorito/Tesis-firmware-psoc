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
 *   - calibration_tables_geo_adder.h
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
#include "calibration_tables_geo_adder.h"
#include "calibration_tables_geo_lp.h"

#if PSOC_HW_CLASS == PSOC_HW_GEO

#ifndef CAL_ADC_CAPTURE_CHANNEL
#define CAL_ADC_CAPTURE_CHANNEL 3u
#endif

#if !defined(VDAC_ref_PGA_DEFAULT_DATA)
    #error "AnalogGeo requiere el componente VDAC_ref_PGA."
#endif
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
    { "GEO_PGA",   0u, CAL_TARGET_COUNTS_GEO_PGA,   CAL_DIRECTION_GEO_PGA,   CAL_DAC_CENTER_GEO_PGA,   CAL_DAC_MAX_CHANGE_GEO_PGA,   cal_vdac_geo_pga },
#if defined(VDAC_ref_BP_DEFAULT_DATA) || defined(CY_DVDAC_VDAC_ref_BP_H)
    { "GEO_BP",    1u, CAL_TARGET_COUNTS_GEO_BP,    CAL_DIRECTION_GEO_BP,    CAL_DAC_CENTER_GEO_BP,    CAL_DAC_MAX_CHANGE_GEO_BP,    cal_vdac_geo_bp },
#endif
    { "GEO_ADDER", 2u, CAL_TARGET_COUNTS_GEO_ADDER, CAL_DIRECTION_GEO_ADDER, CAL_DAC_CENTER_GEO_ADDER, CAL_DAC_MAX_CHANGE_GEO_ADDER, cal_vdac_geo_adder },
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
