#ifndef PSOC_SELFTEST_H
#define PSOC_SELFTEST_H

/*
 * psoc_selftest.h - Autotest de placa del nodo esclavo (PSoC5LP).
 *
 * Este header vive SOLO en el proyecto AcondicionamientoAnalogicoTest.
 * Se incluye una vez desde main.c y todas sus funciones son static: asi no
 * hace falta registrar un .c nuevo en el .cyprj, que no se edita a mano.
 * El precedente es sd_spi.*, implementado dentro de psoc_hw.c por el mismo
 * motivo.
 *
 * REPARTO DE TAREAS
 * -----------------
 * El PSoC NO secuencia el autotest ni saca conclusiones: expone primitivas
 * atomicas (poner un IDAC, medir un tap del AMux, contar flancos de SYNC) y
 * el ESP32 las orquesta y hace la aritmetica. Dos razones:
 *
 *   1. Cada comando termina rapido, asi que el lazo principal del PSoC sigue
 *      contestando pings entre medicion y medicion. Un secuenciador completo
 *      del lado del PSoC bloquearia el lazo varios segundos y el ESP daria
 *      el enlace por caido.
 *   2. La aritmetica (pendientes, cocientes, tolerancias) es mucho mas facil
 *      de leer y de ajustar del lado del ESP, donde hay punto flotante comodo
 *      y no hay presion de SRAM.
 *
 * ESTIMULO SIN TOCAR EL CIRCUITO
 * ------------------------------
 * Todo el estimulo sale de los cuatro IDAC8 de referencia, que ya existen en
 * el TopDesign, y toda la medicion entra por los cinco canales del AMux_ADC,
 * que tambien ya existen. No se agrega ni un componente ni un pin: el
 * TopDesign queda intacto.
 *
 *   IDAC (etapa)         AMux_ADC (tap)
 *   0 = Vref_PGA          ch0 = salida de PGAgain
 *   1 = Vref_BP           ch1 = salida del pasabanda (BPo)
 *   2 = Vref_ADDER        ch2 = salida del sumador / PGAout (SUMo)
 *   3 = Vref_LP           ch3 = salida del pasabajos (LPo)
 *                         ch4 = AMuxCapacitor (100 nF a Vss)
 *
 * El indice de etapa y el canal que le corresponde salen de
 * g_psoc_cal_stages[] (calibration_tables.h), no de constantes duplicadas
 * aca: en HAMMER hay 2 etapas y en GEO 4, y ese mapeo lo decide esa tabla.
 */

#include "project.h"
#include "psoc_hw.h"
#include "psoc_adc.h"
#include "psoc_nv.h"
#include "calibration.h"

/* -------------------------------------------------------------------------
 * Comandos (ESP -> PSoC). Codigos verificados libres contra la tabla de
 * psoc_hw.h: no chocan con ningun comando ni evento existente.
 * ------------------------------------------------------------------------- */
#define PSOC_CMD_ST_REPORT   0xA0u  /* 1 param: que reporte emitir (ver ST_REP_*) */
#define PSOC_CMD_ST_SYNC     0xA1u  /* 1 param: 1=armar contador de flancos, 0=leer */
#define PSOC_CMD_ST_SET_IDAC 0xA2u  /* 2 params: [etapa 0-3][codigo 0-255] */
#define PSOC_CMD_ST_MEAS_DC  0xA4u  /* 1 param: [settle_sel<<4 | canal] */
#define PSOC_CMD_ST_MEAS_AC  0xA7u  /* 1 param: [n_sel<<4 | canal] */

/* Trama de resultado (PSoC -> ESP), 13 bytes:
 *   [0xAB][0xC5][test_id][status][v0 int32 LE][v1 int32 LE][XOR de 2..11]
 * El parser del ESP hoy manda a _badLen cualquier byte[1] que no conozca,
 * asi que del otro lado hay que agregarle este caso. */
#define PSOC_CTRL_ST_RESULT  0xC5u
#define PSOC_ST_RESULT_BYTES 13u

/* status de la trama */
#define ST_OK       0u
#define ST_ERR      1u
#define ST_REJECTED 2u   /* comando valido pero el estado no lo permite */

/* test_id: reportes de valor unico */
#define ST_ID_IDENTITY   0x01u  /* v0=clase HW, v1=Fs nativa */
#define ST_ID_EEPROM     0x02u  /* v0=slots validos, v1=mascara de slots */
#define ST_ID_SYNC       0x03u  /* v0=flancos contados, v1=nivel actual */
#define ST_ID_SD         0x04u  /* v0=estado; v1=[stage][R1][pads][errores] */
#define ST_ID_ADCCFG     0x05u  /* v0=config 1-4, v1=decimacion */
#define ST_ID_STAGES     0x06u  /* v0=cantidad de etapas, v1=canales del AMux */
#define ST_ID_IRQTRAP    0x07u  /* v0=IRQ inesperadas, v1=hardfaults */
#define ST_ID_BUTTON     0x08u  /* v0=nivel crudo del pin (v1 sin uso) */

