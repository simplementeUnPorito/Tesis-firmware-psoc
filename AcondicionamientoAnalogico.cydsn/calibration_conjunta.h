#ifndef CALIBRATION_CONJUNTA_H
#define CALIBRATION_CONJUNTA_H

/* ===========================================================================
 * OPTIMIZACION CONJUNTA DE LAS CUATRO REFERENCIAS
 *
 * QUE PROBLEMA RESUELVE, Y POR QUE EL LAZO SECUENCIAL NO PUEDE
 *
 * El lazo de calibracion cierra una etapa por vez, en orden, llevando cada tap
 * a cero. Eso seria optimo si las etapas fueran independientes. NO LO SON, y
 * esta medido: mover la referencia del ADDER corre la salida del LP 2120,9
 * uV/codigo, contra los 524,7 uV/codigo con que el LP mueve su propio tap. O
 * sea que el ADDER tiene 4,04 veces MAS autoridad sobre el LP que el propio LP.
 *
 * Consecuencia: llevar el ADDER exactamente a cero puede ser la peor decision
 * posible para el LP, que es justo el tap que se captura. Aceptar unos
 * milivoltios de error en el ADDER -que tiene 188,9 mV de autoridad y le
 * sobra- para que al LP le quede margen es un intercambio que conviene, y que
 * el lazo secuencial NO PUEDE HACER: cuando cierra el ADDER todavia no sabe
 * que le va a pasar al LP.
 *
 * COMO SE PLANTEA
 *
 * Es la idea de Elias del 2026-09-05, con sus palabras: "no solo buscas reducir
 * el valor del offset de una etapa sino que buscas reducir al minimo todas,
 * dando mas peso a las que tengan mas ganancia acumulada porque son las que mas
 * probablemente saturen. La idea final es eso, evitar que saturen".
 *
 * Formalizado es un minimos cuadrados ponderado con cajon:
 *
 *     minimizar  J(d) = suma_j  w_j * ( off_j + suma_k G[k][j] * d_k )^2
 *     sujeto a   |d_k| <= limite_k
 *
 * con G la matriz de acople MEDIDA y w_j el cuadrado de la ganancia acumulada
 * hasta el tap j.
 *
 * POR QUE EL PESO ES EL CUADRADO DE LA GANANCIA ACUMULADA, y no otra cosa: lo
 * que decide si un tap satura es (offset + excursion de senal) contra el riel,
 * y la excursion en el tap j es proporcional a su ganancia acumulada A_j. Por
 * lo tanto el presupuesto de offset que le toca a cada tap tiene que ser
 * INVERSAMENTE proporcional a A_j, y penalizar el error con A_j^2 es
 * exactamente eso en minimos cuadrados. No es una heuristica: es el
 * presupuesto de excursion escrito como funcion de costo.
 *
 * COMO SE RESUELVE EN UN PSoC
 *
 * Cuatro variables, forma cuadratica convexa, cajon: descenso ciclico por
 * coordenadas. Para cada k se resuelve EXACTAMENTE el minimo en esa coordenada
 * dejando las otras fijas, y se proyecta al cajon. Cada paso es una division
 * entera. Con cuatro variables converge en pocas pasadas. Sin matrices, sin
 * flotante, sin inversas.
 *
 * UNIDADES. Se trabaja en MICROVOLTIOS EN EL TAP, no en cuentas de ADC: asi la
 * matriz entra tal como se midio y no hay que convertirla. Los codigos de IDAC
 * son enteros con signo -255..255.
 *
 * LO QUE ESTE ARCHIVO NO HACE: no toca hardware ni tiene estado. Es aritmetica
 * pura, para que se pueda probar contra el modelo de Python sin una placa.
 * ===========================================================================
 */

#include <cytypes.h>

#define CAL_CONJ_ETAPAS 4u

/* ---------------------------------------------------------------------------
 * MATRIZ DE ACOPLE MEDIDA, en uV en el tap j por codigo de IDAC de la etapa k.
 *
 * Medida el 2026-09-04 con escalones de +120 codigos en cada IDAC leyendo los
 * cuatro taps durante 150 s, ajustando y(t) = y_inf + A*e^(-t/tau) y tomando el
 * REGIMEN PERMANENTE y_inf, no el ultimo punto.
 *
 * Se usa la matriz de continua y no la instantanea porque lo que se quiere
 * acotar es donde TERMINA la cadena, no por donde pasa; del transitorio se
 * encarga la espera de planta.
 *
 * Control de calidad: cada etapa se midio yendo a +120 y volviendo a 0, y las
 * dos ramas coinciden dentro del 1-1,5 %, lo que valida la matriz y descarta
 * histeresis en el rango medido.
 *
 * La matriz es TRIANGULAR -aguas abajo no afecta aguas arriba-, lo que ademas
 * valida el orden de calibracion 0,1,2,3.
 *
 * OJO: la fila 0 vale para PGA x1. La ganancia de la etapa 0 escala con la
 * ganancia del PGA (el firmware ya lo hace en cal_pi_stage_gain_x1000), y por
 * eso cal_conjunta_resolver() recibe un factor para esa fila.
 * Ver docs/MEDICIONES_2026-09-04.md
 * ------------------------------------------------------------------------- */
