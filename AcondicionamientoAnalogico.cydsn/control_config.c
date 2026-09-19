#include "control_config.h"
#include <string.h>

static int32_t bounded(int32_t x, int32_t lo, int32_t hi)
{ return x < lo ? lo : (x > hi ? hi : x); }
static int64_t magnitude(int64_t x) { return x < 0 ? -x : x; }
void control_config_defaults(ControlConfig *c)
{
    static const int32_t defaults[CP_COUNT] = {
        /* Every voltage below is in the runtime ADC-bank domain (ctl_dc), the
         * only domain the controller observes.  Measured on geo-01, PGA x4,
         * PGAout x24, 2026-09-16: LPo reads +-3 mV of noise, SUMo +-6 mV, and
         * one IDAC2 code moves LPo about -150 mV (the whole valid window) and one
         * IDAC3 code only about +0.25 mV (-149 codes moved LPo -28 mV in closed
         * loop).  IDAC3's whole range (+-64 mV) is smaller than one IDAC2 code.
         *
         * 2026-09-17: ese 0.25 mV/codigo estaba medido con la etapa CONTRA EL
         * RIEL y subestima 10x.  Medido en transiciones limpias del propio
         * lazo: -120 codigos llevaron LPo de +89 mV a -216 mV, o sea
         * 2.5 mV/codigo.  Con el valor viejo cada rescate de 100 codigos
         * cruzaba la ventana entera (+-95 mV) y LPo rebotaba entre los dos
         * rieles sin aterrizar nunca.  RESCUE_STEP baja a 25 (~63 mV). */
        2, 5, 54, 1, 3000, 9000, 1, 4,
        /* DEADBAND is the outer band: inside it a held controller stays
         * still.  It must be narrower than the valid window (+-97 mV).
         * 2026-09-17: 20 mV era mas angosto que la patada que cada rafaga de
         * telemetria I2C le da a LPo, asi que el lazo corregia el golpe y no
         * la senal, y a veces lo mandaba al riel ya estando calibrado. */
        45000, 1000,
        /* RESCUE_MS baja de 60 s a 15: con paso expansivo + biseccion el
         * lazo se frena solo al acercarse, asi que esperar un minuto entre
         * pasos solo agrega demora (x4/x8 tardo 330 s por esto). */
        40, 25, 15000,
        /* SUMo band is a rail guard, not a target: IDAC3 absorbs a SUMo
         * offset of tens of mV, and a band inside SUMo's noise made IDAC2
         * chase noise forever, kicking LPo across its window each time. */
        60000, 1, 2500, -150000,
        /* Slow slopes: measured from the accepted 0 -> (1,-2) learning
         * transition in the same bank domain. */
        600, 400, 43700, 0, 500000, 1200000,
        /* Truthful ADC-bank window: 46158..58862 counts at 52429 counts/V. */
        1000, 5000, 4, 208, -119610, 122699, 25000,
        0, 0, 10000, 10000, 255, 255,
        /* HOLD: the PI freezes once |LPo| is this close; it only wakes again
         * outside DEADBAND.  FINE_MID: past it, and only while the error keeps
         * pushing IDAC3 further out, IDAC2 takes over one code.
         * COARSE_MS: IDAC2 cooldown, it drags the slow band-pass tail.
         * QUIET_MS: every telemetry burst to the ESP (I2C) kicks LPo by
         * +47/-54 mV starting ~20 ms later and settling in ~0.5 s (PSoC
         * trace, 2026-09-16); readings inside that window are discarded. */
        /* 2026-09-17: HOLD sube a 25 mV y QUIET a 1.5 s, por lo mismo:
         * quedarse quieto dentro de una banda comoda y no medir mientras la
         * cadena todavia repica despues de una trama. */
        25000, 240, 45000, 1500,
        /* Settled: the scan (every ~2 s) and the reports (every 5 s) are the
         * ticks left on the oscilloscope on 2026-09-16 16:34.  Once LPo has
         * been quiet for a minute they slow to 20 s / 30 s, and the loop only
         * wakes on 3 consecutive readings (9 s) beyond +-35 mV.
         * 2026-09-17: ya calibrado, despierta solo con 5 lecturas seguidas
         * fuera de +-60 mV, no por un golpe suelto. */
        60000, 60000, 5, 20000, 30000,
        /* Recentrado lento: si lleva 5 min congelado con |LPo| pasado 15 mV,
         * un solo codigo hacia el centro.  Es la unica forma de que la deriva
         * no lo deje pegado a un borde de la banda. */
        15000, 300000
    };
    memcpy(c->value, defaults, sizeof defaults);
}
int control_config_valid(const ControlConfig *c, unsigned channels, unsigned has_cap)
{
    unsigned i;
    static const int32_t low[CP_COUNT] = {
        0,0,1,0,20,1000,0,1,1000,0,1,1,100,1000,1,
        -10000000,-10000000,-1000000,-1000000,1000,-2000000,1000,10000,
        1000,100,0,128,-5000000,1000,0,-255,-255,100,1000,1,1,
        500,1,1000,0,
        0,1000,1,1000,1000,
        /* v5: recentrado lento (umbral y periodo; 0 = apagado). */
        0,0
    };
    static const int32_t high[CP_COUNT] = {
        8,8,4096,1,60000,600000,100,100,1000000,60000,255,255,600000,
        2400000,8,10000000,10000000,1000000,1000000,120000,2000000,
        1000000,3600000,600000,60000,4,26040,-1000,5000000,2000000,
        255,255,100000,120000,255,255,
        1000000,255,600000,10000,
        3600000,1000000,20,600000,600000,
        1000000,3600000
    };
    for (i=0; i<CP_COUNT; ++i)
        if(c->value[i]<low[i] || c->value[i]>high[i]) return 0;
    if ((unsigned)c->value[CP_CAPTURE_CHANNEL]>=channels ||
        (c->value[CP_CAPACITOR] && !has_cap)) return 0;
    if (magnitude(c->value[CP_FINE_SLOPE_UV])<100 ||
        magnitude(c->value[CP_COARSE_SLOPE_UV])<100 ||
        magnitude(c->value[CP_SLOW0_SLOPE_UV])<100 ||
        magnitude(c->value[CP_SLOW1_SLOPE_UV])<100) return 0;
    if (c->value[CP_LEARN_TIMEOUT_MS] < 4*c->value[CP_SLOW_TAU_MS]+c->value[CP_STABLE_MS]) return 0;
    if (c->value[CP_MARGIN_UV]*2 >= c->value[CP_VALID_HIGH_UV]-c->value[CP_VALID_LOW_UV]) return 0;
    /* Hysteresis needs a hold band strictly inside the wake band. */
    if (c->value[CP_HOLD_UV] >= c->value[CP_DEADBAND_UV]) return 0;
    if (c->value[CP_SETTLED_BAND_UV] < c->value[CP_DEADBAND_UV]) return 0;
    if (c->value[CP_FINE_MID] > c->value[CP_FINE_LIMIT]) return 0;
    return 1;
}
int control_config_profile_compatible(const ControlConfig *a, const ControlConfig *b)
{
    /* The learned profile is the slow pair IDAC0/IDAC1.  Fast-loop tuning
     * (IDAC2/IDAC3 slopes, initial codes) does not invalidate it; retuning
     * the PI must never throw the node back to "needs learning". */
    static const unsigned keys[] = {CP_PGA,CP_PGAOUT,CP_CAPACITOR,
        CP_SLOW0_SLOPE_UV,CP_SLOW1_SLOPE_UV,CP_OPA_TARGET_UV};
    unsigned i;
    for(i=0;i<sizeof keys/sizeof keys[0];++i)
        if(a->value[keys[i]]!=b->value[keys[i]]) return 0;
    return 1;
}
void control_config_encode(const ControlConfig *c, uint8_t *out)
{
    unsigned i,j;
    for(i=0;i<CP_COUNT;++i) for(j=0;j<4;++j)
        out[4*i+j]=(uint8_t)((uint32_t)c->value[i]>>(8*j));
}
void control_config_decode(ControlConfig *c, const uint8_t *in)
{ control_config_decode_n(c,in,CP_COUNT); }
void control_config_decode_n(ControlConfig *c, const uint8_t *in, unsigned n)
{
    unsigned i;
    control_config_defaults(c);
    for(i=0;i<n && i<CP_COUNT;++i) c->value[i]=(int32_t)((uint32_t)in[4*i] |
        ((uint32_t)in[4*i+1]<<8) | ((uint32_t)in[4*i+2]<<16) | ((uint32_t)in[4*i+3]<<24));
}
uint16_t control_crc(const uint8_t *data, unsigned n)
{
    uint16_t crc=0xffffu; unsigned i;
    while(n--) { crc^=(uint16_t)*data++<<8;
        for(i=0;i<8;++i) crc=(uint16_t)((crc<<1)^((crc&0x8000u)?0x1021u:0)); }
    return crc;
}
int control_measurement_valid(const ControlConfig *c, int32_t dc_uv)
{
    int64_t low=(int64_t)c->value[CP_VALID_LOW_UV]+c->value[CP_MARGIN_UV];
    int64_t high=(int64_t)c->value[CP_VALID_HIGH_UV]-c->value[CP_MARGIN_UV];
    return (int64_t)dc_uv>=low && (int64_t)dc_uv<=high;
}

