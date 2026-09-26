/* Included once by calibration.c: owns the analog mux through the same
 * exclusive-channel helper as diagnostics. The pure controller is host tested. */
#include "control_runtime.h"
#include "control_config.c"

static ControlConfig ctl_config, ctl_staged, ctl_profile_config;
static ControlPI ctl_pi;
static uint8 ctl_state, ctl_profile, ctl_origin, ctl_channel=4u, ctl_active;
static uint8 ctl_selected, ctl_bytes, ctl_band, ctl_scan, ctl_phase;
static uint8 ctl_learning_refinements;
static int16 ctl_learning_slow_step;
static uint8 ctl_learning_have_pos,ctl_learning_have_neg;
static int16 ctl_learning_pos0,ctl_learning_pos1,ctl_learning_neg0,ctl_learning_neg1;
static uint8 ctl_learning_had_profile;
/* Ultima lectura de SUMo con la que el aprendizaje decidio: sirve para
 * saber si la cadena se aquieto antes de dar el paso siguiente. */
static int32 ctl_learning_prev_sum;
static uint32 ctl_learning_prev_ms;
/* Objetivo de SUMo del aprendizaje.  Arranca en CP_OPA_TARGET_UV y se
 * biseca si con ese valor LPo queda fuera de su banda: ctl_lp_sum_pos y
 * ctl_lp_sum_neg guardan con que SUMo quedo LPo de cada lado. */
static int32 ctl_learn_target, ctl_lp_sum_pos, ctl_lp_sum_neg;
static uint8 ctl_lp_have_pos, ctl_lp_have_neg, ctl_target_libre;
/* Paso de la EXPANSION del objetivo de SUMo, que se duplica hasta que LPo
 * cambia de signo.  0 = todavia no se expandio. */
static int32 ctl_target_paso;
/* Mejor |LPo| alcanzado en la visita actual a la fase 2. */
static int32 ctl_lp_mejor;
/* Correccion MEDIDA del modelo de pendiente del par lento, en milesimas.
 * 1000 = el modelo de fabrica.  Es propiedad del punto de ganancia, asi
 * que se reinicia al cambiar de ganancia, no en cada aprendizaje. */
static int32 ctl_slow_gain_x1000=1000;
static int32 ctl_slow_pred, ctl_slow_sum0;
static uint8 ctl_slow_armado;
/* Desde cuando la cadena esta donde tiene que estar (0 = no lo esta). */
static uint32 ctl_bueno_desde;
#define CTL_LEARN_QUIETO_MS 15000u
/* Desde cuando SUMo esta fuera de su ventana (0 = esta bien). */
static uint32 ctl_sum_malo_desde;
#define CTL_SUM_MUERTO_MS 120000u
static uint32 ctl_lp_malo_desde;
#define CTL_LP_MUERTO_MS   90000u
static uint32 ctl_word, ctl_last_report, ctl_last_scan, ctl_last_pi;
static uint32 ctl_learning_start, ctl_phase_start, ctl_stable_start, ctl_enter_start;
static uint32 ctl_revision, ctl_generation;
static int16 ctl_codes[4], ctl_saved_codes[4], ctl_learning_restore[4];
static uint32 ctl_autosave_ms;
static uint16 ctl_autosave_n;
static int32 ctl_dc[5], ctl_anchor[5];
static uint32 ctl_stamp[5];
static uint8 ctl_valid[5], ctl_seen[5];
static volatile uint32 ctl_ms;
static volatile int64_t ctl_sum;
static volatile uint16 ctl_count, ctl_discard;
static volatile uint8 ctl_ready;
static volatile int32 ctl_average;
static uint8 ctl_nv_slot;
static uint8 ctl_image[256];
static uint8 ctl_initialized;
static uint8 ctl_capture_metadata[CONTROL_CAPTURE_METADATA_BYTES];
/* LPo trace: every fresh LPo average (~20 ms) with its timestamp, to find
 * periodic disturbances without an oscilloscope.  `ctl get 255` dumps it. */
#define CTL_TRACE_LEN 127u
/* Minimo entre guardados automaticos del punto de trabajo.  Con 10 min el peor
 * caso son 144 escrituras por dia, alternando las dos ranuras: la EEPROM del
 * PSoC5 aguanta 1e6 ciclos por fila, o sea decadas. */
#define CTL_AUTOSAVE_MIN_MS 600000u
static int32 ctl_trace_v[CTL_TRACE_LEN];
static uint32 ctl_trace_ms[CTL_TRACE_LEN];
static uint8 ctl_trace_head, ctl_trace_full;
static uint32 ctl_report_start, ctl_quiet_until;
/* Median of the last 5 averages per channel: the chain shows 40 ms dips of
 * -30 mV every ~2 s (PSoC trace, 2026-09-16) that an average cannot remove. */
#define CTL_MEDIAN_LEN 5u
static int32 ctl_med[CTL_MEDIAN_LEN];
static uint8 ctl_med_n, ctl_med_i;
static int32 ctl_median(void)
{
    int32 t[CTL_MEDIAN_LEN],x;uint8 i,j;
    memcpy(t,ctl_med,sizeof t);
    for(i=1;i<CTL_MEDIAN_LEN;++i){x=t[i];for(j=i;j>0&&t[j-1]>x;--j)t[j]=t[j-1];t[j]=x;}
    return t[CTL_MEDIAN_LEN/2u];
}
static uint16 ctl_report_count;
void control_copy_capture_metadata(uint8 *out)
{ memcpy(out,ctl_capture_metadata,CONTROL_CAPTURE_METADATA_BYTES); }

void control_tick(void) { ++ctl_ms; }
uint32 control_millis(void) { return ctl_ms; }
static void ctl_put32(uint8 *p,uint32 x)
{ p[0]=(uint8)x;p[1]=(uint8)(x>>8);p[2]=(uint8)(x>>16);p[3]=(uint8)(x>>24); }
static uint32 ctl_get32(const uint8 *p)
{ return (uint32)p[0]|((uint32)p[1]<<8)|((uint32)p[2]<<16)|((uint32)p[3]<<24); }
static uint8 ctl_load_slot(uint8 slot)
{
    uint16 i,base=(uint16)(256u+256u*slot);
    for(i=0;i<256;++i) ctl_image[i]=EEPROM_ReadByte(base+i);
    return ctl_image[0]==0x47 && ctl_image[1]==0x43 &&
        /* Toda version conocida se acepta y se decodifica con SU cantidad de
         * parametros (ver control_init).  Si aca solo se aceptara la version
         * vigente, cada vez que se agrega un parametro el nodo tiraria el
         * perfil aprendido y arrancaria en FALTA_APRENDER. */
        (ctl_image[2]==3u || ctl_image[2]==4u || ctl_image[2]==5u ||
         ctl_image[2]==CONTROL_NV_VERSION) &&
        ctl_image[3]==PSOC_HW_CLASS &&
        control_crc(ctl_image,254)==((uint16)ctl_image[254]|((uint16)ctl_image[255]<<8));
}
static uint8 ctl_store(void)
{
    uint8 slot=(uint8)(1u-ctl_nv_slot),row;
    uint16 crc,i;
    memset(ctl_image,0,sizeof ctl_image);
    ctl_image[0]=0x47;ctl_image[1]=0x43;ctl_image[2]=CONTROL_NV_VERSION;ctl_image[3]=PSOC_HW_CLASS;
    ctl_put32(ctl_image+4,ctl_generation+1);
    control_config_encode(&ctl_config,ctl_image+8);
    i=8+CONTROL_CONFIG_BYTES;
    ctl_image[i++]=ctl_profile;
    for(row=0;row<4;++row) { ctl_image[i++]=(uint8)ctl_saved_codes[row];ctl_image[i++]=(uint8)((uint16)ctl_saved_codes[row]>>8); }
    ctl_put32(ctl_image+i,ctl_revision);
    crc=control_crc(ctl_image,254);ctl_image[254]=(uint8)crc;ctl_image[255]=(uint8)(crc>>8);
    EEPROM_UpdateTemperature();
    /* Invalidate destination before payload. Commit header LAST. Old slot
     * remains untouched on every failure or interrupted write. */
    { uint8 zero[16]={0}; if(EEPROM_Write(zero,(uint8)(16+slot*16))!=CYRET_SUCCESS) return 0; }
    for(row=1;row<16;++row)
        if(EEPROM_Write(ctl_image+16*row,(uint8)(16+slot*16+row))!=CYRET_SUCCESS) return 0;
    if(EEPROM_Write(ctl_image,(uint8)(16+slot*16))!=CYRET_SUCCESS) return 0;
    if(!ctl_load_slot(slot)) return 0;
    ctl_nv_slot=slot; ++ctl_generation;ctl_origin=1;return 1;
}
static void ctl_emit(uint16 key,int32 value,uint8 capture)
{
    uint8 f[11],i,x=0;
    f[0]=0xAB;f[1]=0xC6;f[2]=CONTROL_CONFIG_VERSION;f[3]=capture;
    f[4]=(uint8)key;f[5]=(uint8)(key>>8);ctl_put32(f+6,(uint32)value);
    for(i=2;i<10;++i)x^=f[i];f[10]=x;psoc_link_put_array(f,11);
    ctl_quiet_until=ctl_ms+(uint32)ctl_config.value[CP_QUIET_MS];
}
static void ctl_emit_trace(void)
{
    uint8 i,n=ctl_trace_full?CTL_TRACE_LEN:ctl_trace_head;
    uint8 idx=ctl_trace_full?ctl_trace_head:0u;
    uint32 prev=0;
    ctl_emit(0x17D,(int32)ctl_report_start,0);
    ctl_emit(0x17E,(int32)ctl_last_report,0);
    ctl_emit(0x17F,(int32)(n?ctl_trace_ms[(uint8)((idx+n-1u)%CTL_TRACE_LEN)]:0),0);
    for(i=0;i<n;++i) {
        uint8 k=(uint8)((idx+i)%CTL_TRACE_LEN);
        uint32 dt=i?ctl_trace_ms[k]-prev:0u;
        int32 mv10=ctl_trace_v[k]/10;
        if(dt>4095u)dt=4095u;
        if(mv10>524287)mv10=524287;
        if(mv10<-524288)mv10=-524288;
        prev=ctl_trace_ms[k];
        ctl_emit((uint16)(0x180+i),(int32)((dt<<20)|((uint32)mv10&0xFFFFFu)),0);
    }
}
/* Definida mas abajo, junto a la tabla de puntos: el snapshot vive antes que
 * ella en este archivo y en C hay que declararla primero. */
static void ctl_punto_emitir(uint8 capture);
void control_emit_snapshot(uint8 capture)
{
    uint16 i;uint32 now=ctl_ms;
    /* The 36 configuration words only change on apply: sending them every
     * report was 36 of 72 I2C frames per second for nothing. */
    uint8 with_config=capture || (ctl_report_count%30u)==0u;
    ++ctl_report_count;
    ctl_emit(0x100,ctl_state,capture);ctl_emit(0x101,ctl_profile,capture);
    ctl_emit(0x102,ctl_band,capture);ctl_emit(0x103,ctl_origin,capture);
    ctl_emit(0x104,ctl_revision,capture);ctl_emit(0x105,(int32)now,capture);
    ctl_emit(0x106,CONTROL_CONFIG_VERSION,capture);
    ctl_emit(0x107,control_crc((const uint8 *)(__DATE__ " " __TIME__),sizeof(__DATE__ " " __TIME__)-1),capture);
    if(with_config)for(i=0;i<CP_COUNT;++i)ctl_emit(i,ctl_config.value[i],capture);
    /* Igual que la config: NO va en cada reporte.  Cada trama es una patada de
     * +-50 mV a LPo (medido 2026-09-16), y el origen del punto solo cambia en
     * un cambio de ganancia, donde ctl_punto_aplicar() lo emite como evento. */
    if(with_config)ctl_punto_emitir(capture);
    for(i=0;i<4;++i)ctl_emit((uint16)(0x110+i),ctl_codes[i],capture);
    for(i=0;i<5;++i) {
        int32 margin=ctl_dc[i]-(ctl_config.value[CP_VALID_LOW_UV]+ctl_config.value[CP_MARGIN_UV]);
        int32 upper=(ctl_config.value[CP_VALID_HIGH_UV]-ctl_config.value[CP_MARGIN_UV])-ctl_dc[i];
        if(upper<margin)margin=upper;
        ctl_emit((uint16)(0x120+i*4),ctl_dc[i],capture);
        ctl_emit((uint16)(0x121+i*4),ctl_seen[i]?ctl_valid[i]:0,capture);
        ctl_emit((uint16)(0x122+i*4),ctl_seen[i]?(int32)(now-ctl_stamp[i]):-1,capture);
        ctl_emit((uint16)(0x123+i*4),ctl_seen[i]?margin:INT32_MIN,capture);
    }
    ctl_emit(0x1ff,1,capture);
}
static void ctl_select(uint8 channel)
{
    uint8 saved;
    ADC_StopConvert();ctl_active=0;
    psoc_amux_select_exclusive(channel,(uint8)ctl_config.value[CP_CAPACITOR]);
    saved=CyEnterCriticalSection();
    ctl_channel=channel;ctl_sum=0;ctl_count=0;ctl_ready=0;ctl_med_n=0;ctl_med_i=0;
    ctl_discard=(uint16)ctl_config.value[CP_SETTLE_SAMPLES];
    ctl_active=1;CyExitCriticalSection(saved);ADC_StartConvert();
}
void control_on_filtered_sample(int32 counts)
{
    if(!ctl_active)return;
    if(ctl_discard){--ctl_discard;return;}
    ctl_sum+=counts;
    if(++ctl_count >= (uint16)ctl_config.value[CP_AVERAGE]) {
        ctl_average=(int32)(ctl_sum/ctl_count);ctl_ready=1;
        ctl_sum=0;ctl_count=0;
    }
}
static void ctl_route_start(void)
{
    ADC_StopConvert();psoc_adc_select_calibration_config();
    (void)psoc_filter_load_fir_coefficients(g_fir_calibration_coeffs_q23,FILTER_FIR_NTAPS);
    dma_route_select(1u);ctl_scan=1;ctl_select(0);
}
static void ctl_write(uint8 stage,int16 code)
{
    if(ctl_state==CONTROL_PAUSED)return;
    if(psoc_calibration_set_stage_dac(stage,code))ctl_codes[stage]=code;
}
/* Canal de VISTA: 0xFF = ninguno, o sea el canal de captura de la config.
 * Deliberadamente NO vive en ControlConfig: es diagnostico, no calibracion,
 * y si estuviera en la config el autosave lo escribiria en la EEPROM y la
 * placa arrancaria en frio mirando una etapa del medio de la cadena. */
