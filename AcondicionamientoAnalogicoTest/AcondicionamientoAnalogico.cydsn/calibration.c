/* LA CALIBRACION VIVE EN EL PROYECTO DE CAMPO. Aca no hay una segunda copia.
 *
 * Elias, 2026-09-05: "hace algo para que no pase mas el problema".
 *
 * EL PROBLEMA. Este archivo era una copia entera de la calibracion del proyecto
 * de campo, y habia divergido. En un solo dia costo cuatro fallas, todas
 * silenciosas -ninguna daba error, todas devolvian numeros de aspecto normal-:
 *
 *   1. Los contadores de espera seguian en uint16. Con la espera de planta en
 *      2 tau son 153.620 muestras y el techo es 65.535: daban la vuelta y
 *      esperaban 8,7 s en vez de 59. Arreglado en el de campo el 2026-09-04 y
 *      no aca.
 *   2. Faltaba la espera de planta ENTERA. Por eso la calibracion contestaba
 *      ok=0 a los 59,7 s: los cuatro timeouts suman 57,6 s, o sea que corria
 *      completa sin haber esperado nunca a que la cadena se asentara.
 *   3. cal_pi_compare_counts devolvia la cuenta cruda en vez de restarle Vref,
 *      asi que el cero que perseguia el lazo no era el mismo cero.
 *   4. El lector de ADC era el helper viejo por polling, el que puede devolver
 *      un cero inventado cuando la ISR le come el flag de fin de conversion, y
 *      que ademas no espera a que el decimador del Delta-Sigma se asiente tras
 *      cambiar el AMux -el error era peor justamente al pasar de SUM a LP, que
 *      son los dos taps que mas se midieron-.
 *
 * Las cuatro venian de lo mismo: dos copias del mismo algoritmo. Ahora hay una.
 * Este proyecto es "el de campo mas las primitivas de laboratorio", que es lo
 * que siempre debio ser.
 *
 * POR QUE UN INCLUDE Y NO UNA CARPETA COMPARTIDA. Sumar un archivo al proyecto
 * obliga a editar el .cyprj a mano, y eso ya rompio un build antes. El .cyprj
 * sigue listando este calibration.c; lo que cambia es que ahora tiene dos
 * lineas. Y un #include "" se resuelve relativo al archivo que incluye, asi que
 * el de campo encuentra sus propias tablas sin tocar rutas de busqueda.
 */

/* Si el include de abajo se resolviera a ESTE archivo -por haberlo movido de
 * carpeta, por ejemplo- la recursion seria infinita y el error del compilador
 * no diria por que. Este centinela lo convierte en un mensaje que se entiende. */
#ifdef PSOC_CAL_SHIM_DEL_AUTOTEST
#error "calibration.c se incluyo a si mismo: la ruta al proyecto de campo quedo mal"
#endif
#define PSOC_CAL_SHIM_DEL_AUTOTEST 1

#include "../../AcondicionamientoAnalogico.cydsn/calibration.c"

/* Las primitivas que el autotest expone a main.c. Van despues del include
 * porque necesitan ver los static de la calibracion; ese es el motivo de que
 * sean un .inc y no un .c aparte. */
#include "psoc_selftest_primitivas.inc"
