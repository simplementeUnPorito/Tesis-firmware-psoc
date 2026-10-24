/* aa_decim.h - antialias antes de decimar por software (FIR polifasico).
 *
 * Reemplaza la media movil de N muestras. La media movil es un sinc con
 * lobulos de -13 dB: lo que esta entre fs_nueva/2 y fs se pliega casi entero
 * sobre la banda util. Este filtro es un sinc con ventana Kaiser (60 dB) que
 * deja pasar hasta 0,8 x Nyquist nuevo y rechaza todo lo que caeria sobre esa
 * banda al decimar (desde fs_nueva - 0,8 x Nyquist nuevo).
 *
 * Fase lineal: retardo constante de (L-1)/2 muestras de entrada, igual en
 * todos los nodos, asi que no altera la fase relativa que usa MASW. Los
 * coeficientes se calculan aca cuando cambia N (flotante por software, solo
 * al configurar); la ISR hace el producto en enteros con acumulador de 64 bits
 * y calcula solo una salida cada N entradas.
 *
 * Header con implementacion: se incluye una sola vez desde main.c, asi no hay
 * que registrar un .c nuevo en el proyecto de PSoC Creator.
 */
#ifndef AA_DECIM_H
#define AA_DECIM_H

#include <project.h>

#define AA_MAX_TAPS   511u     /* impar; con N > 28 la atenuacion baja */
#define AA_ATTEN_DB   60.0f
#define AA_PASS_FRAC  0.8f     /* banda util: hasta 0,8 x Nyquist nuevo */

static int16  g_aa_coef[AA_MAX_TAPS];   /* q15, suma = 32768 (ganancia 1 en DC) */
static int32  g_aa_hist[AA_MAX_TAPS];   /* ultimas L entradas, anillo */
static uint16 g_aa_len   = 1u;
static uint16 g_aa_head  = 0u;          /* proxima posicion a escribir */
static uint16 g_aa_decim = 1u;          /* N configurado */
static uint16 g_aa_phase = 0u;          /* entradas desde la ultima salida */
static uint8  g_aa_primed = 0u;

/* El proyecto no enlaza libm: seno y raiz propios. Sobran para coeficientes
 * que despues se cuantizan a q15. */
static float aa_sinf(float x)
{
    const float pi = 3.14159265f;
    const float two_pi = 6.28318531f;
    float x2;
    float term;
    float sum;
    int k;
    /* Reducir a [-pi, pi]. */
    while (x > pi) { x -= two_pi; }
    while (x < -pi) { x += two_pi; }
    x2 = x * x;
    term = x;
    sum = x;
    for (k = 1; k <= 6; k++) {
        term *= -x2 / (float)((2 * k) * (2 * k + 1));
        sum += term;
    }
    return sum;
}

static float aa_sqrtf(float x)
{
    float r;
    int i;
    if (x <= 0.0f) { return 0.0f; }
    r = (x > 1.0f) ? x : 1.0f;
    for (i = 0; i < 30; i++) {
        float nr = 0.5f * (r + x / r);
        if (nr == r) { break; }
        r = nr;
    }
    return r;
}

/* I0 de Bessel por serie (alcanza para beta <= 10). */
static float aa_bessel_i0(float x)
{
    float sum = 1.0f;
    float term = 1.0f;
    float k;
    float y = x * x / 4.0f;
    for (k = 1.0f; k < 40.0f; k += 1.0f) {
        term *= y / (k * k);
        sum += term;
        if (term < 1e-8f * sum) { break; }
    }
    return sum;
}