static uint8 ctl_view_channel = PSOC_VIEW_CHANNEL_NONE;
uint8 control_capture_channel(void)
{
    if(ctl_view_channel!=PSOC_VIEW_CHANNEL_NONE)return ctl_view_channel;
    return (uint8)ctl_config.value[CP_CAPTURE_CHANNEL];
}
/* Segundos de forzado tras un cambio de ganancia. Con tau ~34 s, 150 s son
 * mas de cuatro constantes: alcanza para que la cola lenta del acople llegue
 * y el lazo cierre sobre el valor de regimen, no sobre el transitorio. */
/* 300 s: medido el 2026-09-18, recuperar un SUMo de ~100 mV pide una decena
 * de codigos de IDAC2 y cada uno arrastra la cola lenta del pasabanda. */
#define CTL_GAIN_FORCE_S 300u
/* Forzado del arranque en frio: alcanza con menos que el de cambio de
 * ganancia porque el punto guardado suele estar cerca. */
#define CTL_BOOT_FORCE_S 90u
/* -- Memoria de puntos de trabajo por par de ganancias ---------------------
 * Cambiar de ganancia deja de ser una busqueda a ciegas.  Cada par que llego a
 * banda deja sus cuatro codigos guardados; al volver a ese par se salta
 * directamente ahi y el lazo solo ajusta, en vez de rescatar desde un riel.
 * Medido el 2026-09-18: un cambio de ganancia sin esto tardaba de 30 s a 8 min
 * segun de donde viniera, y a PGA alto ni siquiera llegaba.
 *
 * INVARIANTE, y no se toca: el punto guardado es una CONDICION INICIAL, nunca
 * un reemplazo del control.  control_on_gain_change() abre la ventana de
 * forzado ANTES de restaurar, asi que el punto entra con el lazo corrigiendo a
 * fondo.  Nada de lo que hay aca congela el lazo ni lo deja sin corregir: eso
 * es lo que permite al nodo absorber temperatura, envejecimiento y deriva del
 * acople, que no estan modelados en ninguna tabla.
 *
 * Medido sobre 191 visitas de banco (2026-09-22/23): de los 90 PASA, los que
 * restauraron un punto entraron en 2,0 s y los que aprendieron de cero
 * tardaron de 57 a 959 s.  Rapido y bueno son el mismo caso. */
/* Codigo de PGA (x16) a partir del cual la busqueda a ciegas del par lento
 * ya no encuentra la ventana de SUMo y hay que sembrar desde un vecino. */
#define CTL_PGA_SIEMBRA 4u

/* De donde salio el punto de trabajo vigente.  Sin esto es IMPOSIBLE
 * distinguir "la semilla sirvio" de "venia bien encaminado", y una campania
 * entera no produce evidencia. */
enum { CTL_ORIGEN_NADA=0u, CTL_ORIGEN_APRENDIZAJE, CTL_ORIGEN_EEPROM,
       CTL_ORIGEN_VECINO, CTL_ORIGEN_INTERPOLADO, CTL_ORIGEN_SIEMBRA,
       CTL_ORIGEN_PERFIL };
/* Politica de CP_PUNTO_POLICY.  Cada nivel cae al siguiente. */
enum { CTL_POL_OFF=0u, CTL_POL_UNICO, CTL_POL_VECINO, CTL_POL_INTERPOLADO };

static uint8 ctl_punto_origen;
/* Codigos con los que arranco el par: la base del RESIDUO de correccion
 * (cuanto hubo que corregir desde el valor modelado).  El host lo resta
 * contra los codigos finales y con eso se mide si el modelo sirve, de que
 * depende el error y cuanto deriva la placa entre dias. */
static int16 ctl_punto_semilla[4];

/* ---- Tabla persistente: un registro por par, los 81 ----------------------
 * La tabla vivia en RAM con OCHO lugares y rotacion circular: nueve de cada
 * diez pares perdian su punto antes de volver a visitarlos.  Ahora entra
 * entera y se indexa directo, sin buscar.
 *
 * Mapa de EEPROM (2 KiB = 128 filas de 16 B): 0..8 psoc_nv por codigo de PGA,
 * 16..47 el perfil (dos slots), 48..52 la tabla VIEJA de ocho (solo lectura,
 * para migrar), 53..123 esta tabla, 124 su cabecera.
 *
 * El CRC es POR REGISTRO y no global: guardar toca una o dos filas (~40 ms)
 * en vez de las cinco de antes (~100 ms), y un corte de luz corrompe un
 * registro, no la tabla entera. */
#define CTL_PT_N        81u
#define CTL_PT_REC      14u
#define CTL_PT_ROW0     53u
#define CTL_PT_BASE     ((uint16)CTL_PT_ROW0*16u)
#define CTL_PT_HDR_ROW  124u
#define CTL_PT_MAGIC0   0x47u
#define CTL_PT_MAGIC1   0x54u
#define CTL_PT_VER      2u
/* Region de la tabla vieja de ocho, de la que se migra una sola vez. */
#define CTL_PUNTO_ROW0   48u
#define CTL_PUNTO_ROWS    5u
#define CTL_PUNTO_BYTES  (CTL_PUNTO_ROWS*16u)
/* Un IDAC pasado esto esta contra su tope: el punto puede estar en banda y
 * aun asi no tener con que corregir.  Medido 2026-09-23: el 28 % de los
 * puntos "buenos" del historico tenian IDAC3 pasado 200. */
#define CTL_PT_TOPE     200
/* Fallos seguidos antes de tirar un punto.  No se tira al primero: el mismo
 * punto puede fallar por venir de otro contexto y servir desde otro. */
#define CTL_PT_FALLOS_MAX 3u

typedef struct {
    int16 codes[4];
    int16 lpo_mv;
    /* b0 valido, b1 promovido, b2..b4 aciertos (0..7), b5..b7 fallos (0..7) */
    uint8 estado;
    /* Residuo de correccion, saturado: lo que el lazo tuvo que mover desde la
     * semilla.  Deja que el PROPIO nodo pre-compense la proxima siembra, sin
     * depender del analisis en la PC. */
    int8  res_slow, res_fast;
} CtlPunto;
static CtlPunto ctl_pt[CTL_PT_N];
/* Indice del registro pendiente de bajar a EEPROM (0xFF = ninguno). */
static uint8 ctl_punto_sucio=0xFFu;
static uint8 ctl_pga_prev=0xFFu, ctl_pgaout_prev=0xFFu;

/* ---- Pool de candidatos con CONTEXTO, en RAM ----------------------------
 * La EEPROM da para UN punto por par.  Pero los cuatro codigos no definen el
 * punto de trabajo: el acople de 680 uF y su absorcion dielectrica hacen que
 * los mismos codigos aterricen distinto segun de donde se venga (medido el
 * 2026-09-19 a x50/x1).  O sea que el punto bueno depende del ESTADO FISICO
 * de entrada, no del par anterior.
 *
 * Por eso el contexto es {SUMo medido al entrar, codigos del par lento}, que
 * es el proxy de la carga del acople, y NO un indice de "de que par venis":
 * eso ultimo serian 81x81 casos y se sobreajustaria al orden del barrido.
 *
 * Vive en RAM y no en EEPROM porque despues de un reset se viene de UN solo
 * contexto -el frio- y ahi alcanza con el punto persistente. */
#define CTL_CAND_N 96u
typedef struct {
    uint8 par;              /* 0xFF = libre */
    uint8 usos;
    int16 codes[4];
    int16 ctx_sum_mv, ctx_i0, ctx_i1;
    int16 lpo_mv;
} CtlCand;
static CtlCand ctl_cand[CTL_CAND_N];
static uint8 ctl_cand_sig;
/* Contexto con el que se ENTRO al par vigente. */
static int16 ctl_ctx_sum_mv, ctl_ctx_i0, ctl_ctx_i1;

#define CTL_PT_IDX(pga,pgaout) ((uint8)((uint8)(pga)*9u+(uint8)(pgaout)))
static uint8 ctl_pt_valido(const CtlPunto *q){return (uint8)(q->estado&1u);}
static uint8 ctl_pt_aciertos(const CtlPunto *q){return (uint8)((q->estado>>2)&7u);}
static uint8 ctl_pt_fallos(const CtlPunto *q){return (uint8)((q->estado>>5)&7u);}
static void ctl_pt_set_aciertos(CtlPunto *q,uint8 n)
{ if(n>7u)n=7u; q->estado=(uint8)((q->estado&(uint8)(~(7u<<2)))|(uint8)(n<<2)); }
static void ctl_pt_set_fallos(CtlPunto *q,uint8 n)
{ if(n>7u)n=7u; q->estado=(uint8)((q->estado&(uint8)(~(7u<<5)))|(uint8)(n<<5)); }

/* Con la politica en OFF la memoria de puntos queda COMPLETAMENTE inerte: no
 * restaura, no guarda, no anota candidatos y no degrada entradas.  La guarda
 * va DENTRO de cada funcion y no en los sitios de llamada, porque los sitios
 * son once y alcanza con olvidarse de uno.
 *
 * Que no guarde es tan importante como que no restaure: si el brazo de
 * CONTROL de un experimento sigue escribiendo la tabla, esta alimentando la
 * memoria que el brazo de MEMORIA va a usar despues, y los dos brazos dejan
 * de ser independientes.  Medido el 2026-09-25. */
static uint8 ctl_punto_activa(void)
{ return (uint8)(ctl_config.value[CP_PUNTO_POLICY]>(int32)CTL_POL_OFF); }

/* Puntuacion de un punto: MENOR es mejor.  No alcanza con |LPo|: un punto
 * centrado pero con IDAC3 contra el tope esta en banda y sin autoridad, y al
 * primer cambio de temperatura se cae.  Guardar por "mejor |LPo|" propagaba
 * justamente esos. */
static int32 ctl_punto_score(const int16 *codes,int16 lpo_mv)
{
    int32 s=2*magnitude((int32)lpo_mv);
    int32 a2=magnitude((int32)codes[2]), a3=magnitude((int32)codes[3]);
    if(a3>CTL_PT_TOPE)s+=(a3-CTL_PT_TOPE)*5;
    if(a2>CTL_PT_TOPE)s+=(a2-CTL_PT_TOPE)*5;
    return s;
}

static uint8 ctl_pt_crc(const uint8 *b,uint8 idx)
{
    uint16 c=control_crc(b,(unsigned)(CTL_PT_REC-1u));
    c=(uint16)(c^((uint16)idx*0x101u));
    return (uint8)(c^(c>>8));
}
static void ctl_pt_pack(uint8 i,uint8 *b)
{
    const CtlPunto *q=&ctl_pt[i];uint8 j,p=0;
    for(j=0;j<4;++j){b[p++]=(uint8)q->codes[j];b[p++]=(uint8)((uint16)q->codes[j]>>8);}
    b[p++]=(uint8)q->lpo_mv;b[p++]=(uint8)((uint16)q->lpo_mv>>8);
    b[p++]=q->estado;b[p++]=(uint8)q->res_slow;b[p++]=(uint8)q->res_fast;
    b[p]=ctl_pt_crc(b,i);
}
static uint8 ctl_pt_unpack(uint8 i,const uint8 *b)
{
    CtlPunto *q=&ctl_pt[i];uint8 j,p=0;int16 c[4];
    if(b[CTL_PT_REC-1u]!=ctl_pt_crc(b,i))return 0u;
    for(j=0;j<4;++j){
        c[j]=(int16)((uint16)b[p]|((uint16)b[p+1]<<8));p=(uint8)(p+2u);
        if(c[j]<-255||c[j]>255)return 0u;
    }
    for(j=0;j<4;++j)q->codes[j]=c[j];
    q->lpo_mv=(int16)((uint16)b[p]|((uint16)b[p+1]<<8));p=(uint8)(p+2u);
    q->estado=b[p++];q->res_slow=(int8)b[p++];q->res_fast=(int8)b[p];
    return 1u;
}
/* Baja UN registro: lee las una o dos filas que lo contienen, le pisa sus
 * catorce bytes y las devuelve.  Nunca reescribe la tabla entera. */
static void ctl_pt_store_one(uint8 i)
{
    uint8 rec[CTL_PT_REC],buf[32];
    uint16 off;
    uint8 row0,row1,r,k,filas,desp;
    if(i>=CTL_PT_N)return;
    off=(uint16)(CTL_PT_BASE+(uint16)CTL_PT_REC*(uint16)i);
    row0=(uint8)(off/16u);row1=(uint8)((off+CTL_PT_REC-1u)/16u);
    filas=(uint8)(row1-row0+1u);desp=(uint8)(off-(uint16)row0*16u);
    ctl_pt_pack(i,rec);
    for(k=0;k<(uint8)(16u*filas);++k)
        buf[k]=EEPROM_ReadByte((uint16)((uint16)row0*16u+k));
    for(k=0;k<CTL_PT_REC;++k)buf[desp+k]=rec[k];
    EEPROM_UpdateTemperature();
    for(r=0;r<filas;++r)
        if(EEPROM_Write(buf+16u*r,(uint8)(row0+r))!=CYRET_SUCCESS)return;
}
static void ctl_pt_store_hdr(void)
{
    uint8 b[16],i;uint16 c;
    for(i=0;i<16u;++i)b[i]=0u;
    b[0]=CTL_PT_MAGIC0;b[1]=CTL_PT_MAGIC1;b[2]=CTL_PT_VER;b[3]=PSOC_HW_CLASS;
    c=control_crc(b,14u);b[14]=(uint8)c;b[15]=(uint8)(c>>8);
    EEPROM_UpdateTemperature();
    (void)EEPROM_Write(b,CTL_PT_HDR_ROW);
}
static uint8 ctl_pt_hdr_ok(void)
{
    uint8 b[16],i;uint16 c;
    for(i=0;i<16u;++i)b[i]=EEPROM_ReadByte((uint16)((uint16)CTL_PT_HDR_ROW*16u+i));
    if(b[0]!=CTL_PT_MAGIC0||b[1]!=CTL_PT_MAGIC1||b[2]!=CTL_PT_VER||
       b[3]!=PSOC_HW_CLASS)return 0u;
    c=control_crc(b,14u);
    return (uint8)((b[14]==(uint8)c && b[15]==(uint8)(c>>8))?1u:0u);
}
/* Migracion desde la tabla vieja de ocho lugares.  Corre una sola vez, cuando
 * la region nueva todavia no tiene cabecera: sin esto el primer arranque con
 * el firmware nuevo tiraria los ocho puntos que ya costaron horas. */