static const int32 g_cal_conj_g_dc_uv[CAL_CONJ_ETAPAS][CAL_CONJ_ETAPAS] = {
    /*            ch0 PGA   ch1 BP   ch2 ADDER   ch3 LP */
    /* Vref_PGA   */ {    61,      0,     -240,     1026 },
    /* Vref_BP    */ {     0,     90,     -388,     1002 },
    /* Vref_ADDER */ {     2,      0,      741,    -2121 },
    /* Vref_LP    */ {    -2,      0,        0,      525 },
};

/* Escala de los pesos. Se normalizan para que el mayor valga esto, y no se usan
 * las ganancias acumuladas crudas: con PGA x50 y PGAout x50 la ganancia
 * acumulada es 2500, su cuadrado 6,25e6, y multiplicado por un residuo de 2e6 uV
 * y una ganancia de 2121 uV/codigo desborda int64. Normalizando a 10 bits el
 * peor producto queda en ~2e13, con cuatro ordenes de margen. */
#define CAL_CONJ_PESO_MAX 1024L

/* ---------------------------------------------------------------------------
 * Ganancia acumulada del camino de SENAL hasta cada tap.
 *
 * PROVISORIO, Y ESTA MARCADO A PROPOSITO. Hoy sale de la estructura supuesta de
 * la cadena: el PGA de entrada amplifica desde el tap 0, y el PGAout desde el
 * tap 2. Las ganancias propias del pasabanda, del sumador y del pasabajos NO
 * estan medidas y aca se toman como uno.
 *
 * COMO SE MIDEN DE VERDAD, y hay que hacerlo: el geofono entrega ruido ambiente
 * de banda ancha, o sea la MISMA excitacion en los cuatro taps al mismo tiempo.
 * El cociente de RMS entre taps consecutivos ES la ganancia de la etapa que hay
 * en el medio, medida sobre la senal real. Eso no se puede sacar de la matriz de
 * acople, que es referencia->tap y no entrada->tap: son dos caminos distintos.
 *
 * No se inventan ganancias que no se midieron; se deja el hueco senalado.
 * ------------------------------------------------------------------------- */
static void cal_conjunta_ganancia_acumulada(uint16 pga_x, uint16 pgaout_x,
                                            uint32 out_acum[CAL_CONJ_ETAPAS])
{
    uint32 g_pga = (pga_x == 0u) ? 1u : (uint32)pga_x;
    uint32 g_out = (pgaout_x == 0u) ? 1u : (uint32)pgaout_x;

    out_acum[0] = g_pga;
    out_acum[1] = g_pga;
    out_acum[2] = g_pga * g_out;
    out_acum[3] = g_pga * g_out;
}

/* Pesos normalizados a CAL_CONJ_PESO_MAX, proporcionales al CUADRADO de la
 * ganancia acumulada. */
static void cal_conjunta_pesos(uint16 pga_x, uint16 pgaout_x,
                               int32 out_pesos[CAL_CONJ_ETAPAS])
{
    uint32 acum[CAL_CONJ_ETAPAS];
    uint32 mayor = 1u;
    uint8 j;

    cal_conjunta_ganancia_acumulada(pga_x, pgaout_x, acum);
    for (j = 0u; j < CAL_CONJ_ETAPAS; j++) {
        if (acum[j] > mayor) { mayor = acum[j]; }
    }
    /* w_j = PESO_MAX * (A_j / A_max)^2, calculado sin flotante y sin perder
     * resolucion: se hace el cuadrado del cociente escalado, no el cociente de
     * los cuadrados, para que no desborde con A_max = 2500. */
    for (j = 0u; j < CAL_CONJ_ETAPAS; j++) {
        int64 razon = ((int64)acum[j] * 1024LL) / (int64)mayor;   /* 0..1024 */
        int64 w = (razon * razon * (int64)CAL_CONJ_PESO_MAX) / (1024LL * 1024LL);
        out_pesos[j] = (w < 1LL) ? 1L : (int32)w;   /* nunca cero: cero ignora el tap */
    }
}