/* Move the coarse actuator and, where fine range permits, counter-move the
 * fine actuator so the total predicted LP correction is continuous.  This is
 * especially important after fine reaches a rail: coarse creates headroom
 * instead of throwing the chain across the opposite rail. */
static void move_coarse_bumpless(ControlPI *p,const int32_t *v,int32_t step)
{
    int32_t old_coarse=p->coarse;
    int32_t next_coarse=bounded(old_coarse+step,-v[CP_COARSE_LIMIT],v[CP_COARSE_LIMIT]);
    int32_t moved=next_coarse-old_coarse;
    int64_t numerator;
    int32_t fine_delta;
    if(moved==0)return;
    numerator=-(int64_t)moved*v[CP_COARSE_SLOPE_UV];
    if(numerator>=0) numerator+=(int64_t)magnitude(v[CP_FINE_SLOPE_UV])/2;
    else numerator-=(int64_t)magnitude(v[CP_FINE_SLOPE_UV])/2;
    fine_delta=(int32_t)(numerator/v[CP_FINE_SLOPE_UV]);
    p->coarse=(int16_t)next_coarse;
    p->fine=(int16_t)bounded(p->fine+fine_delta,-v[CP_FINE_LIMIT],v[CP_FINE_LIMIT]);
}
static void pi_forget(ControlPI *p) { p->has_previous=0; p->fraction=0; }
int control_pi_settled(const ControlPI *p,const ControlConfig *c,uint32_t now)
{ return p->holding && now-p->hold_ms >= (uint32_t)c->value[CP_SETTLED_MS]; }
/* Aprende la pendiente real de IDAC3 mirando lo que consiguio el ultimo
 * movimiento: uV de LPo por codigo, con signo.  Se descartan los pasos
 * chicos (ruido) y los saltos absurdos (lecturas contra el riel). */
