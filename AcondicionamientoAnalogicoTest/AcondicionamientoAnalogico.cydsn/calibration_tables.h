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
 * La busqueda binaria y el servo lento se borraron: el PI es el unico
 * algoritmo. */

#ifndef CAL_TARGET_1V_COUNTS
#define CAL_TARGET_1V_COUNTS 52429L
#endif

#ifndef CAL_TARGET_1V5_COUNTS
#define CAL_TARGET_1V5_COUNTS 78644L
#endif

/* ==========================================================================
 * Conversiones entre dominios. TODO EN ENTEROS.
 * --------------------------------------------------------------------------
 * Hay tres dominios distintos y confundirlos es el error clasico: 127 codigos
 * de IDAC no son 127 counts de ADC ni 127 uV. Las conversiones se hacen SIEMPRE
 * con estas macros y en int64, nunca con un cociente precalculado: uV por count
 * da 19,073 y redondearlo a 19 mete un 0,4 % de error sistematico en el lazo.
 *
 *   codigo de IDAC  ->  uV en la referencia   : x CAL_IDAC_UV_PER_LSB
 *   count de ADC    ->  uV en el tap          : x CAL_ADC_SPAN_UV / CAL_ADC_LEVELS
 *   count de ADC    ->  codigo de IDAC        : las dos de arriba encadenadas
 *
 * Numeros de ESTA placa, medidos, no del datasheet de la portadora JitX:
 *   IDAC8 en 0..31,875 uA con 1/8 uA por bit sobre R = 15 kOhm
 *   -> 125 nA x 15 kOhm = 1875 uV por codigo.
 *   Con polaridad, el rango util es +-255 codigos = +-478 mV de autoridad.
 * ========================================================================== */
#ifndef CAL_IDAC_UV_PER_LSB
#define CAL_IDAC_UV_PER_LSB 1875L
#endif

/* Span del ADC en uV. Se guarda en uV y no en mV para no perder resolucion al
 * dividir por los 262144 niveles. */
#ifndef CAL_ADC_SPAN_UV
#define CAL_ADC_SPAN_UV 5000000L
#endif

#ifndef CAL_ADC_LEVELS
#define CAL_ADC_LEVELS 262144L
#endif

/* Denominador de la conversion count -> codigo de IDAC. Se deja como macro para
 * que quede a la vista que son las dos conversiones encadenadas y no un numero
 * magico: 262144 x 1875 = 491.520.000, o sea 98,304 counts por codigo. */
#define CAL_COUNTS_PER_IDAC_CODE_NUM (CAL_ADC_LEVELS * CAL_IDAC_UV_PER_LSB)
#define CAL_COUNTS_PER_IDAC_CODE_DEN (CAL_ADC_SPAN_UV)

/* Cuantos counts vale un milivoltio en el tap: para los objetivos de las
 * etapas, que se escriben en mV porque asi se piensan. */
#define CAL_COUNTS_PER_MV ((CAL_ADC_LEVELS * 1000L) / CAL_ADC_SPAN_UV)

/* Salida diferencial signed de 18 bits: -2^17 .. 2^17-1. */
#ifndef CAL_ADC_SIGNED_MIN_COUNTS
#define CAL_ADC_SIGNED_MIN_COUNTS (-(CAL_ADC_LEVELS / 2L))
#endif

#ifndef CAL_ADC_SIGNED_MAX_COUNTS
#define CAL_ADC_SIGNED_MAX_COUNTS ((CAL_ADC_LEVELS / 2L) - 1L)
#endif

/* Rango con signo del IDAC. Sale de psoc_hw.h para que haya una sola verdad. */
#define CAL_IDAC_SIGNED_MAX ((int16)PSOC_IDAC_SIGNED_MAX)

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

/* Limitador de pendiente del PI, en codigos por muestra. CERO = sin limite,
 * que es el valor correcto y el que se usa.
 *
 * Estaba en 1, y eso no era una proteccion sino un bug: con un solo codigo por
 * muestra el PI nunca aplicaba su propia ley de control, la dominaba el
 * limitador. Una etapa que necesitaba moverse 84 codigos tardaba 84 muestras
 * como piso aunque el PI pidiera llegar de una, y encima la integral seguia
 * cargando mientras tanto. Se deja la macro por si alguna etapa alguna vez
 * necesita limitar la pendiente de verdad, pero el default es no limitar. */
#ifndef CAL_PI_MAX_DAC_STEP_PER_SAMPLE
#define CAL_PI_MAX_DAC_STEP_PER_SAMPLE 0u
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

/* Los componentes se llaman VDAC_* y esta bien: el bloque es un IDAC8, pero
 * con la resistencia a Vref forma un DAC de TENSION hecho a mano. Eso es lo que
 * ve la etapa analogica. Lo que cambia respecto de un VDAC8 de verdad es el
 * escalon (1875 uV en vez de 16 mV) y que el signo lo da polarity_reg en vez
 * del propio codigo. El orden signo-antes-que-magnitud lo garantiza
 * psoc_hw_idac_apply_polarity(). */
static void cal_vdac_geo_pga(int16 value)
{
    VDAC_ref_PGA_SetValue(psoc_hw_idac_apply_polarity(0u, value));
}
#if defined(VDAC_ref_BP_DEFAULT_DATA) || defined(CY_DVDAC_VDAC_ref_BP_H)
static void cal_vdac_geo_bp(int16 value)
{
    VDAC_ref_BP_SetValue(psoc_hw_idac_apply_polarity(1u, value));
}
#endif
static void cal_vdac_geo_sum(int16 value)
{
    VDAC_Ref_Sum_SetValue(psoc_hw_idac_apply_polarity(2u, value));
}
static void cal_vdac_geo_lp(int16 value)
{
    VDAC_ref_LP_SetValue(psoc_hw_idac_apply_polarity(3u, value));
}

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

static void cal_vdac_hammer_pga(int16 value)
{
    VDAC_PGA_SetValue(psoc_hw_idac_apply_polarity(0u, value));
}
static void cal_vdac_hammer_lp(int16 value)
{
    VDAC_LP_SetValue(psoc_hw_idac_apply_polarity(1u, value));
}

static const PsocCalStage g_psoc_cal_stages[] = {
    { "HAMMER_PGA", 0u, CAL_TARGET_HAMMER_PGA_COUNTS, CAL_DIRECTION_HAMMER_PGA, CAL_DAC_CENTER_HAMMER_PGA, CAL_DAC_MAX_CHANGE_HAMMER_PGA, cal_vdac_hammer_pga },
    { "HAMMER_LP",  1u, CAL_TARGET_HAMMER_LP_COUNTS,  CAL_DIRECTION_HAMMER_LP,  CAL_DAC_CENTER_HAMMER_LP,  CAL_DAC_MAX_CHANGE_HAMMER_LP,  cal_vdac_hammer_lp },
};

#define PSOC_CAL_STAGE_COUNT ((uint8)(sizeof(g_psoc_cal_stages) / sizeof(g_psoc_cal_stages[0])))

#endif

#endif
