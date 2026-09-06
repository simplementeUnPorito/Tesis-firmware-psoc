/* Primitivas de autotest. Se incluye desde calibration.c; NO se compila suelto.
 *
 * Vive aparte desde el 2026-09-05, cuando se unifico el firmware. Antes estaba
 * pegado al final de una COPIA entera de calibration.c, y fue esa copia la que
 * divergio del proyecto de campo. Ahora la calibracion es una sola y esto es lo
 * unico propio del test.
 *
 * Sigue siendo un .inc y no un .c para no tener que tocar el .cyprj a mano.
 *
 * POR QUE HACE POLLING DEL ADC Y LA CALIBRACION NO. Son dos usos distintos: la
 * calibracion toma UNA muestra asentada cada tanto -y por eso el proyecto de
 * campo abandono el polling, que tiene una carrera con la ISR de la ruta
 * filtrada, y espera 30 ms de reloj-, mientras que esto toma SERIES seguidas a
 * la velocidad nativa del ADC, donde 30 ms por muestra no es una opcion. La
 * carrera no aplica aca porque el autotest corre con la ruta filtrada parada.
 */

#ifndef CAL_ADC_DIRECT_WAIT_POLLS
#define CAL_ADC_DIRECT_WAIT_POLLS 60000u
#endif

/* ==========================================================================
 * PRIMITIVAS DE AUTOTEST  (proyecto AcondicionamientoAnalogicoTest)
 * --------------------------------------------------------------------------
 * Este bloque existe SOLO en el proyecto de test. Expone hacia main.c
 * (via psoc_selftest.h) las primitivas de medicion que ya usaba la
 * calibracion, porque el AMux y el lector de ADC son static en este archivo
 * y el autotest los necesita para barrer IDAC contra cada tap del AMux.
 *
 * No se agrega ningun .c nuevo al proyecto: el .cyprj no se toca a mano.
 * ========================================================================== */


/* Sin libm: agregar -lm al link obliga a editar el .cyprj, y ese archivo no
 * se toca a mano. Estas tres funciones cubren lo unico que hacia falta.
 * Solo usan +, -, * y /, que resuelve libgcc. */

/* Raiz cuadrada entera por Newton sobre uint64. Alcanza de sobra: todo lo que
 * se reporta termina redondeado a microvolts enteros. */
static uint32 st_isqrt64(uint64 x)
{
    uint64 r, prev;
    if (x == 0u) { return 0u; }
    /* Semilla: 2^(ceil(bits/2)) */
    r = x;
    prev = 0u;
    {
        uint32 bits = 0u;
        uint64 t = x;
        while (t != 0u) { t >>= 1; bits++; }
        r = (uint64)1u << ((bits + 1u) / 2u);
    }
    /* Newton: r <- (r + x/r)/2, converge en pocas vueltas y es monotona. */
    while (r != prev) {
        prev = r;
        r = (r + x / r) / 2u;
        if (r == 0u) { return 0u; }
        /* corta oscilacion entre dos valores adyacentes */
        if (prev > r && (prev - r) == 1u) { break; }
    }
    while (r * r > x) { r--; }
    return (uint32)r;
}

/* Coseno y seno por Taylor con reduccion de rango a [-pi/4, pi/4].
 * El Goertzel del autotest usa w = 2*pi*f0/fs; con f0 = 50 Hz y fs = 2604 Hz
 * queda w = 0.1207 rad, comodamente dentro del rango donde 5 terminos dan
 * mas precision de la que necesita una medida de ruido. La reduccion esta
 * igual para que siga siendo correcto si se cambia la frecuencia. */
#define ST_PI      3.14159265358979323846
#define ST_TWO_PI  6.28318530717958647692
#define ST_HALF_PI 1.57079632679489661923

static double st_cos_core(double x)   /* |x| <= pi/4 */
{
    double x2 = x * x;
    return 1.0 - x2 * (1.0 / 2.0
         - x2 * (1.0 / 24.0
         - x2 * (1.0 / 720.0
         - x2 * (1.0 / 40320.0
         - x2 * (1.0 / 3628800.0)))));
}