static void ctl_pt_migrar_de_v1(void)
{
    uint8 buf[CTL_PUNTO_BYTES];
    uint16 i;uint8 j,p=4u;
    for(i=0;i<CTL_PUNTO_BYTES;++i)
        buf[i]=EEPROM_ReadByte((uint16)((uint16)CTL_PUNTO_ROW0*16u+i));
    if(buf[0]!=0x47u||buf[1]!=0x50u||buf[2]!=1u||buf[3]!=PSOC_HW_CLASS)return;
    if(control_crc(buf,CTL_PUNTO_BYTES-2u)!=
       ((uint16)buf[CTL_PUNTO_BYTES-2u]|((uint16)buf[CTL_PUNTO_BYTES-1u]<<8)))return;
    for(i=0;i<8u;++i) {
        uint8 g=buf[p++],malo=0u,idx;
        int16 c[4];
        for(j=0;j<4;++j) {
            c[j]=(int16)((uint16)buf[p]|((uint16)buf[p+1]<<8));p=(uint8)(p+2u);
            if(c[j]<-255||c[j]>255)malo=1u;
        }
        if(g==0xFFu||malo||(g>>4)>8u||(g&0x0Fu)>8u)continue;
        idx=CTL_PT_IDX(g>>4,g&0x0Fu);
        for(j=0;j<4;++j)ctl_pt[idx].codes[j]=c[j];
        ctl_pt[idx].lpo_mv=0;ctl_pt[idx].estado=1u;
        ctl_pt[idx].res_slow=0;ctl_pt[idx].res_fast=0;
        ctl_pt_store_one(idx);
    }
}
static void ctl_punto_load(void)
{
    uint8 i,j,rec[CTL_PT_REC];
    for(i=0;i<CTL_PT_N;++i) {
        for(j=0;j<4;++j)ctl_pt[i].codes[j]=0;
        ctl_pt[i].lpo_mv=0;ctl_pt[i].estado=0u;
        ctl_pt[i].res_slow=0;ctl_pt[i].res_fast=0;
    }
    for(i=0;i<CTL_CAND_N;++i)ctl_cand[i].par=0xFFu;
    if(!ctl_pt_hdr_ok()) {
        /* Region virgen: se estrena y se migra lo que haya de la tabla vieja.
         * Si algo de esto falla se arranca con la tabla vacia, que es
         * exactamente el comportamiento anterior. */
        ctl_pt_store_hdr();
        ctl_pt_migrar_de_v1();
        return;
    }
    for(i=0;i<CTL_PT_N;++i) {
        uint16 off=(uint16)(CTL_PT_BASE+(uint16)CTL_PT_REC*(uint16)i);
        for(j=0;j<CTL_PT_REC;++j)rec[j]=EEPROM_ReadByte((uint16)(off+j));
        /* Un registro con CRC malo o codigos imposibles se descarta SOLO;
         * el resto de la tabla sigue sirviendo. */
        if(!ctl_pt_unpack(i,rec))ctl_pt[i].estado=0u;
    }
}
static void ctl_punto_store(void)
{
    if(ctl_punto_sucio>=CTL_PT_N)return;
    ctl_pt_store_one(ctl_punto_sucio);
    ctl_punto_sucio=0xFFu;
}

/* ---- API de la tabla, con los mismos nombres de siempre ----------------- */

static void ctl_ctx_tomar(void)
{
    ctl_ctx_sum_mv=(int16)((ctl_seen[3]&&ctl_valid[3])?ctl_dc[3]/1000:0);
    ctl_ctx_i0=ctl_codes[0];ctl_ctx_i1=ctl_codes[1];
}
/* Anota el punto ACTUAL como candidato del par, con el contexto de entrada.
 * Solo se llama con el lazo ASENTADO (mismo criterio que el autosave): una
 * muestra en transitorio mentiria.  Medido: esperar 1,5 s sobre una planta de
 * tau=34 s da 18,8 mV/codigo cuando en regimen son 12,3. */
static void ctl_cand_anotar(uint8 par)
{
    if(!ctl_punto_activa())return;
    uint8 i,libre=CTL_CAND_N,j;
    int16 lpo=(int16)(ctl_dc[4]/1000);
    for(i=0;i<CTL_CAND_N;++i) {
        if(ctl_cand[i].par==par &&
           magnitude((int32)ctl_cand[i].ctx_sum_mv-(int32)ctl_ctx_sum_mv)<=5)break;
        if(ctl_cand[i].par==0xFFu && libre==CTL_CAND_N)libre=i;
    }
    if(i==CTL_CAND_N) {
        i=(libre<CTL_CAND_N)?libre:ctl_cand_sig;
        ctl_cand_sig=(uint8)((ctl_cand_sig+1u)%CTL_CAND_N);
        ctl_cand[i].usos=0u;
    } else if(ctl_cand[i].usos<255u)++ctl_cand[i].usos;
    ctl_cand[i].par=par;
    for(j=0;j<4;++j)ctl_cand[i].codes[j]=ctl_codes[j];
    ctl_cand[i].ctx_sum_mv=ctl_ctx_sum_mv;
    ctl_cand[i].ctx_i0=ctl_ctx_i0;ctl_cand[i].ctx_i1=ctl_ctx_i1;
    ctl_cand[i].lpo_mv=lpo;
}
static uint8 ctl_cand_n_par(uint8 par)
{
    uint8 i,n=0u;
    for(i=0;i<CTL_CAND_N;++i)if(ctl_cand[i].par==par)++n;
    return n;
}
/* Nivel 2: el candidato cuyo contexto se parece mas al de ahora.  SUMo pesa
 * cuatro veces mas que un codigo porque es la medicion DIRECTA del estado del
 * acople; los codigos del par lento solo lo acompanian. */
static uint8 ctl_cand_vecino(uint8 par,uint8 *out)
{
    uint8 i,best=0xFFu;int32 bd=INT32_MAX;
    for(i=0;i<CTL_CAND_N;++i) {
        int32 d;
        if(ctl_cand[i].par!=par)continue;
        d=4*magnitude((int32)ctl_cand[i].ctx_sum_mv-(int32)ctl_ctx_sum_mv)
          + magnitude((int32)ctl_cand[i].ctx_i0-(int32)ctl_ctx_i0)
          + magnitude((int32)ctl_cand[i].ctx_i1-(int32)ctl_ctx_i1);
        if(d<bd){bd=d;best=i;}
    }
    if(best>=CTL_CAND_N)return 0u;
    *out=best;return 1u;
}
/* Nivel 3: recta entre los dos candidatos que ENCIERRAN el contexto actual.
 * INTERPOLA, nunca extrapola: si no hay uno de cada lado devuelve 0 y se cae
 * al vecino.  Con pocos puntos una recta extrapolada se va contra el riel. */
static uint8 ctl_cand_interp(uint8 par,int16 *out)
{
    uint8 i,lo=0xFFu,hi=0xFFu,j;
    int32 dlo=INT32_MAX,dhi=INT32_MAX,x0,x1,dx;
    for(i=0;i<CTL_CAND_N;++i) {
        int32 d;
        if(ctl_cand[i].par!=par)continue;
        d=(int32)ctl_cand[i].ctx_sum_mv-(int32)ctl_ctx_sum_mv;
        if(d<=0 && -d<dlo){dlo=-d;lo=i;}
        if(d>=0 &&  d<dhi){dhi= d;hi=i;}
    }
    if(lo>=CTL_CAND_N||hi>=CTL_CAND_N||lo==hi)return 0u;
    x0=(int32)ctl_cand[lo].ctx_sum_mv;x1=(int32)ctl_cand[hi].ctx_sum_mv;
    dx=x1-x0;
    if(dx==0)return 0u;
    for(j=0;j<4;++j) {
        int32 y0=(int32)ctl_cand[lo].codes[j],y1=(int32)ctl_cand[hi].codes[j];
        out[j]=(int16)bounded((int)(y0+((y1-y0)*((int32)ctl_ctx_sum_mv-x0))/dx),-255,255);
    }
    return 1u;
}

/* Guarda el punto del par, quedandose con el MEJOR y no con el ultimo.  Si el
 * guardado puntua mejor, solo se le anota el acierto: esa es la diferencia
 * entre acumular evidencia y pisarla. */
static void ctl_punto_guardar(uint8 pga,uint8 pgaout)
{
    if(!ctl_punto_activa())return;
    uint8 idx,j;
    int16 lpo;
    CtlPunto *q;
    if(pga>8u||pgaout>8u)return;
    idx=CTL_PT_IDX(pga,pgaout);q=&ctl_pt[idx];
    lpo=(int16)(ctl_dc[4]/1000);
    if(ctl_pt_valido(q) &&
       ctl_punto_score(q->codes,q->lpo_mv)<=ctl_punto_score(ctl_codes,lpo)) {
        ctl_pt_set_aciertos(q,(uint8)(ctl_pt_aciertos(q)+1u));
        ctl_pt_set_fallos(q,0u);
        ctl_punto_sucio=idx;
        return;
    }
    for(j=0;j<4;++j)q->codes[j]=ctl_codes[j];
    q->lpo_mv=lpo;
    q->estado=(uint8)(1u|(q->estado&(uint8)(7u<<2)));
    ctl_pt_set_aciertos(q,(uint8)(ctl_pt_aciertos(q)+1u));
    ctl_pt_set_fallos(q,0u);
    /* Residuo: cuanto hubo que corregir desde el valor modelado.  Solo tiene
     * sentido si de verdad se partio de una semilla. */
    if(ctl_punto_origen==CTL_ORIGEN_EEPROM||ctl_punto_origen==CTL_ORIGEN_VECINO||
       ctl_punto_origen==CTL_ORIGEN_INTERPOLADO||ctl_punto_origen==CTL_ORIGEN_SIEMBRA) {
        q->res_slow=(int8)bounded((int)((int32)ctl_codes[0]-(int32)ctl_punto_semilla[0]),-127,127);
        q->res_fast=(int8)bounded((int)((int32)ctl_codes[3]-(int32)ctl_punto_semilla[3]),-127,127);
    }
    ctl_punto_sucio=idx;
}

/* Un punto guardado que NO reprodujo el punto de trabajo no se puede
 * restaurar eternamente, o cada vez hay que re-aprender.  Pero tampoco se
 * tira al primer fallo: los cuatro codigos no definen el estado -el acople de
 * 680 uF y su absorcion dielectrica hacen que el mismo par aterrice distinto
 * segun de donde se venga (medido 2026-09-19 a x50/x1)-, asi que un punto
 * puede fallar desde un contexto y servir desde otro.  Se DEGRADA, y recien
 * al tercer fallo se invalida. */
static void ctl_punto_olvidar(uint8 pga,uint8 pgaout)
{
    if(!ctl_punto_activa())return;
    uint8 idx,i;
    CtlPunto *q;
    if(pga>8u||pgaout>8u)return;
    idx=CTL_PT_IDX(pga,pgaout);q=&ctl_pt[idx];
    if(!ctl_pt_valido(q))return;
    ctl_pt_set_fallos(q,(uint8)(ctl_pt_fallos(q)+1u));
    if(ctl_pt_fallos(q)>=CTL_PT_FALLOS_MAX)q->estado=0u;
    ctl_punto_sucio=idx;
    /* El candidato de RAM que se acaba de usar tambien se cae: si el punto no
     * reprodujo desde ESTE contexto, es este contexto el que sobra. */
    for(i=0;i<CTL_CAND_N;++i)
        if(ctl_cand[i].par==idx &&
           magnitude((int32)ctl_cand[i].ctx_sum_mv-(int32)ctl_ctx_sum_mv)<=5)
            ctl_cand[i].par=0xFFu;
}

static uint8 ctl_punto_tiene(uint8 pga,uint8 pgaout)
{
    if(pga>8u||pgaout>8u)return 0u;
    return ctl_pt_valido(&ctl_pt[CTL_PT_IDX(pga,pgaout)]);
}

/* Aplica cuatro codigos como condicion inicial y deja el lazo listo para
 * corregir desde ahi.  NO congela nada: el forzado que abrio
 * control_on_gain_change sigue vigente, asi que el PI arranca enseguida. */
static void ctl_punto_aplicar(const int16 *codes,uint8 origen)
{
    uint8 j;
    for(j=0;j<4;++j){ctl_write(j,codes[j]);ctl_punto_semilla[j]=codes[j];}
    ctl_pi.fine=ctl_codes[3];ctl_pi.coarse=ctl_codes[2];
    ctl_pi.has_previous=0;ctl_pi.fraction=0;ctl_pi.slope_armed=0;
    ctl_punto_origen=origen;
    /* Evento, no reporte periodico: el host necesita el origen y la semilla al
     * PRINCIPIO de la visita.  El residuo de correccion que mide si el modelo
     * sirve es codigos_finales - esta semilla. */
    ctl_emit(0x140,(int32)origen,0);
    for(j=0;j<4;++j)ctl_emit((uint16)(0x148+j),(int32)codes[j],0);
}

static uint8 ctl_punto_restaurar(uint8 pga,uint8 pgaout)
{
    uint8 idx;
    if(pga>8u||pgaout>8u)return 0u;
    idx=CTL_PT_IDX(pga,pgaout);
    if(!ctl_pt_valido(&ctl_pt[idx]))return 0u;
    ctl_punto_aplicar(ctl_pt[idx].codes,CTL_ORIGEN_EEPROM);
    return 1u;
}

/* Cascada de seleccion.  Cada nivel cae al siguiente, y el nivel 0 deja el
 * comportamiento anterior intacto: es el brazo de control del experimento. */
static uint8 ctl_punto_elegir(uint8 pga,uint8 pgaout)
{
    int32 pol=ctl_config.value[CP_PUNTO_POLICY];
    uint8 par,k;int16 pred[4];
    if(pol<=(int32)CTL_POL_OFF)return 0u;
    if(pga>8u||pgaout>8u)return 0u;
    par=CTL_PT_IDX(pga,pgaout);
    if(pol>=(int32)CTL_POL_INTERPOLADO && ctl_cand_interp(par,pred)) {
        ctl_punto_aplicar(pred,CTL_ORIGEN_INTERPOLADO);return 1u;
    }
    if(pol>=(int32)CTL_POL_VECINO && ctl_cand_vecino(par,&k)) {
        ctl_punto_aplicar(ctl_cand[k].codes,CTL_ORIGEN_VECINO);return 1u;
    }
    return ctl_punto_restaurar(pga,pgaout);
}

