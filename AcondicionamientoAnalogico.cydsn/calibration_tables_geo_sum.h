#ifndef CALIBRATION_TABLES_GEO_SUM_H
#define CALIBRATION_TABLES_GEO_SUM_H

/* GEO_SUM: VDAC_Ref_Sum, AMux_ADC=2. */

#ifndef CAL_TARGET_GEO_SUM_MV
#define CAL_TARGET_GEO_SUM_MV 0L
#endif
#define CAL_TARGET_COUNTS_GEO_SUM (CAL_TARGET_GEO_SUM_MV * CAL_TARGET_1V_COUNTS / 1000L)

#define CAL_DIRECTION_GEO_SUM 1

#ifndef CAL_ADELANTO_GEO_SUM_MV
#define CAL_ADELANTO_GEO_SUM_MV 2500L
#endif
/* Arranca en 0, que con polaridad es exactamente Vref: el punto natural
 * para empezar a anular un offset, y el unico desde el que se puede ir
 * para los dos lados. Antes era un adelanto fijo en mV sobre el LSB del
 * VDAC, que no tiene sentido en un IDAC con signo. */
#define CAL_DAC_CENTER_GEO_SUM 0

/* ACOTADO POR MEDICION el 2026-09-04, antes era el rango completo (255).
 *
 * El barrido de la curva mostro que esta etapa SATURA, y no poco: su pendiente
 * local va de 1963 uV/codigo en el centro a 0,4 uV/codigo en los extremos, un
 * factor 5000. El rango donde la pendiente supera el 30 % de su mediana es
 * -178 .. +128, o sea que el 40 % del recorrido nominal no sirve para nada.
 * Fuera de ahi el lazo empuja sin efecto y termina contra el riel.
 *
 * OJO, la curva en S es REAL. En el handoff del 2026-09-03 quedo escrito que
 * era un limitador de firmware y no recorte de la etapa. Eran DOS efectos: el
 * limitador existia y se saco, y debajo estaba esta saturacion, que tapaba.
 *
 * Se usa 128 y no 178 porque el clamp de calibration.c es SIMETRICO respecto de
 * dac_center, asi que hay que tomar el lado que ata. Se pierde el tramo
 * -178..-128, que si es util. Soportar limites asimetricos seria mejor y es un
 * cambio chico, pero toca la estructura y todos los inicializadores; queda
 * anotado como mejora, no se hace a las apuradas.
 *
 * Ver docs/MEDICIONES_2026-09-04.md, seccion 3. */
#define CAL_DAC_MAX_CHANGE_GEO_SUM 128

#ifndef CAL_PI_GAIN_GEO_SUM_X1000
/* Ganancia fisica referencia -> tap, medida en la placa el 2026-09-02
 * con el barrido de D2, dividida por el escalon real de 1875 uV. */
#define CAL_PI_GAIN_GEO_SUM_X1000 411L
#endif

#ifndef CAL_PI_DEADBAND_GEO_SUM_COUNTS
#define CAL_PI_DEADBAND_GEO_SUM_COUNTS 26L
#endif

/* Kp y Ki de la simulacion Monte Carlo del lazo
 * (calculos_modelados/python/calibracion_pi). El barrido de robustez
 * contra tau no discrimina esta etapa porque su banda muerta es ancha y
 * arranca adentro, asi que manda la grilla con perturbacion real, donde
 * este par converge el 100 % con p95 de 122 a 1089 muestras segun la
 * etapa. Igual quedo marcado como robusto para todos los tau probados. */
#define CAL_PI_KP_NUM_GEO_SUM 1L
#define CAL_PI_KP_DIV_GEO_SUM 1L
#define CAL_PI_KI_NUM_GEO_SUM 0L
#define CAL_PI_KI_DIV_GEO_SUM 1L

#ifndef CAL_PI_LOCK_SAMPLES_GEO_SUM
#define CAL_PI_LOCK_SAMPLES_GEO_SUM 3u
#endif

#ifndef CAL_PI_SETTLE_SAMPLES_GEO_SUM
#define CAL_PI_SETTLE_SAMPLES_GEO_SUM CAL_PI_FIR_SETTLE_SAMPLES
#endif

/* Espera de la planta para esta etapa. Ver el bloque de
 * CAL_PI_PLANT_SETTLE_* en calibration_tables.h: es un concepto distinto
 * del vaciado del FIR de arriba, y hoy vale cero a proposito. */
#ifndef CAL_PI_PLANT_SETTLE_SAMPLES_GEO_SUM
#define CAL_PI_PLANT_SETTLE_SAMPLES_GEO_SUM CAL_PI_PLANT_SETTLE_SAMPLES_DEFAULT
#endif

#ifndef CAL_PI_TIMEOUT_SAMPLES_GEO_SUM
/* EL ADDER NECESITA MAS PASOS QUE EL LP, y no por ser peor sino por venir de
 * mas lejos. Es el actuador con autoridad: cuando la cadena arranca contra el
 * riel, es el unico que puede sacarla, y desde el riel la medida esta saturada,
 * asi que el error que ve el lazo es una COTA INFERIOR del real. Cada paso de
 * Newton corrige lo que ve, que es menos que lo que hay, y por eso los primeros
 * pasos avanzan de a poco. Recien cuando el tap sale del riel la medida vuelve
 * a ser fiel y la convergencia se vuelve rapida.
 *
 * Medido el 2026-09-05 con cinco pasos: el LP paso de -2,645 V a 0,091 V, o sea
 * salio del riel y recorrio 2,7 V, pero se quedo sin presupuesto a 2,3 V del
 * objetivo. Doce pasos dan margen de sobra para los dos tramos.
 *
 * 950.000 muestras a 2604 Hz son 365 s: doce pasos de 1 tau mas margen. En
 * muestras porque es TIEMPO; se pasa a iteraciones en el punto de uso. */
#define CAL_PI_TIMEOUT_SAMPLES_GEO_SUM 950000u
#endif

#ifndef CAL_PI_REFINE_ENABLE_GEO_SUM
#define CAL_PI_REFINE_ENABLE_GEO_SUM 1u
#endif

#ifndef CAL_PI_REFINE_SETTLE_SAMPLES_GEO_SUM
#define CAL_PI_REFINE_SETTLE_SAMPLES_GEO_SUM CAL_PI_FIR_SETTLE_SAMPLES
#endif

#endif