static double st_sin_core(double x)   /* |x| <= pi/4 */
{
    double x2 = x * x;
    return x * (1.0 - x2 * (1.0 / 6.0
              - x2 * (1.0 / 120.0
              - x2 * (1.0 / 5040.0
              - x2 * (1.0 / 362880.0)))));
}

/* Reduce a [0, 2pi) y despacha por octante usando las identidades
 * sin(x+pi/2)=cos(x) y cos(x+pi/2)=-sin(x). */
static void st_sincos(double x, double *out_sin, double *out_cos)
{
    int quad;
    double r;

    while (x < 0.0)        { x += ST_TWO_PI; }
    while (x >= ST_TWO_PI) { x -= ST_TWO_PI; }

    quad = (int)(x / ST_HALF_PI);   /* 0..3 */
    r = x - (double)quad * ST_HALF_PI;

    /* Dentro del cuadrante, si r > pi/4 se usa el complemento para que el
     * argumento del Taylor nunca pase de pi/4. */
    if (r > (ST_HALF_PI / 2.0)) {
        double c = ST_HALF_PI - r;
        double sc = st_sin_core(c);
        double cc = st_cos_core(c);
        /* Con c = pi/2 - r, o sea r = pi/2 - c, y x = quad*pi/2 + r:
         *   quad 0: sin(r)        =  cos(c)   cos(r)        =  sin(c)
         *   quad 1: sin(pi/2+r)   =  sin(c)   cos(pi/2+r)   = -cos(c)
         *   quad 2: sin(pi+r)     = -cos(c)   cos(pi+r)     = -sin(c)
         *   quad 3: sin(3pi/2+r)  = -sin(c)   cos(3pi/2+r)  =  cos(c)
         * Los cuadrantes 1 y 3 estaban intercambiados: devolvia el seno y el
         * coseno con los dos signos dados vuelta. No afectaba a la medicion de
         * 50 Hz (w = 0,12 rad cae en el cuadrante 0 y por la otra rama), pero
         * corrompia la magnitud del Goertzel para cualquier tono por encima de
         * ~325 Hz. */
        switch (quad) {
            case 0:  *out_sin =  cc; *out_cos =  sc; break;
            case 1:  *out_sin =  sc; *out_cos = -cc; break;
            case 2:  *out_sin = -cc; *out_cos = -sc; break;
            default: *out_sin = -sc; *out_cos =  cc; break;
        }
    } else {
        double sr = st_sin_core(r);
        double cr = st_cos_core(r);
        switch (quad) {
            case 0:  *out_sin =  sr; *out_cos =  cr; break;
            case 1:  *out_sin =  cr; *out_cos = -sr; break;
            case 2:  *out_sin = -sr; *out_cos = -cr; break;
            default: *out_sin = -cr; *out_cos =  sr; break;
        }
    }
}

/* Rango de entrada a fondo de escala de cada config del ADC, en microvolts.
 * 18 bits con signo -> +-131072 counts a fondo de escala. */
#define SELFTEST_ADC_FS_COUNTS 131072L

/* Muestras que se usan para estimar la continua antes del Goertzel. */
#define ST_DC_PRE_SAMPLES 64u

static int32 selftest_config_range_uv(void)
{
    switch (psoc_adc_get_config())
    {
        case 2u:  return 512000L;    /* ADC_CF_0V512 */
        case 3u:  return 1024000L;   /* ADC_CF_1V024 */
        case 4u:  return 625000L;    /* ADC_CF_0V625 */
        case 1u:
        default:  return 2500000L;   /* ADC_CF_2V5 */
    }
}

/* Lectura del ADC que SI distingue "no convirtio" de "convirtio cero".
 * cal_adc_read_direct_counts() devuelve 0 al vencer el guard, y eso es
 * indistinguible de una medicion legitima de 0 V. Para el autotest esa
 * ambiguedad es inaceptable: convierte un ADC muerto en un PASS. */