/* Anotar el par ACTUAL y bajarlo a la EEPROM en el acto.  Se llama al terminar
 * un aprendizaje: ese punto costo minutos y es el que evita repetirlos.  Antes
 * la tabla colgaba solo del autosave, que exige ademas CP_SETTLED_MS (60 s)
 * congelado; medido el 2026-09-19 a x50/x1, el aprendizaje termino en banda
 * pero con IDAC3 todavia moviendose, asi que la tabla NUNCA se escribio y al
 * reiniciar el nodo volvio a aprender de cero. */
static void ctl_punto_fijar_actual(void)
{
    uint8 pga=(uint8)ctl_config.value[CP_PGA];
    uint8 pgaout=(uint8)ctl_config.value[CP_PGAOUT];
    ctl_punto_guardar(pga,pgaout);
    if(ctl_punto_sucio<CTL_PT_N)ctl_punto_store();
    /* Al pool SOLO entran puntos con el lazo declarandose en banda.  Una
     * muestra en transitorio contaminaria la recta del nivel 3: en esta
     * planta la pendiente instantanea da 18,8 mV/codigo donde en regimen son
     * 12,3, y una recta ajustada con eso corrige de menos siempre. */
    if(ctl_band && pga<=8u && pgaout<=8u)ctl_cand_anotar(CTL_PT_IDX(pga,pgaout));
}

static void ctl_punto_emitir(uint8 capture)
{
    uint8 pga=(uint8)ctl_config.value[CP_PGA];
    uint8 pgaout=(uint8)ctl_config.value[CP_PGAOUT];
    uint8 par,k;uint16 n=0;
    if(pga>8u||pgaout>8u)return;
    par=CTL_PT_IDX(pga,pgaout);
    for(k=0;k<CTL_PT_N;++k)if(ctl_pt_valido(&ctl_pt[k]))++n;
    /* El ORIGEN y la SEMILLA NO se reemiten aca.  Son propiedad de la visita
     * y solo salen como EVENTO (al cambiar de ganancia y al aplicar un
     * punto).  Reemitirlos cada 30 reportes era el vector por el que una
     * visita que no aplico ningun punto terminaba reportando el origen de la
     * anterior: el host limpia la clave al empezar, pero el snapshot se la
     * volvia a mandar.  Medido el 2026-09-24 en el brazo de CONTROL. */
    ctl_emit(0x141,(int32)ctl_pt_aciertos(&ctl_pt[par]),capture);
    ctl_emit(0x142,(int32)ctl_pt_fallos(&ctl_pt[par]),capture);
    ctl_emit(0x143,(int32)ctl_cand_n_par(par),capture);
    ctl_emit(0x144,ctl_config.value[CP_PUNTO_POLICY],capture);
    ctl_emit(0x145,(int32)ctl_ctx_sum_mv,capture);
    ctl_emit(0x146,(int32)n,capture);
    ctl_emit(0x14C,(int32)ctl_pt[par].res_slow,capture);
    ctl_emit(0x14D,(int32)ctl_pt[par].res_fast,capture);
}

/* Volcado de la tabla: TRES tramas por par valido, con claves FIJAS que se
 * repiten en secuencia (par, luego sus dos palabras).
 *
 * No se usa una clave por par -0x300+i, que seria lo natural- porque el ESP
 * DESCARTA toda clave >= 512: `if(key>=512){++_badLen;return;}` en
 * psoc_uart.cpp.  Con 0x300 y 0x400 el volcado salia del PSoC y moria en el
 * ESP sin dejar rastro, y la tabla parecia vacia aunque tuviera entradas.
 *
 * Como el ESP imprime CADA trama que recibe, el host las lee en orden y
 * arma los tripletes; que `_controlLive[key]` se pise no importa. */
#define CTL_PT_K_PAR 0x150u
#define CTL_PT_K_A   0x151u
#define CTL_PT_K_B   0x152u
static void ctl_punto_volcar_uno(uint8 i)
{
    const CtlPunto *q=&ctl_pt[i];
    uint32 a,b,cab;
    if(!ctl_pt_valido(q))return;
    /* Los residuos viajan en la palabra del par: metidos en la de abajo se
     * pasaban de 32 bits y res_fast perdia sus dos bits altos. */
    cab=(uint32)i
        |((uint32)(uint8)q->res_slow<<8)
        |((uint32)(uint8)q->res_fast<<16);
    a=((uint32)(q->codes[0]+512)&0x3FFu)
      |(((uint32)(q->codes[1]+512)&0x3FFu)<<10)
      |(((uint32)(q->codes[2]+512)&0x3FFu)<<20);
    b=((uint32)(q->codes[3]+512)&0x3FFu)
      |((uint32)q->estado<<10);
    ctl_emit(CTL_PT_K_PAR,(int32)cab,0);
    ctl_emit(CTL_PT_K_A,(int32)a,0);
    ctl_emit(CTL_PT_K_B,(int32)b,0);
}
static void ctl_punto_dump(uint8 par)
{
    uint8 i;
    if(par<CTL_PT_N){ctl_punto_volcar_uno(par);return;}
    for(i=0;i<CTL_PT_N;++i)ctl_punto_volcar_uno(i);
}

void control_on_gain_change(uint8 pga_code, uint8 pgaout_code, uint8 pga_changed)
{
    if(!ctl_initialized)return;
    /* Guardar donde estabamos ANTES de cambiar, si valia la pena.  Y bajarlo a
     * la EEPROM aca mismo: el autosave del perfil tiene un minimo de 10 min
     * entre escrituras, asi que colgada de ahi la tabla solo alcanzaba a
     * persistir el primer par de una tanda.  Un cambio de ganancia es un evento
     * raro y son cinco filas: la EEPROM aguanta 1e6 ciclos por fila. */
    if(ctl_pga_prev!=0xFFu && ctl_band) {
        ctl_punto_guardar(ctl_pga_prev,ctl_pgaout_prev);
        ctl_punto_store();
        ctl_cand_anotar(CTL_PT_IDX(ctl_pga_prev,ctl_pgaout_prev));
    }
    /* Contexto de ENTRADA al par nuevo, antes de tocar un solo codigo: SUMo
     * medido mas el par lento son el proxy de la carga del acople de 680 uF,
     * que es lo que decide donde aterrizan unos mismos codigos. */
    ctl_ctx_tomar();
    /* El origen y la semilla son GLOBALES PEGAJOSOS: si esta visita no aplica
     * ningun punto (politica 0, o aprendizaje), sin esto el firmware seguiria
     * reportando los de la visita ANTERIOR y el host calcularia un residuo
     * contra una semilla que nunca se uso.  Medido el 2026-09-24: el brazo de
     * CONTROL salia con el mismo origen y el mismo residuo que el brazo de
     * MEMORIA que corrio justo antes, o sea que la referencia del experimento
     * estaba copiando a lo que tenia que medir.
     * Se emite como evento para que el host lo vea al principio de la visita. */
    ctl_punto_origen=CTL_ORIGEN_NADA;
    ctl_emit(0x140,(int32)CTL_ORIGEN_NADA,0);
    ctl_emit(0x144,ctl_config.value[CP_PUNTO_POLICY],0);
    /* La config tiene que seguir a la placa: si no, un APPLY o un arranque en
     * frio volverian a la ganancia vieja y el perfil guardado mentiria sobre
     * con que ganancia fue aprendido. */
    if(ctl_config.value[CP_PGA]!=(int32)pga_code ||
       ctl_config.value[CP_PGAOUT]!=(int32)pgaout_code) {
        ctl_slow_gain_x1000=1000;ctl_slow_armado=0;
    }
    ctl_config.value[CP_PGA]=(int32)pga_code;
    ctl_config.value[CP_PGAOUT]=(int32)pgaout_code;
    ctl_staged=ctl_config;
    /* Una ganancia no suma un offset: lo MULTIPLICA. Lo que estaba centrado
     * queda contra el riel, y ahi el vernier solo no alcanza — por eso ademas
     * de forzar se borra el enfriamiento del grueso, para que IDAC2 pueda
     * moverse en el primer paso en vez de esperar CP_COARSE_MS. */
    control_pi_force(&ctl_pi,ctl_ms,(uint32)CTL_GAIN_FORCE_S*1000u);
    ctl_pi.has_coarse=0;
    ctl_pi.has_rescue=0;ctl_pi.rescue_step=0;ctl_pi.rescue_dir=0;
    /* Las lecturas viejas son de la ganancia anterior: no sirven. */
    ctl_med_n=0;ctl_med_i=0;
    if(!ctl_scan){ctl_scan=1;ctl_select(0);}
    /* Si ya estuvimos en este par, saltar a sus codigos: ni rescate a ciegas
     * ni aprendizaje. El forzado de arriba alcanza para el ajuste fino.
     * OJO: aca se RETORNABA sin actualizar ctl_pga_prev, asi que el par
     * anterior quedaba con el valor viejo y el proximo cambio de ganancia
     * guardaba los codigos de ESTE par bajo la entrada de OTRO, corrompiendola.
     * La rama de siembra de mas abajo ya lo actualizaba; esta se lo perdia. */
    if(ctl_punto_elegir(pga_code,pgaout_code)) {
        ctl_pga_prev=pga_code;ctl_pgaout_prev=pgaout_code;
        return;
    }
    /* Par nuevo: sembrar con el punto del par ANTERIOR en vez de buscar
     * desde cero.  Medido el 2026-09-19: a PGA x24 la busqueda a ciegas
     * nunca encuentra la ventana — un codigo del par lento mueve SUMo
     * 365 mV contra una ventana de 195 — pero partiendo del punto de un
     * par vecino el mismo x24/x4 aterriza y se queda (banda en 90 s).
     * O sea que el punto EXISTE; lo que no funciona es buscarlo a ciegas. */
    /* La siembra desde el vecino SOLO a PGA alto.  Medido el 2026-09-19:
     * a x4 y x8 la busqueda normal aterriza en 5-20 s, y sembrarla desde
     * un par lejano la empeoro a 350 s; recien de x16 para arriba la
     * busqueda a ciegas no encuentra la ventana y la siembra es la unica
     * forma de llegar. */
    /* La siembra desde el vecino TAMBIEN es reutilizar un punto guardado, asi
     * que tiene que respetar la politica.  Sin esta guarda el brazo de CONTROL
     * del experimento recibe un punto igual que el de MEMORIA en cuanto el PGA
     * llega a x16, y los dos brazos se parecen por construccion: el
     * experimento mediria "no hay diferencia" porque no hay control.
     * Medido el 2026-09-25 al entrar en x50/x48/x32. */
    if(ctl_config.value[CP_PUNTO_POLICY]>(int32)CTL_POL_OFF &&
       pga_code>=CTL_PGA_SIEMBRA && ctl_pga_prev!=0xFFu &&
       ctl_punto_restaurar(ctl_pga_prev,ctl_pgaout_prev)) {
        ctl_punto_origen=CTL_ORIGEN_SIEMBRA;
        ctl_pga_prev=pga_code;ctl_pgaout_prev=pgaout_code;
        return;
    }
    if(pga_changed) {
        /* Medido el 2026-09-18 en cuatro corridas: con el PGA en x8 o mas el
         * offset de entrada multiplicado deja a SUMo en +100 mV, FUERA de su
         * ventana de medicion (+-97,7). Ahi la guardia de SUMo se dispara y
         * hace return, asi que el rescate de LPo nunca corre: el lazo queda
         * trabado, y no es cuestion de sintonia. IDAC2 tiene 0,6 mV/codigo
         * sobre SUMo (165 codigos para 100 mV). El actuador que corresponde
         * al PGA, que esta aguas arriba del sumador, es el par lento
         * IDAC0/IDAC1: o sea volver a aprender. */
        (void)control_command(0xD6u,CONTROL_LEARN,0u);
    }
    ctl_pga_prev=pga_code;ctl_pgaout_prev=pgaout_code;
}
/* Ganancia de PGAout a la que estan medidas las pendientes de la config
 * (x24, geo-01, 2026-09-16), en milesimas. */
#define CTL_SLOPE_REF_PGAOUT_X1000 24000
static ControlConfig ctl_scaled;
/* IDAC2 inyecta en el SUMADOR, aguas arriba de PGAout, asi que su efecto
 * sobre LPo escala con esa ganancia; IDAC3 actua en el pasabajos, aguas
 * abajo, y no escala.  Sin esto el contramovimiento 'bumpless' del vernier
 * compensa 150 mV que el grueso nunca dio: medido el 2026-09-18 a x8/x8,
 * LPo saltaba de un riel al otro (-269 -> +113 mV) en quince segundos. */