/* test_id: por etapa / por canal (nibble bajo = indice) */
#define ST_ID_CAL_BASE   0x10u  /* 0x10|etapa : v0=IDAC final, v1=medida final */
#define ST_ID_IDAC_BASE  0x20u  /* 0x20|etapa : v0=codigo escrito, v1=uV nominales */
#define ST_ID_DC_BASE    0x50u  /* 0x50|canal : v0=media uV, v1=pico-pico uV */
#define ST_ID_ACA_BASE   0x60u  /* 0x60|canal : v0=media uV, v1=RMS uV */
#define ST_ID_ACB_BASE   0x70u  /* 0x70|canal : v0=pico-pico uV, v1=amplitud 50 Hz uV */

/* Tablas de los selectores que viajan en el nibble alto del parametro. */
/* Tope de asentamiento: 1,2 s. Los valores mas largos que habia antes (hasta
 * 12 s) bloqueaban el lazo principal dentro de uart_service, y una trama
 * corrupta con XOR valido alcanzaba para dispararlos: el ESP daria el enlace
 * por caido sin que haya ninguna falla real. */
static const uint16 g_st_settle_ms[8] = { 5u, 30u, 120u, 500u, 1200u, 1200u, 1200u, 1200u };
/* Largos de serie para el Goertzel. Los dos primeros son MULTIPLOS DE LA Fs
 * NATIVA a proposito: con N = m*fs el indice del bin queda k = m*f0, que es
 * entero para cualquier frecuencia entera, el bin cae exacto y el rechazo de
 * la continua es perfecto. Con un N cualquiera (2048, por ejemplo) k = 39,32 a
 * 50 Hz, el bin no cae en su centro, y la fuga de la continua se come la
 * medicion: con un offset de 50000 counts el error medido fue del 61 %. Y los
 * taps sin calibrar tienen justamente offsets de ese orden. */
static const uint16 g_st_series_n[8]  = { 2604u, 5208u, 1302u, 651u, 326u, 163u, 128u, 64u };

/* Cuantas conversiones promedia una medicion DC. Suficientes para bajar el
 * ruido de conmutacion del AMux sin que el comando tarde una eternidad. */
#define ST_DC_AVG_SAMPLES 32u

/* Frecuencia de red que se busca en el piso de ruido. Es informativa: sirve
 * para ver de un vistazo cuanta linea esta captando la cadena. */
#define ST_MAINS_HZ 50u

/* Indice de g_st_series_n[] con N = fs: un segundo justo y bin exacto. */
#define ST_SEL_N_EXACTO 0u

/* -------------------------------------------------------------------------
 * Contador de flancos de SYNC_IN.
 * El test B3 valida el unico cable que no se puede probar de otra forma:
 * ESP GPIO27 -> PSoC SYNC_IN (P0[4]). El ESP arma el contador, conmuta el pin
 * N veces y despues lo lee. Si la cuenta coincide, el cable esta.
 *
 * Solo cuenta con el contador ARMADO: asi no interfiere con el arranque real
 * de captura, que sigue siendo el flanco de subida de siempre.
 * ------------------------------------------------------------------------- */
static volatile uint8  g_st_sync_arm   = 0u;
static volatile uint16 g_st_sync_edges = 0u;

/* Contadores de trampas de IRQ, para el test C2. */
static volatile uint16 g_st_irq_traps  = 0u;
static volatile uint16 g_st_hardfaults = 0u;

static void st_sync_note_edge(void)
{
    if (g_st_sync_arm) { g_st_sync_edges++; }
}

/* -------------------------------------------------------------------------
 * Emisor de la trama de resultado.
 * ------------------------------------------------------------------------- */
static void st_send_result(uint8 test_id, uint8 status, int32 v0, int32 v1)
{
    uint8 f[PSOC_ST_RESULT_BYTES];
    uint32 a = (uint32)v0;
    uint32 b = (uint32)v1;
    uint8 crc = 0u;
    uint8 i;

    f[0]  = 0xABu;
    f[1]  = PSOC_CTRL_ST_RESULT;
    f[2]  = test_id;
    f[3]  = status;
    f[4]  = (uint8)(a & 0xFFu);
    f[5]  = (uint8)((a >> 8) & 0xFFu);
    f[6]  = (uint8)((a >> 16) & 0xFFu);
    f[7]  = (uint8)((a >> 24) & 0xFFu);
    f[8]  = (uint8)(b & 0xFFu);
    f[9]  = (uint8)((b >> 8) & 0xFFu);
    f[10] = (uint8)((b >> 16) & 0xFFu);
    f[11] = (uint8)((b >> 24) & 0xFFu);
    for (i = 2u; i < 12u; i++) { crc ^= f[i]; }
    f[12] = crc;

    psoc_link_put_array(f, (uint16)PSOC_ST_RESULT_BYTES);
}