/* ---------------------------------------------------------------------------
 * El solver.
 *
 *   off_uv     offset de cada tap, en uV, CON LOS CUATRO IDAC EN CERO. Si se
 *              mide con los IDAC en otro lado hay que descontar su aporte
 *              antes de llamar (ver cal_conjunta_offset_equivalente).
 *   pesos      salida de cal_conjunta_pesos()
 *   limite     |codigo| maximo por etapa; permite acotar el ADDER sin tocar
 *              las otras
 *   pga_x      ganancia del PGA de entrada, para escalar la fila 0
 *   out_dac    resultado, codigos con signo
 *
 * Devuelve la cantidad de pasadas que hizo, util para diagnostico.
 * ------------------------------------------------------------------------- */
#define CAL_CONJ_PASADAS_MAX 40u

/* ---------------------------------------------------------------------------
 * REGULARIZACION. No es un adorno numerico: sin esto el problema NO TIENE UNA
 * SOLUCION UTIL, y se comprobo midiendo.
 *
 * EL PROBLEMA. Las columnas de la matriz son casi paralelas: la etapa 0 mueve
 * el tap del LP 1026 uV/codigo y la etapa 1 lo mueve 1002. Practicamente lo
 * mismo. Entonces existe una direccion en la que subir el IDAC de una y bajar
 * el de la otra deja el LP casi igual, y el costo casi no cambia. El problema
 * es convexo pero MAL CONDICIONADO: tiene un valle plano.
 *
 * QUE PASA SI NO SE ARREGLA. Se verifico comparando esta implementacion contra
 * el modelo de Python sobre las mismas entradas: las dos "convergian" a puntos
 * distintos con costos que diferian hasta 1,57x, cada una ganandole a la otra
 * segun el caso, y las dos usaban hasta 255 codigos de DAC para offsets de
 * apenas 60 mV. O sea: gastaban todo el rango moviendose a lo largo del valle
 * sin mejorar nada, y quedaban pegadas al riel, que es justo el modo de falla
 * que se quiere evitar.
 *
 * LA CORRECCION. Se agrega al costo un termino que penaliza el ESFUERZO:
 *
 *     J(d) = suma_j w_j*r_j^2  +  lambda * suma_k d_k^2
 *
 * Eso vuelve el problema estrictamente convexo, hace que el descenso por
 * coordenadas converja rapido, y sobre todo elige, entre todas las soluciones
 * casi equivalentes, LA DE MENOR ESFUERZO. Que es exactamente lo que pidio
 * Elias: "lo ideal es que LP este chico y las otras al minimo".
 *
 * COMO SE ELIGE LAMBDA, y aca hay una trampa en la que cai primero. Lambda
 * tiene que ser un valor ABSOLUTO sumado a la curvatura, no un factor que la
 * multiplique. Multiplicar den por (1 + 1/256) encoge todas las coordenadas un
 * 0,4 % por igual y NO resuelve nada: la direccion plana sigue igual de plana.
 * Sumarle una constante, en cambio, afecta poco a las coordenadas bien
 * determinadas -donde den es grande- y domina en las mal determinadas, que es
 * exactamente lo que se quiere.
 *
 * Se toma lambda = (mayor de los den) / CAL_CONJ_LAMBDA_DEN, asi escala sola
 * con las unidades, con los pesos y con la ganancia del PGA, y no hay que
 * retocarla nunca. Con 1/256, en la direccion bien condicionada el optimo se
 * corre menos de un codigo; en la plana, cuya curvatura es tres ordenes menor,
 * manda y la resuelve hacia el minimo esfuerzo. */
#ifndef CAL_CONJ_LAMBDA_NUM
#define CAL_CONJ_LAMBDA_NUM 1L
#endif
#ifndef CAL_CONJ_LAMBDA_DEN
#define CAL_CONJ_LAMBDA_DEN 256L
#endif