static const ControlConfig *ctl_effective_config(void)
{
    int32 escalada;
    uint16 g=psoc_hw_pgaout_gain_x1000();
    if(g==0u || g==CTL_SLOPE_REF_PGAOUT_X1000)return &ctl_config;
    ctl_scaled=ctl_config;
    escalada=(int32)(((int64)ctl_config.value[CP_COARSE_SLOPE_UV]*(int32)g)
                     /CTL_SLOPE_REF_PGAOUT_X1000);
    /* control_config_valid() exige |pendiente| >= 100 uV: por debajo de eso
     * el grueso no seria un actuador sino ruido. */
    if(escalada>-100 && escalada<100)escalada=(ctl_config.value[CP_COARSE_SLOPE_UV]<0)?-100:100;
    ctl_scaled.value[CP_COARSE_SLOPE_UV]=escalada;
    return &ctl_scaled;
}
void control_note_manual_dac(uint8 stage, int16 code)
{
    if(!ctl_initialized || stage>3u)return;
    /* El DAC ya lo escribio quien llamo (0xAA).  Aca solo se sincroniza el
     * estado del lazo para que no lo pise en el proximo tick. */
    ctl_codes[stage]=code;
    if(stage==3u)ctl_pi.fine=code;
    else if(stage==2u)ctl_pi.coarse=code;
    /* El integrador venia acumulando contra el valor viejo: arrastrarlo
     * despues de un salto manual hace que la primera correccion salga con un
     * error que ya no existe. */
    ctl_pi.has_previous=0;ctl_pi.fraction=0;
}
uint8 control_force_pi(uint16 seconds)
{
    if(!ctl_initialized || ctl_state!=CONTROL_RUNNING)return 0u;
    control_pi_force(&ctl_pi,ctl_ms,(uint32)seconds*1000u);
    /* Que el proximo barrido no tarde: el lazo solo actua cuando tiene una
     * lectura fresca de LPo y de SUMo. */
    if(!ctl_scan){ctl_scan=1;ctl_select(0);}
    return 1u;
}
uint8 control_set_view_channel(uint8 channel)
{
    if(channel!=PSOC_VIEW_CHANNEL_NONE &&
       channel>=(uint8)PSOC_AMUX_SIGNAL_CHANNEL_COUNT)return 0u;
    ctl_view_channel=channel;
    /* Se aplica ya, pero OJO: con el lazo corriendo su barrido vuelve a mover
     * el AMux enseguida. Lo que manda de verdad es el ARM, que vuelve a llamar
     * a control_capture_channel() con el lazo ya congelado — o sea que la
     * vista se ve en la CAPTURA, no necesariamente en el instante. */
    control_select_capture_path();
    return 1u;
}
void control_select_capture_path(void)
{ if(ctl_initialized)psoc_amux_select_exclusive(control_capture_channel(),0); }
void control_prepare_capture(void)
{
    uint16 pos;uint8 j;
    if(!ctl_initialized)return;
    ctl_active=0;ADC_StopConvert();
    if(ctl_state==CONTROL_LEARNING) {
        uint8 i;
        /* Interrupted learning is never persisted; restore the last profile. */
        for(i=0;i<4;++i)ctl_write(i,ctl_learning_restore[i]);
    }
    control_emit_snapshot(1);
    memset(ctl_capture_metadata,0,sizeof ctl_capture_metadata);
    ctl_capture_metadata[0]='C';ctl_capture_metadata[1]='T';ctl_capture_metadata[2]=CONTROL_CONFIG_VERSION;
    ctl_capture_metadata[3]=ctl_profile;ctl_capture_metadata[4]=ctl_band;ctl_capture_metadata[5]=ctl_origin;
    ctl_put32(ctl_capture_metadata+8,ctl_revision);ctl_put32(ctl_capture_metadata+12,ctl_ms);
    control_config_encode(&ctl_config,ctl_capture_metadata+16);pos=16+CONTROL_CONFIG_BYTES;
    for(j=0;j<4;++j){ctl_put32(ctl_capture_metadata+pos,(uint32)(int32)ctl_codes[j]);pos+=4;}
    for(j=0;j<5;++j){ctl_put32(ctl_capture_metadata+pos,(uint32)ctl_dc[j]);pos+=4;
        ctl_put32(ctl_capture_metadata+pos,ctl_seen[j]?ctl_ms-ctl_stamp[j]:UINT32_MAX);pos+=4;
        ctl_put32(ctl_capture_metadata+pos,ctl_seen[j]&&ctl_valid[j]);pos+=4;}
    ctl_put32(ctl_capture_metadata+pos,control_crc((const uint8 *)(__DATE__ " " __TIME__),sizeof(__DATE__ " " __TIME__)-1));
    ctl_state=CONTROL_PAUSED;ctl_pi.has_previous=0;ctl_pi.fraction=0;
    psoc_amux_select_exclusive(control_capture_channel(),0);
    psoc_adc_cal_override(0);
}
void control_resume(void)
{
    if(!ctl_initialized)return;
    ctl_state=ctl_profile?CONTROL_RUNNING:CONTROL_NEEDS_LEARNING;
    ctl_pi.has_previous=0;ctl_pi.fraction=0;ctl_pi.holding=0;
    ctl_pi.fine=ctl_codes[3];ctl_pi.coarse=ctl_codes[2];
    ctl_route_start();
}
void control_init(void)
{
    uint8 a,b,i;uint32 ga=0,gb=0;uint16 pos;
    control_config_defaults(&ctl_config);
    a=ctl_load_slot(0);if(a)ga=ctl_get32(ctl_image+4);
    b=ctl_load_slot(1);if(b)gb=ctl_get32(ctl_image+4);
    ctl_nv_slot=(b && (!a || (int32)(gb-ga)>0))?1:0;
    if((a||b) && ctl_load_slot(ctl_nv_slot)) {
        uint8 legacy=ctl_image[2]==3u;
        /* Cada version guardo su propia cantidad de parametros; los que no
         * estan en la imagen quedan con el valor de fabrica. */
        /* Cada version guardo SU cantidad de parametros.  Sin la rama de la v5
         * una imagen v5 se decodifica con el CP_COUNT nuevo (49) sobre 47
         * guardados: lee ocho bytes de mas -que son el perfil y sus codigos-,
         * los mete en CP_PUNTO_POLICY/ARG, control_config_valid() falla y el
         * PERFIL APRENDIDO SE TIRA.  Visto el 2026-09-24 al grabar: el nodo
         * arranco en FALTA_APRENDER con un perfil perfectamente bueno en la
         * EEPROM. */
        uint16 count=legacy?CONTROL_CONFIG_COUNT_V3:
                     (ctl_image[2]==4u?CONTROL_CONFIG_COUNT_V4:
                      (ctl_image[2]==5u?CONTROL_CONFIG_COUNT_V5:CP_COUNT));
        control_config_decode_n(&ctl_staged,ctl_image+8,count);
        if(legacy) {
            /* v3 stored a fast-loop tuning in the wrong domain.  Keep what
             * belongs to the learned profile, take the fast loop from the
             * v4 defaults. */
            static const uint8 fast[]={CP_PERIOD_MS,CP_TAU_MS,CP_KP_NUM,CP_KP_DEN,
                CP_DEADBAND_UV,CP_FINE_STEP,CP_RESCUE_STEP,CP_RESCUE_MS,CP_SUM_BAND_UV,
                CP_COARSE_STEP,CP_FINE_SLOPE_UV,CP_COARSE_SLOPE_UV,CP_SCAN_MS,CP_REPORT_MS};
            ControlConfig d;control_config_defaults(&d);
            for(i=0;i<sizeof fast;++i)ctl_staged.value[fast[i]]=d.value[fast[i]];
        }
        if(control_config_valid(&ctl_staged,PSOC_AMUX_SIGNAL_CHANNEL_COUNT,CAL_AMUX_HAS_CAP_CHANNEL)) {
            ctl_config=ctl_staged;ctl_origin=1;ctl_generation=ctl_get32(ctl_image+4);
            pos=(uint16)(8u+count*4u);ctl_profile=ctl_image[pos++]==1;
            for(i=0;i<4;++i){ctl_saved_codes[i]=(int16)((uint16)ctl_image[pos]|((uint16)ctl_image[pos+1]<<8));pos+=2;
                if(ctl_saved_codes[i]<-255||ctl_saved_codes[i]>255)ctl_profile=0;}
            ctl_revision=ctl_get32(ctl_image+pos);
        }
    }
    ctl_staged=ctl_config;ctl_profile_config=ctl_config;
    ctl_punto_load();
    psoc_hw_set_pga((uint8)ctl_config.value[CP_PGA]);psoc_hw_set_pgaout((uint8)ctl_config.value[CP_PGAOUT]);
    ctl_state=CONTROL_NEEDS_LEARNING;
    for(i=0;i<4;++i)ctl_write(i,ctl_profile?ctl_saved_codes[i]:0);
    /* El perfil ES el punto del par con el que fue aprendido.  Sin sembrarlo,
     * un arranque en frio con la tabla vacia hacia que el primer comando de
     * ganancia —aunque pidiera la MISMA ganancia— no encontrara punto y
     * disparara un aprendizaje entero de cero (visto 2026-09-19 a x50/x1: 420 s
     * de barrido consumidos re-aprendiendo algo que ya estaba guardado).  Solo
     * se siembra si la tabla no traia ya una entrada para ese par, que seria
     * mas nueva. */
    ctl_punto_origen=ctl_profile?CTL_ORIGEN_PERFIL:CTL_ORIGEN_NADA;
    for(i=0;i<4;++i)ctl_punto_semilla[i]=ctl_codes[i];
    if(ctl_profile && !ctl_punto_tiene((uint8)ctl_config.value[CP_PGA],
                                       (uint8)ctl_config.value[CP_PGAOUT])) {
        ctl_punto_guardar((uint8)ctl_config.value[CP_PGA],
                          (uint8)ctl_config.value[CP_PGAOUT]);
        ctl_punto_store();
    }
    ctl_ctx_tomar();
    ctl_initialized=1;CySysTickStart();CySysTickSetCallback(0,control_tick);
    control_resume();
    /* Arranque en frio FORZADO: sin esto el lazo sale de la EEPROM, y si el
     * punto guardado ya no sirve (temperatura, otra ganancia, la placa estuvo
     * apagada) queda esperando a la histeresis y al rescate lento.  Forzando,
     * la cadencia pasa a CTL_FORCED_STEP_MS y con el paso expansivo aterriza
     * en tres o cuatro vueltas en vez de minutos. */
    control_pi_force(&ctl_pi,ctl_ms,(uint32)CTL_BOOT_FORCE_S*1000u);
}
/* Settled mode slows the diagnostic scan and the reports: both put ticks on
 * the analog chain (AMux switching, I2C bursts to the ESP). */
static uint8 ctl_settled(uint32 now)
{ return ctl_state==CONTROL_RUNNING && control_pi_settled(&ctl_pi,&ctl_config,now); }
static uint32 ctl_scan_ms(uint32 now)
{ return (uint32)ctl_config.value[ctl_settled(now)?CP_SCAN_SETTLED_MS:CP_SCAN_MS]; }
static uint8 ctl_all_healthy(uint32 now)
{
    uint8 i;
    for(i=0;i<5;++i)if(!ctl_seen[i]||!ctl_valid[i]||now-ctl_stamp[i]>ctl_scan_ms(now)*2u)return 0;
    return 1;
}
static void ctl_learning_fail(uint16 reason)
{
    uint8 i;
    for(i=0;i<4;++i) {
        ctl_write(i,ctl_learning_restore[i]);
        if(ctl_learning_had_profile)ctl_saved_codes[i]=ctl_learning_restore[i];
    }
    ctl_profile=ctl_learning_had_profile;
    ctl_state=ctl_profile?CONTROL_RUNNING:CONTROL_FAILED;
    ctl_pi.fine=ctl_codes[3];ctl_pi.coarse=ctl_codes[2];
    ctl_pi.has_previous=0;ctl_pi.fraction=0;
    ctl_emit(0x108,(int32)reason,0);
}
static uint8 ctl_choose_slow_codes(int *out0,int *out1)
{
    const int32 *v=ctl_config.value;
    int a,b,ba=ctl_codes[0],bb=ctl_codes[1];
    int32 k0=(int32)(-(int64)v[CP_SLOW0_SLOPE_UV]*270/68);
    int32 k1=(int32)(-(int64)v[CP_SLOW1_SLOPE_UV]*2700/748);
    int32 gain_x1000=(int32)psoc_hw_pgaout_gain_x1000();
    /* ctl_slow_gain_x1000 corrige el modelo con lo MEDIDO.  Las pendientes de
     * fabrica se midieron a PGA x4 y sirven ahi; a x50 el modelo predice
     * 2,4 mV/codigo y la realidad son ~12,6 (medido el 2026-09-19), asi que
     * cada paso del aprendizaje se pasaba de largo y cruzaba la ventana, una y
     * otra vez.  El factor se aprende del paso anterior: es la unica forma de
     * no depender de una constante que cambia con la ganancia y con la placa. */
    int32 s0=(int32)((int64)k0*gain_x1000/1000*ctl_slow_gain_x1000/1000);
    int32 s1=(int32)((int64)k1*gain_x1000/1000*ctl_slow_gain_x1000/1000);
    int64 best=INT64_MAX,best_cost=INT64_MAX;
    for(a=-255;a<=255;++a)for(b=-255;b<=255;++b) {
        int32 da=a-ctl_codes[0],db=b-ctl_codes[1];
        int64 predicted=(int64)ctl_dc[3]+(int64)da*s0+(int64)db*s1;
        int64 residual=magnitude(predicted-ctl_learn_target);
        int64 cost=(int64)a*a+(int64)b*b;
        /* Solve the one SUMo error with both slow degrees of freedom, choosing
         * the lowest absolute IDAC effort among equivalent integer pairs. */
        if((residual<=v[CP_STABLE_UV] && (best>v[CP_STABLE_UV]||cost<best_cost)) ||
           (best>v[CP_STABLE_UV] && residual<best)) {
            best=residual;best_cost=cost;ba=a;bb=b;
        }
    }
    if(best==INT64_MAX)return 0;
    /* Lo que este paso PROMETE mover SUMo, para poder contrastarlo despues. */
    ctl_slow_pred=(int32)((int64)(ba-ctl_codes[0])*s0+(int64)(bb-ctl_codes[1])*s1);
    ctl_slow_sum0=ctl_dc[3];
    ctl_slow_armado=1u;
    *out0=ba;*out1=bb;return 1;
}
/* Contrasta lo prometido con lo que de verdad paso y corrige el factor.  Se
 * llama cuando SUMo ya se aquieto y sigue siendo medible: con un riel de por
 * medio el numero no significa nada. */
