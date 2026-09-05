#ifndef CALIBRATION_TABLES_H
#define CALIBRATION_TABLES_H

#include "calibration.h"
#include "filter_coeffs.h"
/* PSOC_ADC_NATIVE_FS_HZ: la espera de planta se pide en milisegundos y se
 * convierte a muestras, asi que hace falta la Fs. */
#include "psoc_adc.h"

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

#ifndef CAL_PI_DEADBAND_MIN_COUNTS
#define CAL_PI_DEADBAND_MIN_COUNTS 1L
#endif

/* El FIR de calibracion tiene ganancia DC uno y 128 taps. Tras cambiar AMux o
 * DAC hacen falta exactamente N muestras nuevas antes de realimentar otra
 * correccion. Esperar 512/2048 muestras solo repetia informacion ya asentada;
 * corregir antes de N mezcla dos codigos distintos y vuelve oscilatorio al PI. */
#ifndef CAL_PI_FIR_SETTLE_SAMPLES
#define CAL_PI_FIR_SETTLE_SAMPLES FILTER_FIR_NTAPS
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

/* -------------------------------------------------------------------------
 * ESPERA DE LA PLANTA. Concepto nuevo, no es un ajuste de CAL_PI_SETTLE_*.
 *
 * CAL_PI_SETTLE_SAMPLES_* vacia el FIR y son 128 muestras: ese numero siempre
 * estuvo BIEN. Lo que faltaba es esperar a que la CADENA ANALOGICA se asiente
 * despues de que una etapa de aguas arriba movio su referencia.
 *
 * El polo lento tiene nombre: C1 = 680 uF contra R4 = 43 k en la entrada del
 * pasabanda. tau = 29,2 s del esquematico, 31,3 s medidos en la placa el
 * 2026-09-03 (docs/hardware_recuperacion_saturacion_2026-09-03.txt). A 2604 Hz
 * eso son ~81.500 muestras, que es por lo que los campos tuvieron que pasar de
 * uint16 a uint32: en 16 bits no entra ni un tau. */
/* --- LOS DOS NUMEROS QUE ELIAS PIDIO PODER CAMBIAR, LOS DOS uint16 ---------
 *
 * Pedido textual del 2026-09-05: "Deja el codigo para que pueda cambiar la tau
 * como un define un unsigned uint16, pero por ahora usa 2 tau".
 *
 * Por eso tau se expresa en MILISEGUNDOS y no en muestras: 29.500 ms entra
 * comodo en uint16 (techo 65.535), mientras que en muestras son 76.818 y no
 * entra. Las muestras se calculan en uint32 en tiempo de ejecucion, que es
 * donde tienen que estar. */

#ifndef CAL_PI_TAU_MS
/* 29,5 s MEDIDOS. El tau salio de TRES caminos independientes que coinciden:
 *   R4*C1 = 43k x 680 uF = 29,2 s del esquematico;
 *   31,3 s del decaimiento tras saturar (2026-09-03);
 *   26,4 a 33,4 s, media 29,5 s, en los seis pares (etapa, tap) de la matriz de
 *   acople (2026-09-04).
 * Se usa el de la matriz por ser el mas directo. Ver docs/MEDICIONES_2026-09-04.md
 *
 * OJO: es el tau de BANCO, a ~19 C. C1 es un electrolitico y en campo la
 * temperatura va de 8 a 32 C en la misma semana (medido con Open-Meteo el
 * 2026-09-05), asi que este numero NO sirve como constante fija de campo. Por
 * eso existe la automedicion: el nodo mide su propio tau y pisa esta variable.
 * La decision de Elias fue explicita: NO corregir por temperatura, sino medir
 * tau y ajustar con lo medido. */
#define CAL_PI_TAU_MS 29500u
#endif

#ifndef CAL_PI_PLANT_SETTLE_TAU_X10
/* Multiplicador de tau, en decimas, para poder pedir 0,5 tau o 2,5 tau sin
 * flotante. CERO = sin espera de planta, que era el comportamiento hasta el
 * 2026-09-04.
 *
 * EN 20 (= 2 tau) POR DECISION DE ELIAS del 2026-09-05: "2tau y 20mV es lo
 * maximo que aceptamos pero si llegamos a mejores rangos maravilloso". O sea
 * que 2 tau es el TECHO del presupuesto de tiempo, no un objetivo; si una
 * estrategia consigue el error pedido con menos, mejor.
 *
 * Lo que da el modelo ya calibrado contra la placa, con el lazo secuencial:
 *     sin esperar  17,5 mV en 2 s      <- reproduce los -18 mV observados
 *     1 tau         2,09 mV en 119 s
 *     2 tau         0,47 mV en 237 s
 * De 2 tau en adelante no mejora. */
#define CAL_PI_PLANT_SETTLE_TAU_X10 20u
#endif

/* Muestras que dura un tau, derivadas del tau en ms. En uint32 porque a 2604 Hz
 * un tau son ~76.818 muestras y en uint16 no entra: ese fue exactamente el bug
 * silencioso que se arreglo el 2026-09-04. */
#define CAL_PI_TAU_SAMPLES_FROM_MS(ms)     (((uint32)(ms) * (uint32)PSOC_ADC_NATIVE_FS_HZ) / 1000UL)

#ifndef CAL_PI_TAU_SAMPLES
#define CAL_PI_TAU_SAMPLES CAL_PI_TAU_SAMPLES_FROM_MS(CAL_PI_TAU_MS)
#endif

#ifndef CAL_PI_PLANT_SETTLE_SAMPLES_DEFAULT
/* Se divide ANTES de multiplicar para no desbordar: 76.818/10 x 20 = 153.636.
 * Al reves serian 1,5 millones, que entra igual, pero la forma dividida es la
 * que sigue entrando si alguien sube el multiplicador a 10 tau. */
#define CAL_PI_PLANT_SETTLE_SAMPLES_DEFAULT     ((CAL_PI_TAU_SAMPLES / 10UL) * (uint32)CAL_PI_PLANT_SETTLE_TAU_X10)
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