/* -------------------------------------------------------------------------
 * Reportes compuestos (0xA0).
 * ------------------------------------------------------------------------- */
#define ST_REP_IDENTITY 0u
#define ST_REP_EEPROM   1u
#define ST_REP_CAL      2u
#define ST_REP_ADCCFG   3u
#define ST_REP_IRQ      4u
/* 5 lo atiende main.c (el estado de la SD vive en sus statics). */
#define ST_REP_BUTTON   6u
/* Deja el AMux en el canal de captura y el ADC parado, que es el estado que el
 * resto del firmware espera en IDLE. El autotest lo pide al terminar el grupo
 * analogico: sin esto queda seleccionado el ultimo tap de prueba. */
#define ST_REP_RESTORE  7u

static void st_report_identity(void)
{
    /* El ID de silicio unico no esta expuesto por API del componente, asi que
     * se manda lo que si es verificable y util: la clase de hardware que
     * detecto la compilacion, la Fs nativa y el conteo real de etapas. */
    st_send_result(ST_ID_IDENTITY, ST_OK,
                   (int32)PSOC_HW_CLASS,
                   (int32)PSOC_ADC_NATIVE_FS_HZ);
    st_send_result(ST_ID_STAGES, ST_OK,
                   (int32)psoc_selftest_stage_count(),
                   (int32)psoc_selftest_amux_channel_count());
}

/* Recorre los 9 slots de ganancia de la EEPROM y cuenta cuantos tienen CRC
 * valido. Es SOLO LECTURA: una placa recien armada legitimamente tiene cero
 * slots, y eso no es una falla. */
static void st_report_eeprom(void)
{
    int16 dac[PSOC_NV_CAL_STAGES];   /* codigos CON SIGNO: 0 = referencia en Vref */
    uint8 slot;
    uint8 valid = 0u;
    uint16 mask = 0u;

    for (slot = 0u; slot < PSOC_NV_GAIN_SLOTS; slot++) {
        if (psoc_nv_load_for_gain(slot, dac, PSOC_NV_CAL_STAGES)) {
            valid++;
            mask |= (uint16)(1u << slot);
        }
    }
    st_send_result(ST_ID_EEPROM, ST_OK, (int32)valid, (int32)mask);
}

/* Resultado de la ultima calibracion, etapa por etapa. El ESP decide el
 * veredicto: un codigo pegado a 0 o a 255 significa que el offset de esa
 * etapa no se pudo anular, o sea que hay algo mal en esa etapa. */
static void st_report_cal(void)
{
    uint8 n = psoc_selftest_stage_count();
    uint8 i;

    for (i = 0u; i < n; i++) {
        uint8 dac = 0u, ok = 0u;
        int32 meas = 0;
        if (psoc_selftest_stage_result(i, &dac, &meas, &ok)) {
            st_send_result((uint8)(ST_ID_CAL_BASE | i), ok ? ST_OK : ST_ERR,
                           (int32)dac, meas);
        } else {
            st_send_result((uint8)(ST_ID_CAL_BASE | i), ST_ERR, 0, 0);
        }
    }
}

static void st_report_adccfg(void)
{
    st_send_result(ST_ID_ADCCFG, ST_OK,
                   (int32)psoc_adc_get_config(),
                   (int32)psoc_adc_get_decimation());
}

static void st_report_irq(void)
{
    st_send_result(ST_ID_IRQTRAP,
                   (g_st_irq_traps == 0u && g_st_hardfaults == 0u) ? ST_OK : ST_ERR,
                   (int32)g_st_irq_traps, (int32)g_st_hardfaults);
}

/* Pulsador de calibracion (P2[2]). En la placa nueva ese pin es el pulsador
 * onboard del CY8CKIT-059, por eso en el esquematico de la carrier figura sin
 * conectar.
 *
 * Se reporta solo el nivel crudo del pin. La salida del debouncer vive en el
 * Status Register de superMaquina, cuyo header (status.h) main.c incluye
 * DESPUES de este archivo; alcanzarla desde aca obligaria a duplicar la
 * mascara de bits, y un define repetido que se desincroniza es peor que no
 * tener el dato. El ESP igual detecta un pulsador pegado con el nivel crudo. */
static void st_report_button(void)
{
    st_send_result(ST_ID_BUTTON, ST_OK, (int32)Button_Read(), 0);
}