static void slope_observe(ControlPI *p,const int32_t *v,int32_t lp)
{
    int32_t dfine,dlp,est;
    if(p->slope_armed) {
        dfine=(int32_t)p->fine-(int32_t)p->slope_fine0;
        dlp=lp-p->slope_lp0;
        /* 2 codigos alcanza: con 4 el observador no aprendia nunca durante
         * el PI, que mueve de a uno, y el lazo se quedaba con la pendiente
         * de la config justo en la franja fina donde mas duele. */
        if(dfine>=2 || dfine<=-2) {
            est=dlp/dfine;
            if(est<0)est=-est;
            if(est>=100 && est<=10*magnitude(v[CP_FINE_SLOPE_UV]))
                p->slope_uv=(v[CP_FINE_SLOPE_UV]<0)?-est:est;
        }
    }
    p->slope_fine0=p->fine;p->slope_lp0=lp;p->slope_armed=1;
}
static int32_t slope_now(const ControlPI *p,const int32_t *v)
{ return p->slope_uv ? p->slope_uv : v[CP_FINE_SLOPE_UV]; }
static int forcing(const ControlPI *p,uint32_t now)
{ return p->force_until_ms && (int32_t)(now-p->force_until_ms)<0; }
/* Ritmo de los actuadores mientras se fuerza. No sale de tau sino de lo que
 * hay que recorrer: medido el 2026-09-18, tras un cambio de ganancia LPo
 * queda contra un riel a ~270 mV del centro, o sea unos nueve pasos de
 * rescate. A 60 s el paso eso son nueve minutos; a 10 s, minuto y medio, que
 * entra holgado en la ventana de forzado. La biseccion sigue partiendo el
 * paso al medio cada vez que se pasa, asi que ir mas rapido no lo vuelve
 * agresivo cerca del centro: solo deja de ser lento cuando esta lejos. */
