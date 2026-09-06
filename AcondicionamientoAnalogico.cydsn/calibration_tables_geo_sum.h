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

/* Y EL LADO NEGATIVO, que es otro numero desde que el clamp dejo de ser
 * simetrico. La curva es util hasta -178, pero se permite todo el recorrido:
 *
 *   - el procedimiento desde la PC, que SI centra la cadena a PGA x50, usa -191,
 *     o sea que mas alla de -178 la etapa todavia sirve aunque su pendiente sea
 *     chica; el criterio del 30 % de la mediana era conservador;
 *   - medido el 2026-09-06, con -128 el tap del LP no se mueve NADA -el propio
 *     tap del ADDER recorre 1,73 V- y el LP a rango completo tampoco lo saca:
 *     el unico camino conocido para rescatar la cadena pasa por aca;
 *   - en la region de pendiente chica el lazo no hace pasos de Newton grandes al
 *     vacio: el rescate en lazo abierto avanza a saltos fijos, y el PI recien
 *     toma el control cuando la medida vuelve a significar algo.
 */
#define CAL_DAC_MAX_CHANGE_NEG_GEO_SUM 255

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
/* CATORCE PASOS, y el numero sale de una cuenta, no de un margen elegido a ojo.
 *
 * El ADDER tiene que hacer dos trabajos distintos en la misma etapa:
 *
 *   1. EL RESCATE. Con la cadena contra el riel, el tap del LP no se ve y no se
 *      puede realimentar: hay que empujar a ciegas. El rescate avanza de a
 *      recorrido/8 = 31 codigos, y el tap entra en la ventana observable recien
 *      pasando -176 (medido el 2026-09-06). Son 0, -31, -62, -93, -124, -155,
 *      -186: SIETE pasos antes de que el lazo pueda hacer algo.
 *   2. EL LAZO. Desde ahi, con la medida ya fiel, converge en pocos pasos.
 *
 * Siete mas siete dan catorce, con margen. Cada paso espera 1 tau porque eso es
 * lo que tarda la cadena en contestar.
 *
 * 1.100.000 muestras a 2604 Hz son 422 s. En muestras porque es TIEMPO; se pasa
 * a iteraciones del lazo en el punto de uso. */
#define CAL_PI_TIMEOUT_SAMPLES_GEO_SUM 1100000u
#endif

#ifndef CAL_PI_REFINE_ENABLE_GEO_SUM
#define CAL_PI_REFINE_ENABLE_GEO_SUM 1u
#endif

#ifndef CAL_PI_REFINE_SETTLE_SAMPLES_GEO_SUM
#define CAL_PI_REFINE_SETTLE_SAMPLES_GEO_SUM CAL_PI_FIR_SETTLE_SAMPLES
#endif

#endif