static uint8 st_adc_read_checked(int32 *out)
{
    uint16 guard;
    uint8 ready = 0u;

    if (out == (int32 *)0) { return 0u; }

    ADC_StopConvert();
    ADC_StartConvert();
    for (guard = 0u; guard < (uint16)CAL_ADC_DIRECT_WAIT_POLLS; guard++) {
        if (ADC_IsEndConversion(ADC_RETURN_STATUS)) { ready = 1u; break; }
    }
    if (!ready) {
        ADC_StopConvert();
        return 0u;
    }
    *out = psoc_adc_counts_right_aligned(ADC_GetResult32());
    ADC_StopConvert();
    return 1u;
}

int32 psoc_selftest_counts_to_uv(int32 counts)
{
    int64 num = (int64)counts * (int64)selftest_config_range_uv();
    return (int32)(num / (int64)SELFTEST_ADC_FS_COUNTS);
}

uint8 psoc_selftest_stage_count(void)
{
    return PSOC_CAL_STAGE_COUNT;
}

uint8 psoc_selftest_stage_channel(uint8 stage, uint8 *out_channel)
{
    if (stage >= PSOC_CAL_STAGE_COUNT || out_channel == (uint8 *)0) { return 0u; }
    *out_channel = g_psoc_cal_stages[stage].adc_channel;
    return 1u;
}

uint8 psoc_selftest_amux_channel_count(void)
{
    return (uint8)AMux_ADC_CHANNELS;
}

/* Escribe el IDAC de la etapa SIN clamp: el barrido del autotest necesita
 * poder recorrer el rango entero, -255..+255, no el acotado que usa el PI. */
uint8 psoc_selftest_write_stage_dac(uint8 stage, int16 code)
{
    if (stage >= PSOC_CAL_STAGE_COUNT) { return 0u; }
    g_psoc_cal_stages[stage].write(psoc_hw_idac_clamp_signed(code));
    return 1u;
}

uint8 psoc_selftest_current_stage_dac(uint8 stage, int16 *out_code)
{
    if (stage >= PSOC_CAL_STAGE_COUNT || out_code == (int16 *)0) { return 0u; }
    *out_code = cal_stage_current_dac(stage);
    return 1u;
}

uint8 psoc_selftest_stage_result(uint8 stage, int16 *out_dac, int32 *out_meas, uint8 *out_ok)
{
    if (stage >= PSOC_CAL_STAGE_COUNT || stage >= g_psoc_cal_result_count) { return 0u; }
    if (out_dac  != (int16 *)0) { *out_dac  = g_psoc_cal_results[stage].final_dac; }
    if (out_meas != (int32 *)0) { *out_meas = g_psoc_cal_results[stage].final_measured; }
    if (out_ok   != (uint8 *)0) { *out_ok   = g_psoc_cal_results[stage].ok; }
    return 1u;
}

/* Selecciona un canal del AMux de forma exclusiva y deja el ADC corriendo.
 * with_cap conecta ademas el canal del capacitor (mismo criterio que usa la
 * calibracion para medir una etapa). */