#define CTL_FORCED_STEP_MS 10000u
/* Tope del paso del rescate.  64 codigos son ~1 V en LPo: mas que la ventana
 * entera, asi que no hace falta mas, y con la biseccion de vuelta no se
 * vuelve agresivo cerca del centro. */
#define CTL_RESCUE_STEP_MAX 64
/* Tope de codigos de IDAC2 por paso cuando el vernier ya esta saturado. */
#define CTL_COARSE_RUN_MAX  16
/* El enfriamiento del grueso NO se acorta ni forzando, y esto costo una
 * corrida entera de aprenderlo: IDAC2 arrastra la cola lenta del pasabanda
 * (tau ~34 s medidos) y su contramovimiento 'bumpless' solo cancela la parte
 * INSTANTANEA.  Con el grueso caminando cada 10 s las colas se apilan y
 * llegan juntas decenas de segundos despues: medido el 2026-09-18 a x8/x8,
 * LPo se quedaba plano en -269 mV cien segundos y despues cruzaba la ventana
 * entera hasta el riel opuesto en treinta.  Al actuador que arrastra una cola
 * de 34 s no se lo puede apurar; el que si puede ir rapido es el vernier. */
static int coarse_ready(const ControlPI *p,const int32_t *v,uint32_t now)
{ return !p->has_coarse || now-p->coarse_ms >= (uint32_t)v[CP_COARSE_MS]; }
static void coarse_moved(ControlPI *p,uint32_t now)
{ p->coarse_ms=now; p->has_coarse=1; }
/* Un paso de BISECCION sobre el vernier: expande mientras el signo no
 * cambia y parte al medio cuando cambia.  Solo usa el SIGNO del error, y
 * por eso anda con cualquier pendiente — que en esta cadena varia 25x
 * segun el punto de trabajo (0,6 a 15 mV/codigo, medido 2026-09-18). */