static void ctl_slow_gain_observar(void)
{
    int32 obs,factor;
    if(!ctl_slow_armado)return;
    ctl_slow_armado=0;
    if(!ctl_valid[3])return;
    if(magnitude(ctl_slow_pred)<=ctl_config.value[CP_STABLE_UV])return;
    obs=ctl_dc[3]-ctl_slow_sum0;
    /* Signo distinto: el modelo no se equivoco de tamanio sino de sentido, o
     * la medicion vino de un riel.  No se aprende de eso. */
    if((obs>0)!=(ctl_slow_pred>0))return;
    if(magnitude(obs)<=ctl_config.value[CP_STABLE_UV])return;
    factor=(int32)((int64)magnitude(obs)*1000/magnitude(ctl_slow_pred));
    ctl_slow_gain_x1000=(int32)((int64)ctl_slow_gain_x1000*factor/1000);
    if(ctl_slow_gain_x1000<100)ctl_slow_gain_x1000=100;        /* 0,1x */
    if(ctl_slow_gain_x1000>20000)ctl_slow_gain_x1000=20000;    /* 20x  */
    ctl_emit(0x10B,ctl_slow_gain_x1000,0);
}
static uint8 ctl_rescue_sum_with_slow_pair(uint32 now)
{
    const int32 *v=ctl_config.value;
    int32 k0=magnitude((int32)(-(int64)v[CP_SLOW0_SLOPE_UV]*270/68));
    int32 k1=magnitude((int32)(-(int64)v[CP_SLOW1_SLOPE_UV]*2700/748));
    int sign=ctl_dc[3]>=0?1:-1;
    int total,d0,d1,n0,n1;
    if(ctl_learning_slow_step<1) {
        ctl_learning_slow_step=(int16)(v[CP_RESCUE_STEP]/3);
        if(ctl_learning_slow_step<2)ctl_learning_slow_step=2;
    }
    if(sign>0) {
        ctl_learning_pos0=ctl_codes[0];ctl_learning_pos1=ctl_codes[1];
        ctl_learning_have_pos=1;
    } else {
        ctl_learning_neg0=ctl_codes[0];ctl_learning_neg1=ctl_codes[1];
        ctl_learning_have_neg=1;
    }
    if(ctl_learning_have_pos && ctl_learning_have_neg) {
        /* We know one code pair on each physical rail.  Bisect the codes, not
         * the clipped ADC numbers. */
        n0=(ctl_learning_pos0+ctl_learning_neg0)/2;
        n1=(ctl_learning_pos1+ctl_learning_neg1)/2;
    } else {
        total=ctl_learning_slow_step;
        d0=(int)((int64)total*k0/(k0+k1));
        if(d0<1)d0=1;
        d1=total-d0;if(d1<1)d1=1;
        /* Both modeled slopes are negative.  A positive rail therefore needs
         * positive codes; a negative rail needs negative codes. */
        n0=bounded(ctl_codes[0]+sign*d0,-255,255);
        n1=bounded(ctl_codes[1]+sign*d1,-255,255);
        /* Busqueda expansiva: mientras no se vio el otro riel, el paso se
         * DUPLICA.  Con paso fijo de un codigo esto no converge: medido el
         * 2026-09-18 tras pasar a PGA x8, SUMo quedo en +105,7 mV y un codigo
         * del par lento no lo movio ni el ruido (<0,6 mV), o sea centenares de
         * codigos a un paso cada 87 s: horas.  Duplicando llega en una decena
         * de pasos, y cuando el signo se da vuelta entra la biseccion de
         * arriba, que ya estaba escrita y no depende de ninguna pendiente. */
        if(ctl_learning_slow_step<64)
            ctl_learning_slow_step=(int16)(ctl_learning_slow_step*2);
    }
    if(n0==ctl_codes[0] && n1==ctl_codes[1]) {
        /* La biseccion no se movio porque los dos rieles quedaron
         * anotados con los MISMOS codigos: el que cambio de signo fue la
         * cola lenta, no un paso nuevo.  Antes esto abortaba el
         * aprendizaje y dejaba el nodo muerto — SUMo fuera de ventana,
         * la guardia bloqueando el rescate y nada moviendose durante 14
         * minutos (medido 2026-09-19 en x24/x4).  Se parte el paso y se
         * sigue caminando en el sentido que corresponde. */
        if(ctl_learning_slow_step>1) {
            ctl_learning_slow_step=(int16)(ctl_learning_slow_step/2);
            n0=bounded(ctl_codes[0]+sign*ctl_learning_slow_step,-255,255);
            n1=bounded(ctl_codes[1]+sign*ctl_learning_slow_step,-255,255);
        } else {
            /* VERNIER DIFERENCIAL: con el paso ya en un codigo, mover los
             * dos en el MISMO sentido suma sus pesos y es demasiado
             * grueso — medido el 2026-09-19 a PGA x24, un solo codigo de
             * IDAC0 mueve SUMo 365 mV contra una ventana de 195 mV, asi
             * que no hay combinacion entera que caiga adentro.  Moverlos
             * en sentidos OPUESTOS da la diferencia de los pesos, que es
             * varias veces mas fina. */
            n0=bounded(ctl_codes[0]+sign,-255,255);
            n1=bounded(ctl_codes[1]-sign,-255,255);
        }
        if(n0==ctl_codes[0] && n1==ctl_codes[1])return 0;
    }
    ctl_write(0,(int16)n0);ctl_write(1,(int16)n1);
    ++ctl_learning_refinements;ctl_phase_start=now;
    ctl_learning_prev_ms=now;ctl_learning_prev_sum=ctl_dc[3];
    return 1;
}
static void ctl_learning(uint32 now)
{
    const int32 *v=ctl_config.value;
    uint8 i;
    if(now-ctl_learning_start>(uint32)v[CP_LEARN_TIMEOUT_MS]) {
        /* Al vencer el plazo, quedarse con lo alcanzado si SUMo quedo medible.
         * Abortar RESTAURA los codigos viejos, que suelen ser peores que los
         * que se consiguieron (visto el 2026-09-18: volvia de SUMo -49 mV a
         * +105).  Desde que la fase 2 exige ademas LPo en banda, el plazo se
         * puede vencer sobre un punto perfectamente usable: fallar ahi seria
         * cambiar un nodo lento por uno roto. */
        if(ctl_seen[3] && control_measurement_valid(&ctl_config,ctl_dc[3])) {
            for(i=0;i<4;++i)ctl_saved_codes[i]=ctl_codes[i];
            ctl_profile=1;++ctl_revision;ctl_profile_config=ctl_config;
            if(ctl_store()) {
                ctl_state=CONTROL_RUNNING;
                ctl_pi.fine=ctl_codes[3];ctl_pi.coarse=ctl_codes[2];
                ctl_pi.has_previous=0;ctl_pi.fraction=0;
                ctl_punto_fijar_actual();
                ctl_emit(0x108,(int32)0x10,0);
                return;
            }
        }
        ctl_learning_fail(1);return;
    }
    /* ATAJO: si la cadena YA esta donde tiene que estar, el aprendizaje
     * termino.  No importa en que fase este ni como llego.
     *
     * Medido el 2026-09-19 a x50/x1, dos veces en la misma tarde: el
     * aprendizaje paso por I0/I1 = 12/0 con SUMo en -18,2 mV y LPo en +42,9
     * —en banda, con `banda=1` en la telemetria— y siguio buscando hasta
     * volver al riel.  La maquina de fases se lo perdia porque el punto bueno
     * caia en mitad de una espera de fase, no en el instante en que la fase
     * miraba.  El objetivo es el objetivo: si se cumple y se sostiene
     * CP_STABLE_MS, se guarda y se sale. */
    /* El tiempo de sostenimiento NO se puede acortar.  Probado el 2026-09-19
     * con CP_ENTER_MS (1 s): a x50/x1 la cadena cruza la ventana entera en
     * ~20 s, asi que un segundo adentro es un TRANSITO, no una llegada, y el
     * atajo aceptaba con LPo todavia yendo hacia el riel opuesto — el nodo
     * pasaba a RUNNING contra el riel y el lazo rapido quedaba saturado.
     * CP_STABLE_MS (10 s) distingue las dos cosas. */
    /* El atajo exige lo MISMO que la fase 2, vernier incluido.  Si no, es un
     * bypass: medido el 2026-09-25 a x4/x16, el aprendizaje salia por aca con
     * LPo en banda e IDAC3 en 233 de 255, y la verificacion de la fase 2 no
     * llegaba a correr nunca.  Un atajo que acepta lo que el camino largo
     * rechaza no es un atajo, es un agujero. */
    if(ctl_seen[3] && ctl_valid[3] && ctl_seen[4] && ctl_valid[4] &&
       now-ctl_stamp[3]<=ctl_scan_ms(now)*3u &&
       now-ctl_stamp[4]<=ctl_scan_ms(now)*3u &&
       magnitude(ctl_dc[4])<=v[CP_SETTLED_BAND_UV] &&
       magnitude((int32)ctl_codes[3])<=v[CP_FINE_LIMIT]/2) {
        if(ctl_bueno_desde==0u)ctl_bueno_desde=now;
        else if(now-ctl_bueno_desde>=(uint32)v[CP_STABLE_MS]) {
            for(i=0;i<4;++i)ctl_saved_codes[i]=ctl_codes[i];
            ctl_profile=1;++ctl_revision;ctl_profile_config=ctl_config;
            if(ctl_store()) {
                ctl_state=CONTROL_RUNNING;
                ctl_pi.fine=ctl_codes[3];ctl_pi.coarse=ctl_codes[2];
                ctl_pi.has_previous=0;ctl_pi.fraction=0;
                ctl_punto_fijar_actual();
                ctl_emit(0x108,(int32)0x11,0);
                return;
            }
        }
    } else ctl_bueno_desde=0u;
    /* Bracket del objetivo de SUMo.  Se anota SIEMPRE que SUMo sea medible y
     * LPo este fuera de su banda, no solo en el instante en que la fase 1 mira:
     * medido el 2026-09-19, mirando solo ahi se anotaba UN lado nada mas —el
     * otro riel pasaba entre dos visitas— y la biseccion no llegaba a
     * activarse nunca.  Se guarda el valor que ESTRECHA el intervalo, no el
     * ultimo que tocó, que es lo que la hace converger. */
    /* El bracket se anota SOLO con LPo fuera de banda, y eso es deliberado.
     * Probe agregar aca el caso "vernier agotado con LPo adentro" y fue un
     * error que trabo el aprendizaje: con LPo en banda SUMo casi no se mueve
     * -medido el 2026-09-26: 13,4 mV durante 300 s-, asi que los dos lados del
     * bracket se llenaban con casi el MISMO SUMo.  Con los dos lados marcados
     * la expansion se bloquea (cree que ya tiene el intervalo) y la biseccion
     * no puede actuar porque exige que los lados difieran mas de CP_STABLE_UV.
     * El lazo se quedaba aprendiendo hasta el timeout.
     *
     * No hace falta: el disparador de la EXPANSION ya reconoce el vernier
     * agotado, y mientras LPo siga adentro el bracket no se llena, asi que la
     * expansion corre, corre el objetivo, y cuando LPo cruza al otro lado el
     * bracket se completa solo con dos SUMo realmente distintos. */
    if(ctl_seen[3] && ctl_valid[3] && ctl_seen[4] &&
       (!ctl_valid[4] || magnitude(ctl_dc[4])>v[CP_DEADBAND_UV])) {
        uint8 arriba=(ctl_dc[4]>0);
        int32 *lado =arriba?&ctl_lp_sum_pos :&ctl_lp_sum_neg;
        uint8 *tengo=arriba?&ctl_lp_have_pos:&ctl_lp_have_neg;
        int32 otro  =arriba? ctl_lp_sum_neg : ctl_lp_sum_pos;
        uint8 hay   =arriba? ctl_lp_have_neg: ctl_lp_have_pos;
        if(!*tengo || !hay ||
           magnitude(ctl_dc[3]-otro)<magnitude(*lado-otro)) {
            *lado=ctl_dc[3];*tengo=1u;
        }
    }
    /* Antes de cada paso hay que esperar a que la cadena se AQUIETE, no a que
     * pase un tiempo fijo.  Con 2*CP_SLOW_TAU_MS (87 s) el lazo medía todavía
     * en pleno transitorio: medido el 2026-09-18, tras un paso SUMo seguía
     * moviéndose 90 s después (105,6 -> 98,0 -> 84,6 mV con los códigos
     * quietos), así que el paso siguiente salía calculado sobre una foto falsa
     * y el aprendizaje se pasaba de largo hasta el riel opuesto.  Ahora se
     * espera un tau como piso, después a que dos lecturas consecutivas de SUMo
     * difieran menos de CP_STABLE_UV, y hay tope de 6 tau para no colgarse si
     * la cadena nunca se queda quieta. */
    if(now-ctl_phase_start<(uint32)v[CP_SLOW_TAU_MS])return;
    if(now-ctl_phase_start<(uint32)v[CP_SLOW_TAU_MS]*6u) {
        /* OJO: esto comparaba contra la lectura ANTERIOR, y ctl_learning()
         * corre en CADA vuelta del lazo principal.  Entre dos vueltas SUMo no
         * cambia, asi que |delta| daba 0 y el criterio degeneraba en "esperar
         * un tau" — exactamente la espera corta que se queria evitar.  Medido
         * el 2026-09-19 a x50/x1: con los cuatro codigos quietos SUMo seguia
         * moviendose 46 mV en 32 s (~1,4 mV/s), o sea que el paso siguiente
         * salia calculado sobre una foto en pleno transitorio y el
         * aprendizaje no aterrizaba nunca — 420 s sin salir de LEARNING.
         * El criterio correcto es una TASA: menos de CP_STABLE_UV en
         * CTL_LEARN_QUIETO_MS. */
        if(now-ctl_learning_prev_ms<CTL_LEARN_QUIETO_MS)return;
        {
            int32 delta=ctl_dc[3]-ctl_learning_prev_sum;
            ctl_learning_prev_sum=ctl_dc[3];
            ctl_learning_prev_ms=now;
            if(delta>v[CP_STABLE_UV] || delta<-v[CP_STABLE_UV])return;
        }
    }
    if(ctl_phase==0 && ctl_seen[0] && ctl_seen[1] && ctl_seen[2] && ctl_seen[3]) {
        int ba,bb;
        /* A rail value carries a trustworthy sign but no trustworthy
         * magnitude.  Walk both slow controls in their maximum-effect
         * direction until SUMo becomes observable; never fit the clipped
         * number as though it were a voltage. */
        if(!control_measurement_valid(&ctl_config,ctl_dc[3])) {
            /* El presupuesto de refinamientos solo cuenta la BISECCION: la
             * expansion ya esta acotada por el rango de +-255 (nueve
             * duplicaciones) y por el timeout.  Gastarlo expandiendo era lo
             * que abortaba el aprendizaje antes de llegar, y al abortar
             * restauraba los codigos viejos, PEORES que los alcanzados
             * (visto 2026-09-18: volvia de SUMo -49 mV a +105). */
            if((ctl_learning_have_pos && ctl_learning_have_neg &&
                ctl_learning_refinements>=16u) ||
               !ctl_rescue_sum_with_slow_pair(now))
                ctl_learning_fail(2);
            return;
        }
        if(!ctl_choose_slow_codes(&ba,&bb)){ctl_learning_fail(2);return;}
        ctl_write(0,(int16)ba);ctl_write(1,(int16)bb);
        ctl_phase=1;ctl_phase_start=now;ctl_stable_start=now;
        ctl_learning_prev_ms=now;ctl_learning_prev_sum=ctl_dc[3];
    } else if(ctl_phase==1) {
        /* IDAC0/1 feed the slow BP path.  Do not start the fast controller
         * from their pre-step samples: wait two measured time constants and
         * refine the integer pair if SUMo is still outside its truthful
         * window. */
        if(!ctl_seen[3])return;
        ctl_slow_gain_observar();
        if(!control_measurement_valid(&ctl_config,ctl_dc[3])) {
            ctl_phase=0;
            if((ctl_learning_have_pos && ctl_learning_have_neg &&
                ctl_learning_refinements>=16u) ||
               !ctl_rescue_sum_with_slow_pair(now))
                ctl_learning_fail(2);
            return;
        }
        /* SUMo esta donde se le pidio, pero eso NO garantiza que LPo quede en
         * su ventana: el pasabajos tiene su propio cero y su propia ganancia
         * desde SUMo.  Medido el 2026-09-19 a x50/x1, con SUMo valido en dos
         * puntos limpios: SUMo -25,7 mV -> LPo +43,2 y SUMo +19,1 -> LPo
         * -252,6.  O sea ~-6,6 de ganancia y **LPo se anula con SUMo cerca de
         * -19 mV, no en cero**.  Con CP_OPA_TARGET_UV = 0 el aprendizaje
         * aterriza a LPo en ~+127 mV, fuera de la ventana de +-97,7, por
         * construccion.  A PGAout x24 el lazo rapido lo tapa con IDAC2; a x1
         * IDAC2 casi no tiene autoridad y el nodo se queda afuera para
         * siempre.
         *
         * En vez de meter la ganancia del pasabajos como constante (cambia con
         * el punto y con la placa), se BISECA el objetivo: se anota con que
         * SUMo quedo LPo de cada lado y se apunta al medio.  Solo mira SIGNOS,
         * como el resto del lazo.  Si el objetivo de fabrica ya deja a LPo
         * adentro esto no se activa nunca y los pares que ya andaban no
         * cambian. */
        /* Un solo lado visto: EXPANDIR el objetivo por SIGNO hasta cruzar.
         *
         * La biseccion de abajo necesita LPo de los dos lados, y con LPo
         * clavado en un riel nunca cruza: el bracket no se arma nunca y el
         * objetivo se queda en el de fabrica.  Medido el 2026-09-25 a x4/x16
         * (ganancia 64): SUMo centrado en +34 mV y LPo pegado en -265 mV
         * mientras IDAC3 recorria 69 codigos sin mover LPo un milivoltio.
         *
         * El cero de LPo NO esta en SUMo=0 y ADEMAS se mueve con la ganancia:
         * a x50/x1 se anula en SUMo ~-19 mV y a x4/x16 en ~+7 mV.  Un objetivo
         * fijo aterriza afuera por construccion.  Lo unico confiable con un
         * tap saturado es su SIGNO, asi que se camina el objetivo en el
         * sentido que reduce |LPo| -la etapa invierte: LPo baja cuando SUMo
         * sube- duplicando el paso hasta que LPo cambia de signo.  Ahi se
         * arma el bracket y manda la biseccion. */
        /* El disparador NO puede ser solo "LPo afuera".  Medido el 2026-09-25
         * a x4/x16: el par lento se planto en SUMo +17 mV cuando el cero de
         * LPo estaba en +7, y esos 10 mV por la ganancia de -6,2 de la etapa
         * son ~62 mV que IDAC3 absorbio GASTANDO TODO SU RANGO, de 39 a 255.
         * LPo entro en banda raspando, asi que por el criterio viejo estaba
         * todo bien -y el punto quedaba en banda y sin autoridad, que es la
         * clase de punto que se cae al primer cambio de temperatura-.
         * La senial de que el par lento dejo residuo es IDAC3 AGOTADO. */
        if(ctl_target_libre && !(ctl_lp_have_pos && ctl_lp_have_neg) &&
           ctl_seen[4] && ctl_seen[3] && ctl_valid[3] &&
           (!ctl_valid[4] || magnitude(ctl_dc[4])>v[CP_DEADBAND_UV] ||
            magnitude((int32)ctl_pi.fine)>v[CP_FINE_LIMIT]/2)) {
            /* El umbral del vernier agotado es FINE_LIMIT/2 y no CP_FINE_MID
             * (240) por lo mismo que en la fase 2: el lazo estaciona en 213 a
             * 233 y nunca llega a 240, asi que con FINE_MID este disparador
             * no se cumplia jamas —verificado: la biseccion no se activo ni
             * una vez en una corrida entera de aprendizaje—.  Y tiene que ser
             * el MISMO umbral que usa la fase 2 para rechazar el perfil: si la
             * fase 2 libera el objetivo por vernier agotado y la fase 1 no lo
             * reconoce como motivo, el aprendizaje rebota entre las dos sin
             * mover nada. */
            int32 paso=ctl_target_paso?ctl_target_paso:(v[CP_STABLE_UV]*2);
            /* Si el disparador fue IDAC3 agotado, el sentido lo marca EL
             * ACTUADOR y no LPo: IDAC3 contra su tope positivo esta
             * empujando LPo hacia arriba, o sea que sin el LPo seria mas
             * negativo, o sea que hay que bajar SUMo.  Cuando LPo todavia
             * esta afuera manda su propio signo, que es mas directo. */
            int32 dir;
            if(ctl_valid[4] && magnitude(ctl_dc[4])<=v[CP_DEADBAND_UV])
                dir=(ctl_pi.fine>0)?-1:1;
            else
                dir=(ctl_dc[4]<0)?-1:1;
            int32 nuevo=ctl_learn_target+dir*paso;
            int32 tope=v[CP_VALID_HIGH_UV]-v[CP_MARGIN_UV];
            int32 piso=v[CP_VALID_LOW_UV]+v[CP_MARGIN_UV];
            if(nuevo>tope)nuevo=tope;
            if(nuevo<piso)nuevo=piso;
            if(paso<v[CP_STABLE_UV]*16)ctl_target_paso=paso*2;
            if(nuevo!=ctl_learn_target) {
                ctl_learn_target=nuevo;
                ctl_emit(0x10A,nuevo,0);
                ctl_phase=0;ctl_phase_start=now;
                ctl_learning_prev_ms=now;ctl_learning_prev_sum=ctl_dc[3];
                return;
            }
        }
        /* Misma correccion que arriba: el vernier agotado tambien es motivo
         * para cerrar la biseccion sobre el medio.  Si no, con LPo en banda y
         * IDAC3 en 233 esta rama no se ejecuta, el objetivo queda donde la
         * expansion lo dejo y el aprendizaje acepta el punto sin margen. */
        if(ctl_target_libre && ctl_lp_have_pos && ctl_lp_have_neg &&
           magnitude(ctl_lp_sum_pos-ctl_lp_sum_neg)>v[CP_STABLE_UV] &&
           (!ctl_valid[4] || magnitude(ctl_dc[4])>v[CP_DEADBAND_UV] ||
            magnitude((int32)ctl_pi.fine)>v[CP_FINE_LIMIT]/2)) {
            int32 medio=(int32)(((int64)ctl_lp_sum_pos+(int64)ctl_lp_sum_neg)/2);
            if(medio!=ctl_learn_target) {
                ctl_learn_target=medio;
                ctl_emit(0x10A,medio,0);
                ctl_phase=0;ctl_phase_start=now;
                ctl_learning_prev_ms=now;ctl_learning_prev_sum=ctl_dc[3];
                return;
            }
        }
        ctl_phase=2;ctl_phase_start=now;ctl_stable_start=now;
        ctl_learning_prev_ms=now;ctl_learning_prev_sum=ctl_dc[3];
        ctl_lp_mejor=INT32_MAX;
        for(i=0;i<5;++i)ctl_anchor[i]=ctl_dc[i];
    } else if(ctl_phase==2) {
        /* La condicion de cierre pedia que SEo, BPo y OPA_SUMo se quedaran
         * QUIETOS dentro de CP_STABLE_UV (10 mV) durante CP_STABLE_MS.  En
         * esta planta eso es casi imposible: medido el 2026-09-19, con los
         * cuatro codigos sin tocar BPo se mueve mas de 10 mV en 20 s por la
         * cola de ~44 s del acople de 680 uF, y a PGA x50 va multiplicada.
         * El resultado era que el aprendizaje se quedaba sentado ENCIMA de un
         * punto bueno sin aceptarlo: a x50/x1, I0/I1 = 12/0 dejaba SUMo en
         * -13,8 mV y LPo en +16,5 mV —en banda— y la busqueda siguio de largo
         * hasta volver al riel.
         *
         * Lo que hay que exigir es lo que el perfil tiene que LOGRAR, no la
         * quietud de unos taps intermedios: SUMo medible y LPo dentro de la
         * banda del lazo rapido, sostenido CP_STABLE_MS. */
        /* La banda de aceptacion es la del MODO ESTABLE (60 mV), no la banda
         * muerta (45).  El perfil no tiene que dejar LPo centrado: tiene que
         * dejarlo donde el vernier pueda terminar el trabajo, que es para lo
         * que esta.  Medido el 2026-09-19 a x50/x1: el aprendizaje tenia SUMo
         * en -20,0 mV y LPo en +51,0 —seis milivoltios fuera de la banda
         * muerta, y holgadamente dentro de la ventana de +-97,7— y lo
         * descarto para seguir buscando. */
        /* ...pero "donde el vernier pueda terminar" hay que MEDIRLO, no
         * suponerlo.  Barrido de IDAC3 a x4/x16 el 2026-09-25, con el PI
         * pasivo (banda muerta ensanchada a 95 mV) y 160 codigos sin saturar:
         *
         *     0,129 mV por codigo  ->  66 mV en TODO su rango de 510
         *
         * O sea que el vernier entero cubre un tercio de la ventana valida
         * (+-97 mV), y la banda de aceptacion de 60 mV le pide casi el doble
         * de lo que tiene.  Por eso el aprendizaje venia aceptando puntos con
         * IDAC3 en 213, 231, 233, 248: LPo en banda y el actuador agotado, que
         * es justo la clase de punto que la primera deriva termica se lleva
         * puesto.  Y peor: con el vernier ya en el tope, los dos remedios que
         * existen -el descargue y la biseccion del objetivo- estan gatillados
         * en CP_FINE_MID = 240 y el lazo estaciona en 233, siete codigos
         * cortos de disparar su propia cura.
         *
         * Entonces el perfil tiene que dejar DOS cosas, no una: LPo en banda
         * Y el vernier con margen.  Si LPo entro pero IDAC3 esta pasado de la
         * mitad de su rango, esto NO es un perfil aceptable: se trata igual
         * que "todavia no llego", o sea se reinicia el reloj de estabilidad y,
         * pasadas las mismas 2 a 4 tau, se libera el objetivo de SUMo para que
         * la biseccion lo corra.  Hay a donde ir: un codigo de IDAC1 mueve LPo
         * 17,79 mV (medido en el mismo barrido), asi que contra el par lento
         * el vernier tiene 3,7 veces el rango que necesita.  Contra IDAC2, con
         * ~150 mV por codigo, no llegaba ni a la mitad: los actuadores no eran
         * insuficientes, estaban mal emparejados.
         *
         * Se usa FINE_LIMIT/2 y no CP_FINE_MID a proposito: FINE_MID es el
         * umbral de los remedios y subirlo o bajarlo cambia otras dos ramas;
         * esto es una condicion de ACEPTACION y no necesita parametro nuevo
         * -agregar uno sube la version de NV y tira el perfil aprendido-. */
        if(!ctl_all_healthy(now) || magnitude(ctl_dc[4])>v[CP_SETTLED_BAND_UV] ||
           magnitude((int32)ctl_codes[3])>v[CP_FINE_LIMIT]/2) {
            int32 dist=ctl_seen[4]?magnitude(ctl_dc[4]):INT32_MAX;
            /* Mientras LPo MEJORE no se abandona el punto: el reloj de
             * "este punto no sirve" se reinicia.  Si no, el lazo rapido esta
             * convergiendo y el aprendizaje le saca los codigos de abajo. */
            if(dist+v[CP_STABLE_UV]<ctl_lp_mejor) {
                ctl_lp_mejor=dist;
                ctl_phase_start=now;
            }
            ctl_stable_start=now;
            /* Al lazo rapido se le da su oportunidad PRIMERO: recien si
             * despues de varias tau LPo sigue fuera de banda se declara que el
             * objetivo de SUMo es el que esta mal y se lo deja bisecar.  Sin
             * esta puerta la biseccion del objetivo se activaria tambien en
             * los pares donde el objetivo de fabrica ya funciona, que es
             * cambiar algo que anda.  Un riel cuenta antes (2 tau) que un LPo
             * valido pero corrido (4 tau), porque del riel no se vuelve solo. */
            if(ctl_seen[4] &&
               now-ctl_phase_start>(uint32)v[CP_SLOW_TAU_MS]*(ctl_valid[4]?4u:2u)) {
                ctl_target_libre=1u;
                ctl_phase=1;ctl_phase_start=now;
                ctl_learning_prev_ms=now;ctl_learning_prev_sum=ctl_dc[3];
            }
            return;
        }
        if(now-ctl_stable_start >= (uint32)v[CP_STABLE_MS]) {
            for(i=0;i<4;++i)ctl_saved_codes[i]=ctl_codes[i];
            ctl_profile=1;++ctl_revision;ctl_profile_config=ctl_config;
            if(!ctl_store()){ctl_learning_fail(3);return;}
            ctl_state=CONTROL_RUNNING;ctl_pi.fine=ctl_codes[3];ctl_pi.coarse=ctl_codes[2];
            ctl_punto_fijar_actual();
        }
    }
}
/* Guardado automatico del punto de trabajo.  Sin esto, un arranque en frio
 * parte de los codigos del ultimo APRENDIZAJE (o de cero) y el integrador tiene
 * que recorrer toda la cadena otra vez; con LPo fuera de ventana eso es el
 * rescate a ciegas.  Criterio, todo a la vez:
 *   - el lazo corre, con perfil valido y SE DECLARA EN BANDA,
 *   - esta congelado en modo estable (control_pi_settled): no es un paso al
 *     vuelo sino un punto que ya se sostuvo,
 *   - los codigos cambiaron respecto de lo guardado,
 *   - y paso CTL_AUTOSAVE_MIN_MS desde el ultimo guardado.
 * Corre en el lazo principal (control_service), nunca en ISR ni durante una
 * captura: el control esta pausado ahi.  Si la escritura falla se restaura lo
 * que habia, para que EEPROM y RAM no queden diciendo cosas distintas. */