void psoc_selftest_select_channel(uint8 channel, uint8 with_cap)
{
    /* Pedir el capacitor EN PARALELO al propio canal del capacitor no tiene
     * sentido y ademas confunde a los dos trackers, que apuntarian al mismo
     * canal fisico con dos significados distintos. */
#if CAL_AMUX_HAS_CAP_CHANNEL
    if (channel == CAL_AMUX_CAP_CHANNEL) { with_cap = 0u; }
#endif

    /* RECUPERAR EL ADC ANTES DE USARLO, Y NO DESPUES DE QUE FALLE.
     *
     * Observado el 2026-09-05: despues de correr una calibracion, TODAS las
     * medidas de continua del banco empezaron a devolver ok=0, y el unico modo
     * de recuperarlas era un ToggleReset por KitProg. El enlace con el PSoC
     * seguia arriba: lo que quedaba tomado era el camino del ADC.
     *
     * La causa es que la calibracion se apropia del ADC y del ruteo de DMA, y
     * solo los devuelve en cal_async_complete(). Si la corrida ABORTA -y ese
     * dia aborto: contesto ok=0 a los 60 s- ese camino no se recorre y el ADC
     * queda configurado para la calibracion.
     *
     * psoc_selftest_measure_dc() ya llamaba a psoc_selftest_restore(), pero
     * DESPUES de que la lectura fallara, o sea que la primera medida se perdia
     * siempre y, si el estado no se arreglaba solo, las siguientes tambien.
     *
     * Restaurar aca es preventivo y es barato: reclamar el ADC antes de cada
     * seleccion de canal cuesta lo mismo que hacerlo despues de fallar, y evita
     * que una calibracion abortada deje al banco mudo sin explicacion. */
    psoc_calibration_restore_capture_path();

    ADC_Stop();
    psoc_amux_select_exclusive(channel, with_cap);
    ADC_Start();
}

void psoc_selftest_restore(void)
{
    psoc_calibration_restore_capture_path();
}

/* Medicion DC de un canal: promedio de n conversiones completas y
 * reiniciadas (cada una asienta el decimador del DelSig por si sola, que es
 * lo correcto despues de conmutar el AMux). Devuelve media y pico-pico en
 * counts right-aligned. */
uint8 psoc_selftest_measure_dc(uint8 channel, uint16 settle_ms, uint16 n,
                               uint8 with_cap, int32 *out_mean, int32 *out_pp)
{
    int64 sum = 0;
    int32 vmin = 0, vmax = 0;
    uint16 i;

    if (n == 0u || out_mean == (int32 *)0) { return 0u; }

    psoc_selftest_select_channel(channel, with_cap);
    if (settle_ms > 0u) { CyDelay(settle_ms); }

    for (i = 0u; i < n; i++) {
        int32 s;
        if (!st_adc_read_checked(&s)) {
            /* El ADC no convirtio. Abortar: devolver ceros seria decir "0 V,
             * perfectamente estable" sobre una placa rota. */
            psoc_selftest_restore();
            return 0u;
        }
        sum += (int64)s;
        if (i == 0u) { vmin = s; vmax = s; }
        else {
            if (s < vmin) { vmin = s; }
            if (s > vmax) { vmax = s; }
        }
    }

    *out_mean = (int32)(sum / (int64)n);
    if (out_pp != (int32 *)0) { *out_pp = vmax - vmin; }
    return 1u;
}

/* Serie continua para el piso de ruido: no reinicia la conversion entre
 * muestras, asi que respeta la Fs nativa y sirve para estimar amplitud a una
 * frecuencia dada. Calcula todo en streaming (sin buffer): media, RMS,
 * pico-pico y la amplitud a tone_hz por Goertzel.
 *
 * Salidas en counts; el RMS y la amplitud son magnitudes de AC (media ya
 * restada en el caso del RMS). */