static uint8 cal_conjunta_resolver(const int32 off_uv[CAL_CONJ_ETAPAS],
                                   const int32 pesos[CAL_CONJ_ETAPAS],
                                   const int16 limite[CAL_CONJ_ETAPAS],
                                   uint16 pga_x,
                                   int16 out_dac[CAL_CONJ_ETAPAS])
{
    int32 d[CAL_CONJ_ETAPAS];
    int32 g[CAL_CONJ_ETAPAS][CAL_CONJ_ETAPAS];
    int64 den_fijo[CAL_CONJ_ETAPAS];
    int64 lambda;
    uint8 pasada;
    uint8 k, j, m;
    uint32 escala_pga = (pga_x == 0u) ? 1u : (uint32)pga_x;

    /* Copia local de la matriz con la fila 0 escalada por la ganancia del PGA.
     * La fila 0 se midio a x1 y la etapa 0 inyecta antes de la amplificacion,
     * que es lo que ya supone cal_pi_stage_gain_x1000(). */
    for (k = 0u; k < CAL_CONJ_ETAPAS; k++) {
        for (j = 0u; j < CAL_CONJ_ETAPAS; j++) {
            g[k][j] = g_cal_conj_g_dc_uv[k][j];
            if (k == 0u) {
                g[k][j] = (int32)((int64)g[k][j] * (int64)escala_pga);
            }
        }
        d[k] = 0L;
    }

    /* Curvatura de cada coordenada. No depende de d, asi que sale una sola vez
     * fuera del bucle, y de paso fija lambda. */
    {
        int64 den_max = 0LL;
        for (k = 0u; k < CAL_CONJ_ETAPAS; k++) {
            int64 den_k = 0LL;
            for (j = 0u; j < CAL_CONJ_ETAPAS; j++) {
                den_k += (int64)pesos[j] * (int64)g[k][j] * (int64)g[k][j];
            }
            den_fijo[k] = den_k;
            if (den_k > den_max) { den_max = den_k; }
        }
        lambda = den_max / (int64)CAL_CONJ_LAMBDA_DEN;
        if (lambda < 1LL) { lambda = 1LL; }
    }

    for (pasada = 0u; pasada < CAL_CONJ_PASADAS_MAX; pasada++) {
        uint8 movio = 0u;

        for (k = 0u; k < CAL_CONJ_ETAPAS; k++) {
            int64 num = 0LL;    /* suma_j w_j * r_j(sin k) * g[k][j] */
            int64 den;
            int32 nuevo;

            for (j = 0u; j < CAL_CONJ_ETAPAS; j++) {
                int64 r_sin_k;
                if (g[k][j] == 0L) { continue; }
                r_sin_k = (int64)off_uv[j];
                for (m = 0u; m < CAL_CONJ_ETAPAS; m++) {
                    if (m == k) { continue; }
                    r_sin_k += (int64)d[m] * (int64)g[m][j];
                }
                num += (int64)pesos[j] * r_sin_k * (int64)g[k][j];
            }
            /* Regularizacion: lambda SUMADA a la curvatura, no multiplicandola.
             * Ver el bloque de arriba: multiplicarla no resuelve nada. */
            den = den_fijo[k] + lambda;
            if (den == 0LL) { continue; }

            /* Minimo exacto en la coordenada k, con redondeo al entero mas
             * cercano en vez de truncar hacia cero: truncar sesga siempre hacia
             * el mismo lado y con cuatro coordenadas eso se acumula. */
            {
                int64 q = -num;
                int64 mitad = den / 2LL;
                nuevo = (int32)((q >= 0LL) ? ((q + mitad) / den) : ((q - mitad) / den));
            }
            if (nuevo >  (int32)limite[k]) { nuevo =  (int32)limite[k]; }
            if (nuevo < -(int32)limite[k]) { nuevo = -(int32)limite[k]; }
            if (nuevo != d[k]) { d[k] = nuevo; movio = 1u; }
        }
        if (!movio) {
            break;
        }
    }

    for (k = 0u; k < CAL_CONJ_ETAPAS; k++) {
        out_dac[k] = (int16)d[k];
    }
    return (uint8)(pasada + 1u);
}

/* ---------------------------------------------------------------------------
 * Offset EQUIVALENTE con los IDAC en cero, a partir de una medida hecha con los
 * IDAC en cualquier lado.
 *
 * Hace falta para iterar: en la segunda vuelta los DAC ya no estan en cero, y
 * si se le pasara al solver la medida cruda resolveria un problema incremental
 * cuyo error se acumula. Descontando el aporte conocido, cada iteracion resuelve
 * el problema ABSOLUTO y no arrastra la anterior.
 * ------------------------------------------------------------------------- */
static void cal_conjunta_offset_equivalente(const int32 medido_uv[CAL_CONJ_ETAPAS],
                                            const int16 dac[CAL_CONJ_ETAPAS],
                                            uint16 pga_x,
                                            int32 out_off_uv[CAL_CONJ_ETAPAS])
{
    uint32 escala_pga = (pga_x == 0u) ? 1u : (uint32)pga_x;
    uint8 j, k;

    for (j = 0u; j < CAL_CONJ_ETAPAS; j++) {
        int64 aporte = 0LL;
        for (k = 0u; k < CAL_CONJ_ETAPAS; k++) {
            int64 gkj = (int64)g_cal_conj_g_dc_uv[k][j];
            if (k == 0u) { gkj *= (int64)escala_pga; }
            aporte += (int64)dac[k] * gkj;
        }
        out_off_uv[j] = (int32)((int64)medido_uv[j] - aporte);
    }
}

#endif /* CALIBRATION_CONJUNTA_H */