static void st_handle_report(uint8 what)
{
    switch (what)
    {
        case ST_REP_IDENTITY: st_report_identity(); break;
        case ST_REP_EEPROM:   st_report_eeprom();   break;
        case ST_REP_CAL:      st_report_cal();      break;
        case ST_REP_ADCCFG:   st_report_adccfg();   break;
        case ST_REP_IRQ:      st_report_irq();      break;
        case ST_REP_BUTTON:   st_report_button();   break;
        case ST_REP_RESTORE:
            psoc_selftest_restore();
            st_send_result(ST_ID_IDENTITY, ST_OK, (int32)ST_REP_RESTORE, 0);
            break;
        default:
            st_send_result(ST_ID_IDENTITY, ST_ERR, (int32)what, 0);
            break;
    }
}

/* -------------------------------------------------------------------------
 * Contador de flancos de SYNC (0xA1).
 * ------------------------------------------------------------------------- */
static void st_handle_sync(uint8 arm)
{
    if (arm) {
        g_st_sync_edges = 0u;
        g_st_sync_arm = 1u;
        st_send_result(ST_ID_SYNC, ST_OK, 0, (int32)SYNC_IN_Read());
    } else {
        uint16 n = g_st_sync_edges;
        g_st_sync_arm = 0u;
        st_send_result(ST_ID_SYNC, ST_OK, (int32)n, (int32)SYNC_IN_Read());
    }
}

/* -------------------------------------------------------------------------
 * Escritura de un IDAC de referencia (0xA2).
 * Devuelve el codigo aplicado y la tension nominal que le corresponde, para
 * que el ESP pueda contrastar lo medido contra lo pedido.
 * ------------------------------------------------------------------------- */
static void st_handle_set_idac(uint8 stage, uint8 code)
{
    uint8 id = (uint8)(ST_ID_IDAC_BASE | (stage & 0x0Fu));

    if (!psoc_selftest_write_stage_dac(stage, code)) {
        st_send_result(id, ST_ERR, (int32)stage, (int32)code);
        return;
    }
    st_send_result(id, ST_OK, (int32)code, (int32)psoc_idac_code_to_uv(code));
}

/* -------------------------------------------------------------------------
 * Medicion DC de un tap (0xA4). Parametro: [settle_sel<<4 | canal].
 * El capacitor del AMux se conecta en paralelo, igual que hace la
 * calibracion al medir una etapa: filtra ruido chico sin cambiar el DC.
 * ------------------------------------------------------------------------- */
static void st_handle_meas_dc(uint8 param)
{
    uint8 ch  = (uint8)(param & 0x0Fu);
    uint8 sel = (uint8)((param >> 4) & 0x07u);
    int32 mean = 0, pp = 0;
    uint8 id = (uint8)(ST_ID_DC_BASE | ch);

    if (ch >= psoc_selftest_amux_channel_count()) {
        st_send_result(id, ST_ERR, 0, 0);
        return;
    }
    if (!psoc_selftest_measure_dc(ch, g_st_settle_ms[sel], ST_DC_AVG_SAMPLES,
                                  1u, &mean, &pp)) {
        st_send_result(id, ST_ERR, 0, 0);
        return;
    }
    st_send_result(id, ST_OK,
                   psoc_selftest_counts_to_uv(mean),
                   psoc_selftest_counts_to_uv(pp));
}

/* -------------------------------------------------------------------------
 * Piso de ruido de un tap (0xA7). Parametro: [n_sel<<4 | canal].
 * Emite DOS tramas: (media, RMS) y (pico-pico, amplitud a 50 Hz).
 *
 * Aca el capacitor del AMux NO se conecta: filtraria justo lo que se quiere
 * medir.
 * ------------------------------------------------------------------------- */
static void st_handle_meas_ac(uint8 param)
{
    uint8 ch  = (uint8)(param & 0x0Fu);
    uint8 sel = (uint8)((param >> 4) & 0x07u);
    int32 mean = 0, rms = 0, pp = 0, tone = 0;
    uint8 ida = (uint8)(ST_ID_ACA_BASE | ch);
    uint8 idb = (uint8)(ST_ID_ACB_BASE | ch);

    if (ch >= psoc_selftest_amux_channel_count()) {
        st_send_result(ida, ST_ERR, 0, 0);
        st_send_result(idb, ST_ERR, 0, 0);
        return;
    }
    if (!psoc_selftest_measure_series(ch, g_st_settle_ms[2], g_st_series_n[sel],
                                      0u, (uint16)ST_MAINS_HZ,
                                      &mean, &rms, &pp, &tone)) {
        st_send_result(ida, ST_ERR, 0, 0);
        st_send_result(idb, ST_ERR, 0, 0);
        return;
    }
    st_send_result(ida, ST_OK,
                   psoc_selftest_counts_to_uv(mean),
                   psoc_selftest_counts_to_uv(rms));
    st_send_result(idb, ST_OK,
                   psoc_selftest_counts_to_uv(pp),
                   psoc_selftest_counts_to_uv(tone));
}

#endif /* PSOC_SELFTEST_H */