static void bisect_fine(ControlPI *p,const int32_t *v,uint32_t now,int32_t e)
{
    int32_t step=0;
    int dir;
    uint32_t espera_rescate=(uint32_t)v[CP_RESCUE_MS];
    {
        if(forcing(p,now) && CTL_FORCED_STEP_MS<espera_rescate)
            espera_rescate=CTL_FORCED_STEP_MS;
        if(!p->has_rescue || now-p->rescue_ms >= espera_rescate) {
            dir=((e>0)==(v[CP_FINE_SLOPE_UV]>0)) ? 1 : -1;
            if(!p->has_rescue || p->rescue_step<=0) {
                p->rescue_step=(int16_t)v[CP_RESCUE_STEP];
                p->rescue_dir=0;
                p->rescue_bracket=0;
                p->coarse_run=0;
            } else if(p->rescue_dir!=0 && dir!=p->rescue_dir) {
                /* Cambio de signo: se cruzo la ventana.  Termina la fase de
                 * EXPANSION para siempre —ya hay bracket— y de aca en mas el
                 * paso solo se parte al medio.  Sin esta bandera el rescate
                 * partia el paso al cruzar y lo volvia a DUPLICAR en cuanto el
                 * sentido coincidia dos veces seguidas, asi que nunca achicaba
                 * de verdad: medido el 2026-09-19 a x50/x1, ciclo limite de
                 * riel a riel con periodo de ~280 s y amplitud constante. */
                p->rescue_bracket=1;
                if(p->rescue_step>1)p->rescue_step=(int16_t)(p->rescue_step/2);
            } else if(!p->rescue_bracket && p->rescue_dir==dir &&
                      p->rescue_step<CTL_RESCUE_STEP_MAX) {
                /* MISMO sentido: sigue lejos y el paso se queda corto.  Sin
                 * esto el rescate camina de a CP_RESCUE_STEP para siempre:
                 * medido el 2026-09-18, salir de un riel de 270 mV a dos
                 * codigos por vuelta son nueve minutos.  Duplicando se llega
                 * en tres o cuatro vueltas, y el paso se parte al medio
                 * apenas se cruza — expansion + biseccion, sin depender de
                 * ninguna pendiente. */
                p->rescue_step=(int16_t)(p->rescue_step*2);
            }
            p->rescue_dir=(int8_t)dir;
            step=dir*p->rescue_step;
            /* Escalar al grueso en FINE_MID y no en el limite duro: con el
             * vernier en 225 de 255 el rescate seguia caminando de a dos
             * codigos y tardaba 15 min en llegar al tope antes de tocar
             * IDAC2, con LPo contra el riel todo ese tiempo (medido a
             * x50/x1 y x8/x8 el 2026-09-18). */
            if((p->fine>=v[CP_FINE_MID] && step>0)||(p->fine<=-v[CP_FINE_MID] && step<0)) {
                if(coarse_ready(p,v,now)) {
                    /* UN codigo por enfriamiento, y nada mas.  El 2026-09-19
                     * probe expandir aca tambien —duplicando mientras el signo
                     * no cambiara, como hace el vernier— para que IDAC2 saliera
                     * mas rapido del riel a PGAout x1.  REGRESIONO x8/x8, que
                     * venia pasando: el corte de la expansion depende de ver el
                     * cambio de signo, y el efecto de IDAC2 tarda los ~44 s de
                     * la cola del pasabanda en llegar, asi que duplica a ciegas
                     * varios enfriamientos y se pasa de largo.  Expandir un
                     * actuador cuyo efecto llega mas tarde que el proximo paso
                     * no se puede hacer a lazo cerrado sobre la lectura
                     * instantanea. */
                    move_coarse_bumpless(p,v,((e>0)==(v[CP_COARSE_SLOPE_UV]>0)) ? 1 : -1);
                    coarse_moved(p,now);
                }
            } else p->fine=(int16_t)bounded(p->fine+step,-v[CP_FINE_LIMIT],v[CP_FINE_LIMIT]);
            p->rescue_ms=now; p->has_rescue=1;
        }
    }
}

void control_pi_force(ControlPI *p,uint32_t now,uint32_t ms)
{
    if(ms==0u){p->force_until_ms=0;return;}
    p->force_until_ms=now+ms;
    if(p->force_until_ms==0u)p->force_until_ms=1u;   /* 0 significa 'sin forzado' */
    p->holding=0;p->wake_count=0;p->has_previous=0;p->fraction=0;
}