/* Calcula los coeficientes para decimar por N. Llamar fuera de la ISR. */
static void aa_configure(uint16 n)
{
    uint16 i;
    uint16 len;
    float beta;
    float fc;
    float m;
    float i0b;
    float acc = 0.0f;
    int32 sum = 0;
    float taps_f;

    if (n == 0u) { n = 1u; }
    g_aa_decim = n;
    g_aa_phase = 0u;
    g_aa_head = 0u;
    g_aa_primed = 0u;
    if (n == 1u) {
        g_aa_len = 1u;
        g_aa_coef[0] = 32767;
        return;
    }

    /* Transicion: de 0,4/N a 0,6/N ciclos/muestra (normalizado a fs). */
    taps_f = (AA_ATTEN_DB - 8.0f) / (2.285f * 2.0f * 3.14159265f * (1.0f - AA_PASS_FRAC) / (float)n);
    len = (uint16)(taps_f + 1.5f);
    if ((len & 1u) == 0u) { len++; }
    if (len > AA_MAX_TAPS) { len = AA_MAX_TAPS; }
    if (len < 3u) { len = 3u; }
    g_aa_len = len;

    beta = 0.1102f * (AA_ATTEN_DB - 8.7f);
    i0b = aa_bessel_i0(beta);
    fc = 0.5f / (float)n;            /* corte en el Nyquist nuevo */
    m = (float)(len - 1u) / 2.0f;

    /* Primero en flotante para normalizar la suma exacta. */
    for (i = 0u; i < len; i++) {
        float x = (float)i - m;
        float h = (x == 0.0f) ? 2.0f * fc
                              : aa_sinf(2.0f * 3.14159265f * fc * x) / (3.14159265f * x);
        float r = x / m;
        float w = aa_bessel_i0(beta * aa_sqrtf(1.0f - r * r)) / i0b;
        acc += h * w;
    }
    for (i = 0u; i < len; i++) {
        float x = (float)i - m;
        float h = (x == 0.0f) ? 2.0f * fc
                              : aa_sinf(2.0f * 3.14159265f * fc * x) / (3.14159265f * x);
        float r = x / m;
        float w = aa_bessel_i0(beta * aa_sqrtf(1.0f - r * r)) / i0b;
        float q = (h * w / acc) * 32768.0f;
        int32 c = (int32)(q >= 0.0f ? q + 0.5f : q - 0.5f);
        if (c > 32767) { c = 32767; }
        if (c < -32768) { c = -32768; }
        g_aa_coef[i] = (int16)c;
        sum += c;
    }
    /* El redondeo deja la suma corrida unas cuentas: se corrige en el centro
     * para que la ganancia en continua sea exacta. */
    {
        int32 center = (int32)g_aa_coef[len / 2u] + (32768 - sum);
        if (center > 32767) { center = 32767; }
        g_aa_coef[len / 2u] = (int16)center;
    }
}

/* Una entrada a tasa completa. Devuelve 1 y deja la salida en *out cuando
 * toca (una de cada N). Corre en la ISR. */
static uint8 aa_push(int32 in, int32 *out)
{
    uint16 i;
    uint16 idx;
    int64 acc;

    if (g_aa_decim <= 1u) {
        *out = in;
        return 1u;
    }
    if (!g_aa_primed) {
        /* Llenar con la primera muestra: evita el transitorio de arranque
         * de un historial en cero (un escalon del tamano del offset). */
        for (i = 0u; i < g_aa_len; i++) { g_aa_hist[i] = in; }
        g_aa_primed = 1u;
    }
    g_aa_hist[g_aa_head] = in;
    g_aa_head++;
    if (g_aa_head >= g_aa_len) { g_aa_head = 0u; }
    g_aa_phase++;
    if (g_aa_phase < g_aa_decim) { return 0u; }
    g_aa_phase = 0u;

    /* Producto con el historial: coef[0] con la entrada mas vieja. */
    acc = 0;
    idx = g_aa_head;
    for (i = 0u; i < g_aa_len; i++) {
        acc += (int64)g_aa_coef[i] * (int64)g_aa_hist[idx];
        idx++;
        if (idx >= g_aa_len) { idx = 0u; }
    }
    /* q15 -> entero, redondeando. */
    *out = (int32)((acc + (acc >= 0 ? 16384 : -16384)) >> 15);
    return 1u;
}

/* Reinicia el estado sin recalcular (inicio de cada captura). */
static void aa_reset(void)
{
    g_aa_phase = 0u;
    g_aa_head = 0u;
    g_aa_primed = 0u;
}

#endif /* AA_DECIM_H */