static void ctl_autosave(uint32 now)
{
    int16 previos[4];
    uint8 i, cambio=0;

    if(ctl_state!=CONTROL_RUNNING || !ctl_band || !ctl_profile) return;
    if(!control_pi_settled(&ctl_pi,&ctl_config,now)) return;
    if(ctl_autosave_n && (now-ctl_autosave_ms)<CTL_AUTOSAVE_MIN_MS) return;
    for(i=0;i<4;++i) if(ctl_codes[i]!=ctl_saved_codes[i]) cambio=1;
    if(!cambio) return;

    for(i=0;i<4;++i){previos[i]=ctl_saved_codes[i];ctl_saved_codes[i]=ctl_codes[i];}
    if(ctl_store()) {
        ctl_autosave_ms=now;
        /* Mismo criterio que el perfil (en banda, congelado y estable) para
         * anotar el punto de ESTA ganancia y bajar la tabla a la EEPROM. Es
         * lo que hace que el proximo arranque en cualquiera de los pares ya
         * visitados salte directo al punto en vez de re-aprenderlo. */
        ctl_punto_fijar_actual();
        ctl_emit(0x109,(int32)(++ctl_autosave_n),0);
    } else {
        for(i=0;i<4;++i) ctl_saved_codes[i]=previos[i];
    }
}

