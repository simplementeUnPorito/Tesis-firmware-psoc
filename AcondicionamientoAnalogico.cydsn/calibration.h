#ifndef CALIBRATION_H
#define CALIBRATION_H

#include "project.h"
#include "psoc_hw.h"

#define PSOC_CAL_MAX_STAGES 4u

/* Factor global para convertir los tiempos simulados de asentamiento a
 * muestras descartadas. CAL_SETTLE_FACTOR es el numerador y
 * CAL_SETTLE_FACTOR_DEN el denominador: 3/1 = 3x, 2/1 = 2x, 3/2 = 1.5x.
 * Mas alto = mas seguro y lento; mas bajo = mas rapido y sensible a transitorios. */
#ifndef CAL_SETTLE_FACTOR
#define CAL_SETTLE_FACTOR 3u
#endif

#ifndef CAL_SETTLE_FACTOR_DEN
#define CAL_SETTLE_FACTOR_DEN 1u
#endif

typedef void (*PsocCalVdacWrite)(uint8 value);
typedef void (*PsocCalDiagHook)(uint8 event, uint8 value);

/* Configuracion de promediado por ventana deslizante (ver calibration.c,
 * async_measure_service). "measured" = promedio de las ultimas
 * avg_n*window_count muestras (peso constante, no se diluye). Se considera
 * "estable" cuando ese promedio varia <= settle_tol_counts durante
 * stable_streak comparaciones consecutivas con el buffer ya lleno; si se
 * llega a max_samples sin racha estable, se usa el promedio igual. */
typedef struct {
    uint8  avg_n;             /* Muestras por ventana parcial; mas bajo = responde mas rapido, mas ruido. */
    uint8  window_count;      /* Cantidad de ventanas en el promedio; piso total = avg_n*window_count. */
    uint16 max_samples;       /* Techo duro de muestras; si no estabiliza antes, usa el promedio actual. */
    int32  settle_tol_counts; /* Diferencia maxima entre promedios para contar una ventana como estable. */
    uint8  stable_streak;     /* Ventanas estables consecutivas requeridas; mas bajo = mas rapido. */
} PsocCalAvgCfg;

/* Fase final con la señal real del geofono (entrada fija, sin selector):
 * ajuste fino de +-1 LSB sobre el resultado de biseccion/verify, hasta
 * max_nudges veces, revirtiendo si un nudge satura o empeora |error|.
 * enable=0 salta la etapa (se conserva el resultado de verify tal cual). */
typedef struct {
    uint8  enable;                /* 1 ejecuta realcheck con entrada real; 0 conserva el resultado de verify. */
    int32  tol_counts;            /* Error aceptable en realcheck; mismo concepto que CAL_TOL_COUNTS_GEO_*. */
    uint8  nudge_step;            /* Paso de ajuste fino en codigos VDAC, normalmente 1 LSB. */
    uint8  max_nudges;            /* Maximo de pasos finos; mas alto = mas lento pero puede rescatar el cero. */
    uint16 discard_samples;       /* Muestras a tirar al entrar al realcheck/cambiar AMux. */
    uint16 nudge_discard_samples; /* Muestras a tirar despues de cada nudge. */
    PsocCalAvgCfg avg;            /* Promediado usado durante realcheck. */
} PsocCalRealcheckCfg;

typedef struct {
    const char *name;             /* Nombre para logs. */
    uint8 adc_channel;            /* Canal AMux_ADC que mide esta etapa durante biseccion/verify. */
    int32 target_counts;          /* Objetivo ADC; en GEO normalmente 0 counts diferencial. */
    int8 direction;               /* Sentido inicial esperado; el probe mide y corrige el sentido real. */
    uint8 dac_center;             /* Codigo VDAC inicial/centro de busqueda. */
    uint8 dac_max_change;         /* Rango de busqueda: [center-max_change, center+max_change]. */
    uint8 probe_step;             /* Primer salto para detectar pendiente antes de biseccionar. */
    uint16 max_iter;              /* Tope de iteraciones de biseccion. */
    int32 tolerance_counts;       /* TOL: error que define "suficientemente calibrado". */
    int32 deadband_counts;        /* DEADBAND: zona donde se deja de mover para no perseguir ruido/LSB. */
    int32 sat_counts;             /* Umbral de saturacion/riel real para descartar candidatos. */
    uint16 settle_samples;        /* Descarte tras escribir VDAC en la fase de busqueda. */
    uint16 verify_settle_samples; /* Descarte tras escribir VDAC en la fase verify. */
    PsocCalAvgCfg avg;            /* Promediado usado durante biseccion. */
    PsocCalAvgCfg verify_avg;     /* Promediado usado durante verify. */
    PsocCalRealcheckCfg realcheck;/* Ajuste final con entrada real. */
    PsocCalVdacWrite write;       /* Funcion que escribe el VDAC fisico de la etapa. */
} PsocCalStage;

/* Resultado por etapa para telemetria post-calibracion (ver uart_send_diag
 * en main.c, eventos PSOC_EVT_CAL_STAGE_DAC / PSOC_EVT_CAL_STAGE_MEAS). */
typedef struct {
    uint8 final_dac;
    int32 final_measured;
    uint8 ok;
} PsocCalResult;

extern PsocCalResult g_psoc_cal_results[PSOC_CAL_MAX_STAGES];
extern uint8 g_psoc_cal_result_count;

void psoc_calibration_set_diag_hook(PsocCalDiagHook hook);
void psoc_calibration_start_references(void);
void psoc_calibration_restore_capture_path(void);
void psoc_calibration_reset_references(void);

/* Aplica dac_values[count] como punto de inicio de calibración:
 * escribe cada DAC al hardware y puebla g_psoc_cal_results.
 * Llamar después de psoc_calibration_start_references() para arrancar
 * desde los valores guardados en EEPROM en vez de los defaults. */
void psoc_calibration_seed_dac(const uint8 *dac_values, uint8 count);
void psoc_calibration_report_adc_snapshot(void);

/* Etapas reales de la cascada activa (2 en HAMMER, 3-4 en GEO segun
 * VDAC_ref_BP). main.c no incluye calibration_tables.h (donde vive
 * PSOC_CAL_STAGE_COUNT), así que lo necesita vía este accessor — p.ej. para
 * el gate de PSOC_CMD_SAVE_EEPROM contra el conteo real de etapas, no contra
 * el tope fijo PSOC_NV_CAL_STAGES del layout EEPROM. */
uint8 psoc_calibration_stage_count(void);

uint8 psoc_calibration_start_async(void);
uint8 psoc_calibration_service_async(void);
uint8 psoc_calibration_async_busy(void);
uint8 psoc_calibration_async_result_ok(void);
void psoc_calibration_servo_enable(uint8 enable);
uint8 psoc_calibration_servo_enabled(void);
void psoc_calibration_servo_abort(void);
uint8 psoc_calibration_servo_service(void);

#endif
