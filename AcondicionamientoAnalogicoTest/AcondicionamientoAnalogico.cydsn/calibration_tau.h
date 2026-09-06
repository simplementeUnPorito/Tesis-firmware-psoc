#ifndef CALIBRATION_TAU_H
#define CALIBRATION_TAU_H

/* ===========================================================================
 * AUTOMEDICION DE TAU
 *
 * POR QUE EXISTE
 * La espera de planta se dimensiona con tau, y tau depende de la temperatura
 * porque C1 es un electrolitico. En la ubicacion del nodo la temperatura va de
 * 8,1 a 31,6 C en la misma semana (Open-Meteo, 2026-09-05), asi que una
 * constante de banco tomada a 19 C no cubre el caso real.
 *
 * La decision de Elias fue explicita y es la correcta: **no corregir por
 * temperatura, sino MEDIR tau en el propio nodo** y dimensionar con lo medido.
 * La temperatura queda como explicacion del fenomeno, no como entrada del
 * algoritmo, y de paso el nodo no necesita ningun sensor.
 *
 * EL ESTIMADOR, Y POR QUE ESTE Y NO UN AJUSTE
 * Con y(t) = y_inf + A*e^(-t/tau), TRES muestras igualmente espaciadas alcanzan:
 *
 *     r = ( y(2t1) - y(0) ) / ( y(t1) - y(0) )  =  1 + e^(-t1/tau)
 *     tau = t1 / ( -ln(r - 1) )
 *
 * Lo que lo hace viable en un PSoC es que **y_inf se cancela**: no hay que
 * esperar a que la planta se asiente para estimar cuanto tarda en asentarse,
 * que es el problema del huevo y la gallina de cualquier metodo que necesite el
 * valor final. Y no hay que guardar cientos de muestras ni ajustar nada.
 *
 * VALIDADO CONTRA DATO REAL. Se probo sobre las series del escalon del
 * 2026-09-04, que tienen ~500 puntos y un ajuste exponencial completo por
 * minimos cuadrados como referencia. Programa:
 * src/interfaces/python/validar_estimador_tau.py
 *
 * El resultado tiene un matiz que importa: promediado sobre TODOS los pares
 * (etapa, tap) el error es del 22 % y parece inservible. Separando por par, el
 * de **etapa 1 -> ch2** da **0,7 % y 2,7 %** en dos ensayos independientes con
 * t1 = 45 s. Los que fallan son los pares de senal debil, que no aportan
 * informacion y ensucian el promedio.
 *
 * POR ESO EL PAR NO ES ARBITRARIO: se mide moviendo la referencia del PASABANDA
 * (etapa 1) y mirando el tap del ADDER (ch2). Es el par con mayor relacion
 * senal-ruido de toda la matriz, porque su transitorio es MAS GRANDE que su
 * regimen permanente: G_TRANS = -861 uV/codigo contra G_DC = -388. Justo lo que
 * un estimador de transitorio necesita.
 *
 * Este archivo tiene solo la aritmetica, sin hardware ni estado, para poder
 * probarlo contra el modelo de Python sin una placa.
 * ===========================================================================
 */

#include <cytypes.h>

/* Par elegido y escalon de medicion. */
#define CAL_TAU_ETAPA        1u      /* referencia del pasabanda   */
#define CAL_TAU_TAP          2u      /* tap del ADDER              */
#define CAL_TAU_DELTA_COD    20      /* +-20 codigos: ~25 mV de excursion en ch2,
                                      * unas 850 veces el ruido de una medida, y
                                      * lejos del riel en cualquier ganancia
                                      * razonable */
#define CAL_TAU_T1_MS        45000UL /* el t1 que mejor salio en la validacion */

/* ---------------------------------------------------------------------------
 * -ln(x) para x en (0,1), en punto fijo.
 *
 * La tabla da  -ln(x) * 4096  para x = i/32, i = 1..31. Entre puntos se
 * interpola linealmente. Con 31 entradas el error de interpolacion se queda por
 * debajo del 1 % en todo el rango util, que es un orden menos que la dispersion
 * del propio tau entre ensayos: no tiene sentido gastar mas tabla.
 * ------------------------------------------------------------------------- */