uint8 psoc_selftest_measure_series(uint8 channel, uint16 settle_ms, uint16 n,
                                   uint8 with_cap, uint16 tone_hz,
                                   int32 *out_mean, int32 *out_rms,
                                   int32 *out_pp, int32 *out_tone)
{
    int64 sum = 0;
    int64 sumsq = 0;
    int32 vmin = 0, vmax = 0;
    uint16 i;
    uint16 got = 0u;
    double w, cw, sw, coeff;
    double s0 = 0.0, s1 = 0.0, s2 = 0.0;
    double dc_est = 0.0;

    if (n == 0u || out_mean == (int32 *)0) { return 0u; }

    psoc_selftest_select_channel(channel, with_cap);
    if (settle_ms > 0u) { CyDelay(settle_ms); }

    /* Goertzel: k = n*f0/fs, w = 2*pi*k/n = 2*pi*f0/fs */
    w = ST_TWO_PI * (double)tone_hz / (double)PSOC_ADC_NATIVE_FS_HZ;
    st_sincos(w, &sw, &cw);
    coeff = 2.0 * cw;

    /* Descarta las primeras conversiones: el decimador del DelSig arrastra
     * historia del canal anterior. */
    (void)cal_adc_read_direct_counts();

    /* Estimacion de continua para restarsela al Goertzel.
     *
     * Con N multiplo de la Fs el bin cae exacto y la continua se rechaza sola,
     * pero esto cubre el caso de que alguien pida un N cualquiera: sin restarla,
     * la fuga de un offset de decenas de miles de counts (lo normal en un tap
     * sin calibrar) se mide como si fuera senal a 50 Hz. Se estima con las
     * primeras muestras, que es suficiente porque la continua no se mueve en
     * el segundo que dura la medicion. */
    {
        int32 dcs;
        uint16 g2;
        uint16 got_dc = 0u;
        int64 dcsum = 0;
        ADC_StopConvert();
        ADC_StartConvert();
        for (dcs = 0; dcs < (int32)ST_DC_PRE_SAMPLES; dcs++) {
            uint8 rdy = 0u;
            for (g2 = 0u; g2 < (uint16)CAL_ADC_DIRECT_WAIT_POLLS; g2++) {
                if (ADC_IsEndConversion(ADC_RETURN_STATUS)) { rdy = 1u; break; }
            }
            if (!rdy) { break; }
            dcsum += (int64)psoc_adc_counts_right_aligned(ADC_GetResult32());
            got_dc++;
        }
        ADC_StopConvert();
        if (got_dc > 0u) { dc_est = (double)dcsum / (double)got_dc; }
    }

    ADC_StopConvert();
    ADC_StartConvert();
    for (i = 0u; i < n; i++) {
        int32 s;
        uint16 guard;
        uint8 ready = 0u;

        for (guard = 0u; guard < (uint16)CAL_ADC_DIRECT_WAIT_POLLS; guard++) {
            if (ADC_IsEndConversion(ADC_RETURN_STATUS)) { ready = 1u; break; }
        }
        if (!ready) {
            /* Corte a mitad de serie: la ventana ya no es la pedida, asi que
             * ni la normalizacion del Goertzel ni el RMS son comparables con
             * nada. Se aborta en vez de devolver numeros que parecen validos. */
            ADC_StopConvert();
            psoc_selftest_restore();
            return 0u;
        }

        s = psoc_adc_counts_right_aligned(ADC_GetResult32());
        sum += (int64)s;
        sumsq += (int64)s * (int64)s;
        if (got == 0u) { vmin = s; vmax = s; }
        else {
            if (s < vmin) { vmin = s; }
            if (s > vmax) { vmax = s; }
        }

        s0 = coeff * s1 - s2 + ((double)s - dc_est);
        s2 = s1;
        s1 = s0;
        got++;
    }
    ADC_StopConvert();

    /* Se exige la ventana COMPLETA: ver el comentario del corte de arriba. */
    if (got != n) { psoc_selftest_restore(); return 0u; }

    *out_mean = (int32)((double)sum / (double)got);

    if (out_pp != (int32 *)0) { *out_pp = vmax - vmin; }

    if (out_rms != (int32 *)0) {
        /* var = E[x^2] - E[x]^2, en double para no perder la resta. */
        double ex2 = (double)sumsq / (double)got;
        double ex  = (double)sum / (double)got;
        double var = ex2 - ex * ex;
        if (var < 0.0) { var = 0.0; }
        *out_rms = (int32)st_isqrt64((uint64)var);
    }

    if (out_tone != (int32 *)0) {
        /* Magnitud del bin, normalizada a amplitud de pico. El termino DC no
         * molesta porque Goertzel a f0 != 0 lo rechaza solo salvo fuga; con n
         * grande alcanza. */
        double re = s1 - s2 * cw;
        double im = s2 * sw;
        double p  = re * re + im * im;
        if (p < 0.0) { p = 0.0; }
        *out_tone = (int32)(2.0 * (double)st_isqrt64((uint64)p) / (double)got);
    }

    return 1u;
}
