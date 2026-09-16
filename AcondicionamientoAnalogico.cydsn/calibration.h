#ifndef CALIBRATION_H
#define CALIBRATION_H

#include "project.h"
#include "psoc_hw.h"

#define PSOC_CAL_MAX_STAGES 4u

/* Codigo CON SIGNO: la magnitud va al IDAC y el signo a polarity_reg. */
typedef void (*PsocCalVdacWrite)(int16 value);
typedef void (*PsocCalDiagHook)(uint8 event, uint8 value);

typedef struct {
    const char *name;       /* Nombre para logs. */
    uint8 adc_channel;      /* Canal AMux_ADC que mide esta etapa. */
    int32 target_counts;    /* Objetivo ADC; en GEO normalmente 0 counts diferencial. */
    int8 direction;         /* Signo del esfuerzo PI respecto del VDAC. */
    int16 dac_center;       /* Arranque en codigos de IDAC con signo; 0 = Vref. */
    /* CUANTO PUEDE SUBIR la referencia respecto de dac_center. */
    int16 dac_max_change;
    /* Y CUANTO PUEDE BAJAR, que NO tiene por que ser lo mismo.
     *
     * Era un solo numero y el rango salia simetrico, [center-N, center+N]. Eso
     * costo caro: la curva del ADDER es util entre -178 y +128 codigos, asi que
     * con un limite simetrico habia que quedarse con 128 -el lado que ata- y
     * tirar el tramo -178..-128. Estaba escrito en la tabla, con la nota de que
     * soportar limites asimetricos seria mejor.
     *
     * Y ese tramo tirado es justo el que hace falta: medido el 2026-09-06, con
     * PGA x50 la cadena arranca contra el riel y el ADDER no la saca ni con -128
     * -su propio tap recorre 1,73 V y el del LP no se mueve nada-, mientras que
     * el procedimiento desde la PC, que SI centra la cadena, usa -191. El
     * firmware se quedaba 63 codigos corto de lo que necesitaba, por una
     * simetria que nadie habia pedido. */
    int16 dac_max_change_neg;
    PsocCalVdacWrite write; /* Funcion que escribe el IDAC fisico de la etapa. */
    /* SI ESTA ETAPA SE CALIBRA O SOLO EXISTE.
     *
     * Esta tabla cumple dos papeles que hasta el 2026-09-05 estaban colapsados:
     * es la SECUENCIA de calibracion y es tambien el MAPA DE ACTUADORES que usa
     * el instrumento de laboratorio para direccionar cada IDAC.
     *
     * Al pasar la calibracion a dos actuadores se recorto la tabla a dos filas,
     * y con eso el autotest perdio la capacidad de escribir los IDAC del PGA y
     * del BP: `set_idac 2` y `set_idac 3` empezaron a fallar EN SILENCIO, y la
     * cadena arrancaba cada ensayo desde un estado distinto. Se noto porque dos
     * corridas seguidas del mismo experimento partieron de taps casi opuestos.
     *
     * Ahora las cuatro etapas siguen existiendo -el instrumento las ve todas- y
     * este campo dice cuales recorre la calibracion. 0 = presente pero no se
     * calibra. */
    uint8 en_secuencia;
    /* SI EL LAZO USA LA CURVA MEDIDA DEL LP EN VEZ DE UNA GANANCIA CONSTANTE.
     *
     * La referencia del LP tiene una transferencia claramente no lineal -entre
     * la mitad negativa y la positiva la pendiente cambia por un factor 2,6- y
     * por eso su etapa usa la pendiente de cuerda de una tabla medida en vez de
     * una constante.
     *
     * Este campo existe porque la condicion estaba escrita como
     * `stage->adc_channel == 3u`, o sea "la etapa que mira el tap del LP". Eso
     * fue cierto mientras cada etapa miraba su propio tap. Al pasar a dos
     * actuadores el ADDER tambien mira el canal 3, y quedo tomando la curva del
     * LP como si fuera la suya: una ganancia POSITIVA en un lazo cuya ganancia
     * real es NEGATIVA. Realimentacion positiva, y el lazo se iba al riel a toda
     * velocidad. Medido: se le entrego la cadena con los cuatro taps a menos de
     * 100 mV de Vref y la dejo con el LP en el riel de arriba.
     *
     * La condicion correcta no es que tap mira sino QUE ACTUADOR MUEVE. */
    uint8 usa_curva_lp;
} PsocCalStage;

/* Resultado por etapa para telemetria post-calibracion (ver uart_send_diag
 * en main.c, eventos PSOC_EVT_CAL_STAGE_DAC / PSOC_EVT_CAL_STAGE_MEAS). */
typedef struct {
    int16 final_dac;        /* Con signo: negativo = referencia por debajo de Vref. */
    int32 final_measured;
    uint8 ok;
} PsocCalResult;