static const uint16 g_cal_tau_menos_ln_x4096[32] = {
    0,      /* i=0 no se usa: -ln(0) diverge */
    14200, 11364, 9705, 8529, 7616, 6870, 6239, 5693,
    5211,  4780,  4390, 4033, 3705, 3400, 3115, 2848,
    2596,  2358,  2131, 1916, 1710, 1513, 1324, 1143,
    968,   799,   636,  478,  325,  177,  33
};

/* Devuelve -ln(x)*4096 para x dado como numerador/4096 en (0,4096). */
static int32 cal_tau_menos_ln_q12(int32 x_q12)
{
    int32 pos, i, frac, a, b;

    if (x_q12 <= 0L) { return 0L; }
    if (x_q12 >= 4096L) { return 0L; }

    /* posicion en la tabla, en Q5 sobre 32 entradas */
    pos = (x_q12 * 32L);              /* 0..131072 */
    i = pos / 4096L;                  /* 0..31 */
    frac = pos - i * 4096L;           /* 0..4095 */
    if (i < 1L) { i = 1L; frac = 0L; }
    if (i >= 31L) { return (int32)g_cal_tau_menos_ln_x4096[31]; }
    a = (int32)g_cal_tau_menos_ln_x4096[i];
    b = (int32)g_cal_tau_menos_ln_x4096[i + 1L];
    return a + ((b - a) * frac) / 4096L;
}

/* ---------------------------------------------------------------------------
 * El estimador.
 *
 *   y0, y1, y2   las tres muestras del tap, en counts de ADC, tomadas en
 *                0, t1_ms y 2*t1_ms despues del escalon
 *   ruido_counts el ruido de una medida sola, para poder rechazar
 *
 * Devuelve tau en ms, o 0 si hay que RECHAZAR la medicion. Rechazar es la
 * respuesta correcta cuando el dato no alcanza: un tau inventado hace que la
 * calibracion espere un tiempo ridiculo, y eso es peor que seguir con el valor
 * anterior.
 * ------------------------------------------------------------------------- */
static uint16 cal_tau_estimar_ms(int32 y0, int32 y1, int32 y2,
                                 uint32 t1_ms, int32 ruido_counts)
{
    int32 da = y1 - y0;
    int32 db = y2 - y0;
    int32 x_q12, ln_q12;
    int64 tau;

    /* La primera diferencia tiene que destacarse del ruido, o el cociente es
     * ruido dividido ruido. Ocho veces es holgado y con el par elegido -unas
     * 850 veces el ruido- no rechaza nunca en condiciones normales. */
    if (da > -8L * ruido_counts && da < 8L * ruido_counts) {
        return 0u;
    }

    /* x = r - 1 = e^(-t1/tau), que tiene que caer en (0,1). Se calcula como
     * (db - da)/da para no perder resolucion restando dos numeros parecidos
     * despues de dividir. */
    x_q12 = (int32)(((int64)(db - da) * 4096LL) / (int64)da);

    /* Fuera de (0,1) la serie no es una exponencial simple: puede estar
     * recortando contra el riel, que es el modo de falla conocido. Rechazar.
     * Los limites dejan afuera tambien los casos donde t1 quedo absurdamente
     * corto o largo respecto de tau, donde la cuenta pierde toda precision. */
    if (x_q12 < 82L || x_q12 > 4014L) {      /* ~0,02 .. ~0,98 */
        return 0u;
    }

    ln_q12 = cal_tau_menos_ln_q12(x_q12);
    if (ln_q12 <= 0L) {
        return 0u;
    }

    /* tau = t1 / (-ln x). Con -ln x en Q12: tau_ms = t1_ms * 4096 / ln_q12. */
    tau = ((int64)t1_ms * 4096LL) / (int64)ln_q12;

    /* Cota de cordura, la misma que psoc_cal_set_tau_ms: 1 s a 60 s. Un tau
     * fuera de ahi no es fisico para C1 = 680 uF contra R4 = 43 k ni con la
     * tolerancia de un electrolitico y el rango de temperatura de campo. */
    if (tau < 1000LL || tau > 60000LL) {
        return 0u;
    }
    return (uint16)tau;
}

#endif /* CALIBRATION_TAU_H */