void control_service(void)
{
    uint32 now=ctl_ms;int32 counts;uint8 ready,saved;
    if(!ctl_initialized||ctl_state==CONTROL_PAUSED)return;
    saved=CyEnterCriticalSection();ready=ctl_ready;counts=ctl_average;ctl_ready=0;CyExitCriticalSection(saved);
    if(ready && ctl_channel==4) {
        int32 lp=(int32)(((int64)counts*1000000LL)/ADC_CF_2V5_COUNTS_PER_VOLT)-1000000;
        ctl_trace_v[ctl_trace_head]=lp;ctl_trace_ms[ctl_trace_head]=now;
        if(++ctl_trace_head>=CTL_TRACE_LEN){ctl_trace_head=0;ctl_trace_full=1;}
    }
    /* The trace keeps everything; the controller does not see readings taken
     * while the analog chain is still ringing from a telemetry burst. */
    if(ready && (int32)(ctl_quiet_until-now)>0)ready=0;
    if(ready) {
        /* sm_sample_cal_filt() already right-aligns the Delta-Sigma result.
         * ADC_CountsTo_uVolts() must not be used here: that generated helper
         * applies ADC_CF_2V5_DEC_DIV a second time (32 on this design).
         * Calibration forces CF_2V5, whose right-aligned scale is explicit. */
        int32 bank=(int32)(((int64)counts*1000000LL)/ADC_CF_2V5_COUNTS_PER_VOLT);
        ctl_med[ctl_med_i]=bank-1000000;ctl_med_i=(uint8)((ctl_med_i+1u)%CTL_MEDIAN_LEN);
        if(ctl_med_n<CTL_MEDIAN_LEN)++ctl_med_n;
    }
    if(ready && ctl_med_n<CTL_MEDIAN_LEN)ready=0;
    if(ready) {
        ctl_dc[ctl_channel]=ctl_median();
        ctl_valid[ctl_channel]=(uint8)control_measurement_valid(&ctl_config,ctl_dc[ctl_channel]);
        ctl_seen[ctl_channel]=1;ctl_stamp[ctl_channel]=now;
        if(ctl_channel==4) {
            /* "In band" is the controller's own verdict, so the flag (and its
             * I2C frame) only toggles when the loop freezes or wakes. */
            /* 2026-09-18: pedir ctl_pi.holding dejaba al nodo en ROJO
             * mientras dura un forzado, aunque LPo estuviera centrado —
             * el forzado justamente impide que se congele.  Lo que
             * importa es DONDE esta LPo, no si el lazo esta quieto. */
            uint8 inside=ctl_valid[4] &&
                (ctl_pi.holding ||
                 (ctl_dc[4]<=ctl_config.value[CP_DEADBAND_UV] &&
                  ctl_dc[4]>=-ctl_config.value[CP_DEADBAND_UV]));
            /* El umbral es DEADBAND y no HOLD por coherencia: dentro de
             * la banda muerta el controlador DECIDE no corregir (las
             * rafagas de telemetria mueven LPo +-50 mV, corregirlas es
             * perseguir ruido).  Declarar rojo a -29 mV mientras el lazo
             * se niega a tocar nada era incoherente: el semaforo tiene
             * que usar el mismo criterio de aceptacion que el lazo. */
            uint8 old=ctl_band;
            if(!inside){ctl_enter_start=now;ctl_band=0;}
            else if(now-ctl_enter_start>=(uint32)ctl_config.value[CP_ENTER_MS])ctl_band=1;
            if(old!=ctl_band)ctl_emit(0x102,ctl_band,0);
            if((ctl_state==CONTROL_RUNNING || (ctl_state==CONTROL_LEARNING && ctl_phase==2)) && now-ctl_last_pi>=(uint32)ctl_config.value[CP_PERIOD_MS] &&
               ctl_seen[3] && now-ctl_stamp[3]<=ctl_scan_ms(now)*2u+(uint32)ctl_config.value[CP_SCAN_MS]) {
                control_pi_step(&ctl_pi,ctl_effective_config(),now,ctl_dc[4],ctl_valid[4],ctl_dc[3],ctl_valid[3]);
                ctl_write(2,ctl_pi.coarse);ctl_write(3,ctl_pi.fine);ctl_last_pi=now;
            }
        }
        if(ctl_scan) {
            if(ctl_channel<4)ctl_select((uint8)(ctl_channel+1));
            else{ctl_scan=0;ctl_last_scan=now;}
        }
    }
    /* Vigilancia del bloqueo por SUMo.  Con SUMo fuera de su ventana la
     * guardia hace return antes del rescate de LPo, asi que si el
     * aprendizaje fallo el nodo se queda muerto para siempre.  Pasados
     * CTL_SUM_MUERTO_MS se vuelve a aprender en vez de quedarse asi. */
    if(ctl_state==CONTROL_RUNNING && ctl_seen[3] && !ctl_valid[3]) {
        if(ctl_sum_malo_desde==0u)ctl_sum_malo_desde=now;
        else if(now-ctl_sum_malo_desde>=CTL_SUM_MUERTO_MS) {
            ctl_sum_malo_desde=0u;
            ctl_punto_olvidar((uint8)ctl_config.value[CP_PGA],
                              (uint8)ctl_config.value[CP_PGAOUT]);
            (void)control_command(0xD6u,CONTROL_LEARN,0u);
        }
    } else ctl_sum_malo_desde=0u;
    /* Vigilancia del bloqueo por LPo.  SUMo puede estar perfecto y LPo contra
     * el riel igual: el pasabajos amplifica x16 desde el sumador y agrega su
     * propio offset, asi que el error puede nacer entero aguas abajo de SUMo y
     * la vigilancia de arriba no se entera nunca.
     *
     * Cuando eso pasa a PGAout bajo el nodo queda muerto: IDAC2 esta aguas
     * ARRIBA de PGAout, asi que su autoridad sobre LPo se divide por la
     * ganancia de esa etapa — a x1 son 24 veces menos que en el punto de
     * diseno, tanto que ctl_effective_config() le clava la pendiente en el
     * piso de 100 uV.  Con IDAC3 ya en su tope, el lazo rapido se queda sin
     * actuadores y el unico que conserva autoridad es el par lento, que solo
     * se mueve aprendiendo.  Medido el 2026-09-19 a x50/x1: LPo llego a +113
     * mV (tope de medicion) y se quedo ahi 280 s sin que se moviera un codigo.
     *
     * No alcanza con que LPo este invalido: hay que exigir tambien que el lazo
     * rapido NO este moviendose, para no re-aprender en mitad de un rescate
     * que estaba funcionando. */
    if(ctl_state==CONTROL_RUNNING && ctl_profile && ctl_seen[4] && !ctl_valid[4] &&
       magnitude((int32)ctl_pi.fine)>=ctl_config.value[CP_FINE_MID]) {
        if(ctl_lp_malo_desde==0u)ctl_lp_malo_desde=now;
        else if(now-ctl_lp_malo_desde>=CTL_LP_MUERTO_MS) {
            ctl_lp_malo_desde=0u;
            ctl_punto_olvidar((uint8)ctl_config.value[CP_PGA],
                              (uint8)ctl_config.value[CP_PGAOUT]);
            (void)control_command(0xD6u,CONTROL_LEARN,0u);
        }
    } else ctl_lp_malo_desde=0u;
    ctl_autosave(now);
    if(!ctl_scan && now-ctl_last_scan>=ctl_scan_ms(now)){ctl_scan=1;ctl_select(0);}
    if(ctl_state==CONTROL_LEARNING)ctl_learning(now);
    if(now-ctl_last_report>=(uint32)ctl_config.value[ctl_settled(now)?CP_REPORT_SETTLED_MS:CP_REPORT_MS]){ctl_report_start=ctl_ms;control_emit_snapshot(0);ctl_last_report=ctl_ms;}
}
uint8 control_command(uint8 cmd,uint8 p1,uint8 p2)
{
    uint8 i;
    if(ctl_state==CONTROL_PAUSED && !(cmd==0xD6 && p1==CONTROL_RESUME))return 0;
    if(cmd==0xD0){if(p1>=CP_COUNT)return 0;ctl_selected=p1;ctl_bytes=0;ctl_word=0;return 1;}
    if(cmd>=0xD1 && cmd<=0xD4){unsigned shift=8u*(cmd-0xD1);ctl_word=(ctl_word&~((uint32)255<<shift))|((uint32)p1<<shift);ctl_bytes|=(uint8)(1u<<(cmd-0xD1));return 1;}
    if(cmd==0xD5){if(ctl_bytes!=15)return 0;ctl_staged.value[ctl_selected]=(int32)ctl_word;ctl_bytes=0;return 1;}
    if(cmd==0xD7 && p1==0xFF){ctl_emit_trace();return 1;}
    if(cmd==0xD7){if(p1>=CP_COUNT)return 0;ctl_emit(p1,ctl_config.value[p1],0);return 1;}
    if(cmd!=0xD6)return 0;
    switch(p1) {
    case CONTROL_APPLY:
        if(ctl_state==CONTROL_LEARNING || !control_config_valid(&ctl_staged,PSOC_AMUX_SIGNAL_CHANNEL_COUNT,CAL_AMUX_HAS_CAP_CHANNEL))return 0;
        ctl_active=0;ADC_StopConvert();
        if(!control_config_profile_compatible(&ctl_staged,&ctl_profile_config))ctl_profile=0;
        ctl_config=ctl_staged;ctl_origin=2;ctl_report_count=0;
        psoc_hw_set_pga((uint8)ctl_config.value[CP_PGA]);psoc_hw_set_pgaout((uint8)ctl_config.value[CP_PGAOUT]);
        control_resume();return 1;
    case CONTROL_SAVE: if(ctl_state==CONTROL_LEARNING)return 0;return ctl_store();
    case CONTROL_DEFAULTS: control_config_defaults(&ctl_staged);return 1;
    case CONTROL_LEARN:
        if(ctl_state==CONTROL_LEARNING)return 0;
        ctl_learning_had_profile=ctl_profile;
        for(i=0;i<4;++i)ctl_learning_restore[i]=ctl_codes[i];
        ctl_punto_origen=CTL_ORIGEN_APRENDIZAJE;
        ctl_state=CONTROL_LEARNING;ctl_phase=0;ctl_learning_refinements=0;
        ctl_learning_slow_step=0;ctl_learning_have_pos=ctl_learning_have_neg=0;
        ctl_learning_start=ctl_phase_start=ctl_ms;
        ctl_bueno_desde=0u;
        ctl_learn_target=ctl_config.value[CP_OPA_TARGET_UV];
        ctl_lp_have_pos=ctl_lp_have_neg=ctl_target_libre=0u;ctl_target_paso=0;
        ctl_learning_prev_ms=ctl_ms;ctl_learning_prev_sum=ctl_dc[3];
        for(i=0;i<4;++i)ctl_write(i,i<2?0:(int16)ctl_config.value[i==2?CP_INITIAL2:CP_INITIAL3]);
        ctl_route_start();return 1;
    case CONTROL_PAUSE: control_prepare_capture();return 1;
    case CONTROL_RESUME: control_resume();return 1;
    case CONTROL_REPORT: ctl_report_count=0;control_emit_snapshot(0);return 1;
    /* Siembra de la tabla de puntos.  El dato NO viene en p1/p2 -sobran dos
     * bytes- sino en la palabra que quedo en CP_PUNTO_ARG con `ctl set`: esa
     * es la unica forma de meter 32 bits sin un APPLY, y un APPLY reinicia el
     * lazo en medio de un experimento. */
    case CONTROL_PUNTO_SET: {
        uint32 w=(uint32)ctl_staged.value[CP_PUNTO_ARG];
        uint8 par=(uint8)(w&0x7Fu), cual=(uint8)((w>>7)&1u);
        int16 a=(int16)((int32)((w>>8)&0x3FFu)-512);
        int16 b=(int16)((int32)((w>>18)&0x3FFu)-512);
        if(par>=CTL_PT_N||a<-255||a>255||b<-255||b>255)return 0;
        ctl_pt[par].codes[cual?2u:0u]=a;
        ctl_pt[par].codes[cual?3u:1u]=b;
        /* El commit viene en la SEGUNDA palabra: hasta que no estan los cuatro
         * codigos el registro no se marca valido ni se baja a la EEPROM. */
        if(w&0x10000000UL) {
            ctl_pt[par].estado=(uint8)(ctl_pt[par].estado|1u);
            ctl_pt_set_fallos(&ctl_pt[par],0u);
            ctl_pt[par].res_slow=0;ctl_pt[par].res_fast=0;
            ctl_pt_store_one(par);
        }
        return 1;
    }
    case CONTROL_PUNTO_DUMP: ctl_punto_dump(p2); return 1;
    case CONTROL_PUNTO_CLEAR:
        if(p2<CTL_PT_N) {
            if(ctl_pt_valido(&ctl_pt[p2])){ctl_pt[p2].estado=0u;ctl_pt_store_one(p2);}
        } else if(p2==0xFFu) {
            /* Solo se reescribe lo que estaba valido: borrar los 81 a ciegas
             * son 162 filas de EEPROM y varios segundos con el lazo parado. */
            for(i=0;i<CTL_PT_N;++i)
                if(ctl_pt_valido(&ctl_pt[i])){ctl_pt[i].estado=0u;ctl_pt_store_one(i);}
            for(i=0;i<CTL_CAND_N;++i)ctl_cand[i].par=0xFFu;
        } else return 0;
        return 1;
    /* La politica se cambia EN CALIENTE, sin APPLY: pasar de un brazo
     * experimental a otro no tiene por que reiniciar el nodo ni tirar el
     * punto de trabajo que ya estaba. */
    case CONTROL_PUNTO_POL:
        if(p2>(uint8)CTL_POL_INTERPOLADO)return 0;
        ctl_config.value[CP_PUNTO_POLICY]=(int32)p2;
        ctl_staged.value[CP_PUNTO_POLICY]=(int32)p2;
        return 1;
    default:return 0;
    }
}