void control_pi_step(ControlPI *p,const ControlConfig *c,uint32_t now,
                     int32_t lp,int valid,int32_t sum,int sum_valid)
{
    const int32_t *v=c->value;
    int32_t step=0, e=-lp;
    uint32_t dt=p->has_previous ? now-p->previous_ms : (uint32_t)v[CP_PERIOD_MS];
    int64_t increment, denom;
    /* 1. SUMo guard.  Only a rail or a gross offset moves IDAC2 here: IDAC3
     * absorbs tens of mV of SUMo offset, whereas one IDAC2 code throws LPo
     * across its whole window and drags the slow band-pass tail. */
    if(!sum_valid || magnitude(sum)>v[CP_SUM_BAND_UV]) {
        pi_forget(p); p->holding=0;
        if(coarse_ready(p,v,now)) {
            /* Coarse changes affect SUM with opposite sign to their LP effect. */
            step=((sum>0)==(v[CP_COARSE_SLOPE_UV]>0)) ? v[CP_COARSE_STEP] : -v[CP_COARSE_STEP];
            p->coarse=(int16_t)bounded(p->coarse+step,-v[CP_COARSE_LIMIT],v[CP_COARSE_LIMIT]);
            coarse_moved(p,now);
        }
        /* La guardia NO puede dejar al vernier sin correr.  Hacia `return`
         * siempre, y con SUMo fuera de banda el rescate de LPo no se ejecutaba
         * nunca: medido el 2026-09-19 a x50/x1, LPo estuvo 200 s contra el riel
         * moviendose UN codigo de IDAC2 cada 24 s mientras IDAC3 no se movio
         * ni una vez.  El rescate de LPo es una biseccion que no mira SUMo
         * para nada —IDAC3 esta aguas abajo—, asi que dejarlo correr es seguro
         * y es la unica forma de salir del riel a tiempo. */
        if(!valid) { bisect_fine(p,v,now,e); }
        return;
    }
    /* 2. LPo contra un riel: su magnitud no dice nada, solo el signo.  El
     * rescate es una BISECCION: se camina IDAC3 en el sentido del signo y cada
     * vez que el signo se da vuelta (o sea que se cruzo la ventana) el paso se
     * parte al medio.  Asi no depende de la pendiente, que en esta cadena
     * cambia de 0.8 a 5.8 mV/codigo segun donde este parada (medido el
     * 2026-09-17): con paso fijo el lazo rebotaba entre los dos rieles para
     * siempre.  CP_RESCUE_STEP es solo el primer paso. */
    /* Lejos del objetivo se usa BISECCION, no PI.  La pendiente de IDAC3 no es
     * unica: medida 6,68 mV/codigo a x1/x1, ~15 a x4/x24 y 0,6 el 2026-09-18 en
     * otro punto del mismo par — 25 veces menos que lo que dice la config.  El
     * PI multiplica el error por esa pendiente, asi que con ese error de modelo
     * corrige 25 veces de menos y tarda una eternidad.  La biseccion solo mira
     * el SIGNO y converge igual con cualquier pendiente; el PI queda para el
     * ajuste fino dentro de DEADBAND, donde el error ya es chico. */
    if(valid)slope_observe(p,v,lp);
    /* 2. LPo contra un riel: la biseccion nunca se filtra por histeresis,
     * un riel es un riel. */
    if(!valid) {
        pi_forget(p); p->holding=0;
        bisect_fine(p,v,now,e);
        return;
    }
    /* OJO: el bracket de la biseccion NO se borra por el solo hecho de que
     * LPo haya vuelto a ser medible.  Se borraba aca, y eso era lo que
     * impedia que la biseccion partiera el paso: con un paso grande LPo cruza
     * la ventana entera en un salto, pasa por "valido" un instante en el
     * medio, y al llegar al riel opuesto el rescate arrancaba de cero otra vez
     * con el paso inicial y lo volvia a cruzar.  El resultado es un ciclo
     * limite de riel a riel que no converge nunca — medido el 2026-09-19 a
     * x50/x1: periodo de ~280 s, amplitud constante, horas asi.
     *
     * El bracket se borra cuando el lazo realmente se planta (entra en HOLD):
     * ahi si la busqueda termino y la proxima empieza limpia. */
    /* 2.b Ventana de forzado: mientras dura, el lazo no se congela. No es un
     * modo aparte — solo se saltea la histeresis, todo lo demas (banda de
     * validez, rescate, mid-ranging, PI) sigue igual. */
    if(p->force_until_ms) {
        if(forcing(p,now)) { p->holding=0; p->wake_count=0; }
        else p->force_until_ms=0;
    }
    /* 3. Hysteresis.  Once LPo is inside HOLD the loop freezes and ignores
     * every sample until LPo leaves DEADBAND.  Without it the integer loop
     * chased +-3 mV of ADC noise and never stopped moving. */
    if(p->holding) {
        int settled=control_pi_settled(p,c,now);
        int32_t band=settled ? v[CP_SETTLED_BAND_UV] : v[CP_DEADBAND_UV];
        pi_forget(p);
        if(magnitude(lp)<=band) {
            p->wake_count=0;
            /* Recentrado lento: un codigo, y sigue congelado.  Sin esto la
             * deriva deja a LPo pegado al borde de la banda y la proxima
             * correccion sale de golpe desde ahi. */
            if(v[CP_RECENTER_MS]>0 && magnitude(lp)>v[CP_RECENTER_UV] &&
               now-p->hold_ms>=(uint32_t)v[CP_RECENTER_MS] &&
               now-p->recenter_ms>=(uint32_t)v[CP_RECENTER_MS]) {
                int32_t paso=((-lp>0)==(v[CP_FINE_SLOPE_UV]>0)) ? 1 : -1;
                p->fine=(int16_t)bounded(p->fine+paso,-v[CP_FINE_LIMIT],v[CP_FINE_LIMIT]);
                p->recenter_ms=now;
            }
            /* Descargue del vernier EN BANDA.  El mid-ranging del paso 4 solo
             * corre con el lazo despierto, asi que un nodo que entro en banda
             * con IDAC3 contra su tope se quedaba ahi: sostiene el punto pero
             * sin autoridad para el lado ya agotado, y la primera perturbacion
             * lo manda al riel (visto 2026-09-18: IDAC3=248 con FINE_MID=240,
             * quieto y en banda durante minutos).  Se descarga sin despertar
             * el lazo y solo si el movimiento bumpless predice quedarse dentro
             * de la banda de hold: si no, se deja como esta, que no es peor
             * que hoy. */
            if(magnitude(p->fine)>=v[CP_FINE_MID] && coarse_ready(p,v,now)) {
                int32_t dir=p->fine>0 ? 1 : -1;
                int32_t moved=((v[CP_COARSE_SLOPE_UV]>0)==(v[CP_FINE_SLOPE_UV]>0)) ? dir : -dir;
                ControlPI trial=*p;
                int64_t lp_after;
                move_coarse_bumpless(&trial,v,moved);
                lp_after=(int64_t)lp+(int64_t)(trial.coarse-p->coarse)*v[CP_COARSE_SLOPE_UV]
                         +(int64_t)(trial.fine-p->fine)*v[CP_FINE_SLOPE_UV];
                if(magnitude(trial.fine)<magnitude(p->fine) &&
                   magnitude((int32_t)lp_after)<=v[CP_HOLD_UV]) {
                    p->coarse=trial.coarse; p->fine=trial.fine;
                    coarse_moved(p,now); pi_forget(p);
                }
            }
            return;
        }
        /* Settled: a lone reading outside is a glitch, not drift. */
        if(settled && ++p->wake_count < (uint8_t)v[CP_WAKE_COUNT]) return;
        p->holding=0; p->wake_count=0;
    } else if(magnitude(lp)<=v[CP_HOLD_UV]) {
        p->holding=1; p->hold_ms=now; p->wake_count=0; pi_forget(p);
        p->has_rescue=0; p->rescue_step=0; p->rescue_dir=0;
        p->rescue_bracket=0; p->coarse_run=0;
        return;
    }
    /* 4. Mid-ranging: IDAC3 is at the end of its range and the error still
     * asks for more in that direction.  One IDAC2 code is larger than all of
     * IDAC3, so the counter-move usually swings IDAC3 to the other end; that
     * is the smallest LPo jump available.  An error that would bring IDAC3
     * back by itself never moves IDAC2. */
    if(magnitude(p->fine)>=v[CP_FINE_MID] &&
       (p->fine>0)==((e>0)==(v[CP_FINE_SLOPE_UV]>0)) && coarse_ready(p,v,now)) {
        int32_t dir=p->fine>0 ? 1 : -1;
        /* Bumpless fine change is -moved*coarse/fine: pick the sign of
         * `moved` that pulls fine back toward zero. */
        int32_t moved=((v[CP_COARSE_SLOPE_UV]>0)==(v[CP_FINE_SLOPE_UV]>0)) ? dir : -dir;
        ControlPI trial=*p;
        int64_t lp_after;
        move_coarse_bumpless(&trial,v,moved);
        lp_after=(int64_t)lp+(int64_t)(trial.coarse-p->coarse)*v[CP_COARSE_SLOPE_UV]
                 +(int64_t)(trial.fine-p->fine)*v[CP_FINE_SLOPE_UV];
        /* Only if the predicted error is really smaller.  Otherwise LPo sits
         * in a gap between two IDAC2 codes that IDAC3 cannot bridge: moving
         * would just flip the error's sign and come back 45 s later. */
        if(magnitude(lp_after)+v[CP_HOLD_UV] < magnitude(lp)) {
            p->coarse=trial.coarse; p->fine=trial.fine;
            coarse_moved(p,now); pi_forget(p);
            return;
        }
    }
    /* 4.b Lejos del objetivo: biseccion en vez de PI.  El PI multiplica el
     * error por CP_FINE_SLOPE_UV, y con ese modelo errado 25x corrige 25
     * veces de menos: medido el 2026-09-18, LPo tardo 300 s en bajar de
     * +113 a +23 mV.  Aca ya paso el filtro de histeresis, asi que esto no
     * reacciona a golpes sueltos de telemetria.
     * 2026-09-18, segunda medicion: con el umbral en DEADBAND (45 mV) el
     * lazo se plantaba a -36 mV en x16/x4 y nunca llegaba a los +-25 que
     * hacen falta para declarar banda, porque esa franja la corregia el PI
     * con la pendiente errada.  El umbral baja a HOLD: si el lazo decidio
     * actuar, actua por biseccion. */
    if(magnitude(lp)>v[CP_DEADBAND_UV]) { bisect_fine(p,v,now,e); return; }
    /* 5. PI toward zero, in fractional codes.  alpha=T/tau of the error per
     * period plus Kp on its change; tolerates ~1/alpha of slope error. */
    if(dt>(uint32_t)v[CP_PERIOD_MS]*2u) dt=(uint32_t)v[CP_PERIOD_MS];
    denom=(int64_t)v[CP_TAU_MS]*slope_now(p,v);
    increment=((int64_t)e+(p->has_previous?p->previous_error:e))*dt*32768/denom;
    increment+=(int64_t)(e-(p->has_previous?p->previous_error:0))*v[CP_PERIOD_MS]*65536/denom*v[CP_KP_NUM]/v[CP_KP_DEN];
    p->fraction+=increment;
    step=(int32_t)(p->fraction/65536);
    step=bounded(step,-v[CP_FINE_STEP],v[CP_FINE_STEP]);
    /* Piso de un codigo: si el incremento fraccionario se trunca a cero pero
     * LPo esta fuera de HOLD, hay que moverse igual.  Sin esto el lazo se
     * plantaba a -32 mV y nunca llegaba a los +-25 que declaran banda (medido
     * 2026-09-18 en x16/x4): el modelo de pendiente hacia el incremento tan
     * chico que truncaba a cero una y otra vez. */
    if(step==0 && magnitude(lp)>v[CP_HOLD_UV])
        step=((e>0)==(slope_now(p,v)>0)) ? 1 : -1;
    if(step!=0) {
        int32_t next=bounded(p->fine+step,-v[CP_FINE_LIMIT],v[CP_FINE_LIMIT]);
        p->fraction-=((int64_t)(next-p->fine))*65536;
        if(next==v[CP_FINE_LIMIT] || next==-v[CP_FINE_LIMIT] || magnitude(p->fraction)>65536) p->fraction=0;
        p->fine=(int16_t)next;
    }
    p->previous_error=e; p->previous_ms=now; p->has_previous=1;
}