extern PsocCalResult g_psoc_cal_results[PSOC_CAL_MAX_STAGES];
extern uint8 g_psoc_cal_result_count;

void psoc_calibration_set_diag_hook(PsocCalDiagHook hook);
void psoc_calibration_start_references(void);
void psoc_calibration_restore_capture_path(void);
void psoc_calibration_reset_references(void);
void psoc_calibration_seed_default_dac(void);

/* Aplica dac_values[count] como punto de inicio de calibración: escribe cada
 * IDAC al hardware y puebla g_psoc_cal_results. Llamar después de
 * psoc_calibration_start_references() para arrancar desde los valores
 * guardados en EEPROM en vez de los defaults.
 *
 * Los códigos son CON SIGNO: negativo = referencia por debajo de Vref, y el 0
 * es Vref exacto, que es de donde conviene arrancar a calibrar. La conversión
 * a los bytes del slot de EEPROM (magnitud + máscara de signos) la hace
 * psoc_nv.c, que es quien conoce el layout de la fila. */
void psoc_calibration_seed_dac(const int16 *dac_values, uint8 count);
/* Ajuste manual de una referencia. Actualiza el codigo aplicado y marca esa
 * etapa como no verificada; una calibracion posterior vuelve a determinarla. */
uint8 psoc_calibration_set_stage_dac(uint8 stage_index, int16 dac);
/* Acepta los códigos actuales como resultado de un controlador externo. El
 * llamador debe haber verificado todos los taps; esto sólo habilita el gate de
 * EEPROM que, correctamente, queda cerrado después de cada ajuste manual. */
uint8 psoc_calibration_accept_external_result(void);
void psoc_calibration_report_adc_snapshot(void);

/* Etapas reales de la cascada activa (2 en HAMMER, 3-4 en GEO segun
 * VDAC_ref_BP). main.c no incluye calibration_tables.h (donde vive
 * PSOC_CAL_STAGE_COUNT), así que lo necesita vía este accessor — p.ej. para
 * el gate de PSOC_CMD_SAVE_EEPROM contra el conteo real de etapas, no contra
 * el tope fijo PSOC_NV_CAL_STAGES del layout EEPROM. */
uint8 psoc_calibration_stage_count(void);

/* Retorna: 0=no arranco, 1=calibracion completa en curso,
 * 2=verificacion previa OK; no hizo falta mover DACs. */
uint8 psoc_calibration_start_async(void);
uint8 psoc_calibration_service_async(void);
uint8 psoc_calibration_async_busy(void);
uint8 psoc_calibration_async_result_ok(void);

/* --- Tau de la planta, ajustable en tiempo de ejecucion ---------------------
 * Existen porque tau depende de la temperatura y el nodo lo mide solo, en vez
 * de corregir por temperatura. Ver el bloque en calibration.c. */
uint32 psoc_cal_plant_settle_samples(void);
uint16 psoc_cal_get_tau_ms(void);
/* Devuelve 0 si el valor esta fuera de 1000..60000 ms y no lo aplica. */
uint8  psoc_cal_set_tau_ms(uint16 tau_ms);
uint16 psoc_cal_get_plant_tau_x10(void);
/* Espera despues de CADA paso del lazo, en decimas de tau. Es la otra espera y
 * no la misma: sin ella el lazo mide antes de que la cadena conteste. */
uint32 psoc_cal_step_settle_samples(void);
uint16 psoc_cal_get_step_tau_x10(void);
uint8  psoc_cal_set_step_tau_x10(uint16 x10);
/* Multiplicador de tau en decimas. 0 = sin espera. Devuelve 0 si supera 100. */
uint8  psoc_cal_set_plant_tau_x10(uint16 x10);
/* El servo lento se borro. Su unica API que main.c todavia necesitaba era
 * abortar una corrida en curso, que ya la cubre el PI asincrono. */

#if defined(PSOC_TEST) && PSOC_TEST
int32 psoc_selftest_counts_to_uv(int32 counts);
uint8 psoc_selftest_stage_count(void);
uint8 psoc_selftest_stage_channel(uint8 stage,uint8 *channel);
uint8 psoc_selftest_amux_channel_count(void);
uint8 psoc_selftest_write_stage_dac(uint8 stage,int16 code);
uint8 psoc_selftest_current_stage_dac(uint8 stage,int16 *code);
uint8 psoc_selftest_stage_result(uint8 stage,int16 *dac,int32 *meas,uint8 *ok);
void psoc_selftest_select_channel(uint8 channel,uint8 with_cap);
void psoc_selftest_restore(void);
uint8 psoc_selftest_measure_dc(uint8 channel,uint16 settle_ms,uint16 n,uint8 with_cap,int32 *mean,int32 *pp);
uint8 psoc_selftest_measure_series(uint8 channel,uint16 settle_ms,uint16 n,uint8 with_cap,uint16 tone_hz,int32 *mean,int32 *rms,int32 *pp,int32 *tone);
#endif
#endif
