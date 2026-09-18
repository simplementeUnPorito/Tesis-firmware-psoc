#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../AcondicionamientoAnalogico.cydsn/control_config.h"

/* Plant in the ADC-bank domain, shaped after the 2026-09-16 hardware log
 * (geo-01, PGA x4, PGAout x24): IDAC3 is local to the LP stage (fast),
 * IDAC2 gives -80 mV at once and another -70 mV through the slow band-pass
 * tail, LPo reads +-3 mV of noise plus a 40 ms -30 mV dip every 2 s, SUMo
 * +-6 mV, rails at -261/+112 mV.  Readings go through the runtime's median
 * of 5 averages. */
typedef struct { int fine_changes, coarse_changes, outside, samples; double worst; } SimResult;
static double noise(double a) { return a*(2.0*rand()/RAND_MAX-1.0); }
static SimResult simulate(const ControlConfig *c, double fine_mv_true, double start_mv,
                          double drift_mv_per_min, unsigned minutes, unsigned judge_after_s)
{
    ControlPI p={0}; SimResult r={0,0,0,0,0.0};
    double tail=0.0, lp=start_mv, offset=start_mv;
    uint32_t t, last_pi=0; int16_t fine=0, coarse=0;
    double med[5]={0}; unsigned mi=0;
    srand(7);
    for(t=0;t<minutes*60000u;t+=20) {
        double dt=0.020, target, read, sum;
        int32_t lp_uv, sum_uv;
        offset+=drift_mv_per_min*dt/60.0;
        tail+=(-70.0*coarse-tail)*dt/35.0;
        target=offset+fine_mv_true*fine-80.0*coarse+tail;
        lp+=(target-lp)*dt/1.5;
        read=lp+noise(3.0)+((t%2000u)<40u ? -30.0 : 0.0);
        if(read>112.0)read=112.0;
        if(read<-261.0)read=-261.0;
        med[mi++%5]=read;
        { double m[5]; unsigned a,b; memcpy(m,med,sizeof m);
          for(a=1;a<5;++a)for(b=a;b>0&&m[b-1]>m[b];--b){double x=m[b];m[b]=m[b-1];m[b-1]=x;}
          read=m[2]; }
        sum=5.0+10.0*coarse+noise(6.0);
        lp_uv=(int32_t)(read*1000.0); sum_uv=(int32_t)(sum*1000.0);
        if(t-last_pi>=(uint32_t)c->value[CP_PERIOD_MS]) {
            int16_t f0=p.fine=fine, c0=p.coarse=coarse;
            control_pi_step(&p,c,t,lp_uv,control_measurement_valid(c,lp_uv),
                            sum_uv,control_measurement_valid(c,sum_uv));
            fine=p.fine; coarse=p.coarse; last_pi=t;
            if(t>=judge_after_s*1000u) { r.fine_changes+=fine!=f0; r.coarse_changes+=coarse!=c0; }
        }
        if(t>=judge_after_s*1000u) {
            ++r.samples;
            if(fabs(lp)>c->value[CP_SETTLED_BAND_UV]/1000.0+5.0)++r.outside;
            if(fabs(lp)>r.worst)r.worst=fabs(lp);
        }
    }
    return r;
}
int main(void)
{
    ControlConfig c,d;ControlPI p={0};uint8_t bytes[CONTROL_CONFIG_BYTES];unsigned i;int16_t paso,contra;int32_t hold,banda,bandaq;uint32_t t;
    setvbuf(stdout,NULL,_IONBF,0);
    control_config_defaults(&c);
    assert(control_config_valid(&c,5,1));
    assert(control_measurement_valid(&c,0));
    assert(!control_measurement_valid(&c,115600));
    assert(!control_measurement_valid(&c,-100000));
    assert(!control_config_valid(&c,5,0));
    control_config_encode(&c,bytes);control_config_decode(&d,bytes);
    assert(!memcmp(&c,&d,sizeof c));
    { uint16_t crc=control_crc(bytes,sizeof bytes);bytes[7]^=1;assert(crc!=control_crc(bytes,sizeof bytes)); }
    /* A v3 image carries 36 words; the new ones come from defaults. */
    d=c;d.value[CP_HOLD_UV]=1;control_config_encode(&d,bytes);
    control_config_decode_n(&d,bytes,CONTROL_CONFIG_COUNT_V3);
    assert(d.value[CP_HOLD_UV]==c.value[CP_HOLD_UV]);
    d=c;d.value[CP_KP_DEN]=0;assert(!control_config_valid(&d,5,1));
    /* The deadband must be inside the valid window: v3 had 100 mV against a
     * +-97 mV window, so the PI branch could never run. */
    assert(c.value[CP_DEADBAND_UV] < c.value[CP_VALID_HIGH_UV]-c.value[CP_MARGIN_UV]);
    d=c;d.value[CP_HOLD_UV]=d.value[CP_DEADBAND_UV];assert(!control_config_valid(&d,5,1));
    /* Retuning the fast loop keeps the learned slow profile. */
    d=c;d.value[CP_FINE_SLOPE_UV]=900;d.value[CP_PERIOD_MS]=20;assert(control_config_profile_compatible(&c,&d));
    d.value[CP_CAPACITOR]=0;assert(!control_config_profile_compatible(&c,&d));

    /* Histeresis: se congela dentro de HOLD, se queda quieto hasta DEADBAND y
     * despierta pasado eso.  Los umbrales salen de la config: se afinaron el
     * 2026-09-17 porque las rafagas de telemetria I2C entraban dentro de la
     * banda vieja y el lazo corregia el golpe, no la senal. */
    hold=c.value[CP_HOLD_UV]; banda=c.value[CP_DEADBAND_UV]; bandaq=c.value[CP_SETTLED_BAND_UV];
    memset(&p,0,sizeof p);p.fine=12;p.coarse=-4;
    control_pi_step(&p,&c,3000,hold/2,1,0,1);assert(p.holding&&p.fine==12);
    control_pi_step(&p,&c,6000,-(banda-2000),1,0,1);assert(p.holding&&p.fine==12);
    control_pi_step(&p,&c,9000,banda-1000,1,0,1);assert(p.holding&&p.fine==12);
    control_pi_step(&p,&c,12000,-2*banda,1,0,1);assert(!p.holding&&p.fine>12);
    /* Hold activo (< SETTLED_MS): una sola lectura pasada DEADBAND despierta. */
    memset(&p,0,sizeof p);
    control_pi_step(&p,&c,3000,hold/2,1,0,1);assert(p.holding&&!control_pi_settled(&p,&c,30000));
    control_pi_step(&p,&c,30000,banda+5000,1,0,1);assert(!p.holding);
    /* Modo estable (>= SETTLED_MS congelado): banda mas ancha y WAKE_COUNT
     * lecturas seguidas afuera; uina lectura adentro reinicia la cuenta. */
    memset(&p,0,sizeof p);
    control_pi_step(&p,&c,3000,hold/2,1,0,1);assert(control_pi_settled(&p,&c,63000));
    t=63000;
    for(i=1;i<(unsigned)c.value[CP_WAKE_COUNT];++i) {
        control_pi_step(&p,&c,t,bandaq+10000,1,0,1);assert(p.holding&&p.fine==0);
        t+=3000;
    }
    control_pi_step(&p,&c,t,hold/2,1,0,1);assert(p.holding&&p.wake_count==0);
    t+=3000;
    for(i=1;i<(unsigned)c.value[CP_WAKE_COUNT];++i) {
        control_pi_step(&p,&c,t,bandaq+10000,1,0,1);assert(p.holding);
        t+=3000;
    }
    control_pi_step(&p,&c,t,bandaq+10000,1,0,1);assert(!p.holding);
    /* A rail is never filtered by the settled mode. */
    memset(&p,0,sizeof p);
    control_pi_step(&p,&c,3000,4000,1,0,1);
    control_pi_step(&p,&c,90000,112000,0,0,1);
    assert(!p.holding&&p.fine==-c.value[CP_RESCUE_STEP]);
    /* SUMo inside its guard never touches IDAC2, however noisy. */
    memset(&p,0,sizeof p);p.coarse=-4;
    for(i=1;i<=100;++i)control_pi_step(&p,&c,i*3000,(i&1)?-30000:30000,1,(i&1)?-12000:12000,1);
    assert(p.coarse==-4);
    /* SUMo past the guard: one IDAC2 code, then its cooldown. */
    memset(&p,0,sizeof p);p.coarse=-4;
    control_pi_step(&p,&c,1000,0,1,70000,1);assert(p.coarse==-5);
    control_pi_step(&p,&c,20000,0,1,70000,1);assert(p.coarse==-5);
    control_pi_step(&p,&c,47000,0,1,70000,1);assert(p.coarse==-6);
    /* LPo on its positive rail: IDAC3 walks down by RESCUE_STEP, 10 s apart.
     * El paso sale de la config: con los 100 codigos de antes (y la pendiente
     * subestimada) el rescate cruzaba la ventana y rebotaba entre rieles. */
    memset(&p,0,sizeof p);p.fine=80;
    paso=(int16_t)c.value[CP_RESCUE_STEP];
    control_pi_step(&p,&c,1000,112000,0,0,1);assert(p.fine==80-paso);
    control_pi_step(&p,&c,5000,112000,0,0,1);assert(p.fine==80-paso);
    control_pi_step(&p,&c,11000,112000,0,0,1);assert(p.fine==80-2*paso);
    /* Fine at its limit on a rail: IDAC2 toma un codigo (-150 mV en LPo) y el
     * contramovimiento de IDAC3 son 150/2.5 = 60 codigos, no todo su rango. */
    memset(&p,0,sizeof p);p.fine=-255;p.coarse=0;
    control_pi_step(&p,&c,1000,112000,0,0,1);
    contra=(int16_t)((-c.value[CP_COARSE_SLOPE_UV]+c.value[CP_FINE_SLOPE_UV]/2)/c.value[CP_FINE_SLOPE_UV]);
    assert(p.coarse==1&&p.fine==-255+contra);
    /* Mid-ranging con el vernier sano: el contramovimiento de IDAC3 cancela al
     * codigo de IDAC2, asi que la prediccion lo rechaza y corrige con IDAC3. */
    memset(&p,0,sizeof p);p.fine=250;p.coarse=0;
    control_pi_step(&p,&c,1000,-50000,1,0,1);
    assert(p.coarse==0&&p.fine>250);
    /* ...never when the error itself brings IDAC3 back. */
    memset(&p,0,sizeof p);p.fine=250;p.coarse=0;
    control_pi_step(&p,&c,1000,50000,1,0,1);
    assert(p.coarse==0&&p.fine<250);

    /* Recentrado lento: congelado y pegado a un borde, mueve UN codigo hacia
     * el centro cada CP_RECENTER_MS y sigue congelado.  Sin esto la deriva lo
     * deja en el borde y la proxima correccion sale de golpe. */
    memset(&p,0,sizeof p);
    control_pi_step(&p,&c,3000,hold/2,1,0,1);assert(p.holding&&p.fine==0);
    t=3000+(uint32_t)c.value[CP_RECENTER_MS];
    control_pi_step(&p,&c,t,-(c.value[CP_RECENTER_UV]+3000),1,0,1);
    assert(p.holding&&p.fine>0);        /* un codigo hacia el centro */
    contra=p.fine;
    control_pi_step(&p,&c,t+3000,-(c.value[CP_RECENTER_UV]+3000),1,0,1);
    assert(p.holding&&p.fine==contra);  /* y espera otro periodo entero */
    /* Dentro del umbral no toca nada. */
    memset(&p,0,sizeof p);
    control_pi_step(&p,&c,3000,hold/2,1,0,1);
    control_pi_step(&p,&c,3000+(uint32_t)c.value[CP_RECENTER_MS],c.value[CP_RECENTER_UV]/2,1,0,1);
    assert(p.holding&&p.fine==0);

    /* Rescate por biseccion: arranca contra el riel y tiene que aterrizar
     * dentro de la ventana SIN saber la pendiente.  En esta cadena va de 0.8 a
     * 5.8 mV/codigo segun el punto de trabajo (medido 2026-09-17); con paso
     * fijo el lazo cruzaba la ventana entera y rebotaba entre rieles. */
    {
        static const double pend[]={0.5,1.0,2.5,5.8,8.0};
        unsigned j;
        for(j=0;j<sizeof pend/sizeof pend[0];++j) {
            double offset=400000.0;          /* uV: bien pasado el riel */
            uint32_t ahora=1000;
            int adentro=0;
            memset(&p,0,sizeof p);
            for(i=0;i<60 && !adentro;++i) {
                double real=offset+p.fine*pend[j]*1000.0
                            +p.coarse*(double)c.value[CP_COARSE_SLOPE_UV];
                int32_t medido=(int32_t)(real>130000.0?130000.0:(real<-130000.0?-130000.0:real));
                int val=control_measurement_valid(&c,medido);
                control_pi_step(&p,&c,ahora,medido,val,0,1);
                ahora+=(uint32_t)c.value[CP_RESCUE_MS];
                real=offset+p.fine*pend[j]*1000.0
                     +p.coarse*(double)c.value[CP_COARSE_SLOPE_UV];
                adentro=(real>-(double)c.value[CP_DEADBAND_UV] &&
                         real<(double)c.value[CP_DEADBAND_UV]);
            }
            printf("biseccion %.1f mV/codigo: %s en %u pasos (fine=%d)\n",
                   pend[j], adentro ? "adentro" : "NO ENTRO", i, p.fine);
            assert(adentro);
        }
    }

    /* Closed loop against the plant: converge, then stay quiet. */
    {
        /* Pendientes reales de IDAC3 medidas en placa, con dispersion. */
        static const double slopes[]={1.5,2.5,4.0};
        static const double starts[]={40.0,-40.0,100.0};
        for(i=0;i<3;++i) {
            unsigned j;
            for(j=0;j<3;++j) {
                SimResult r=simulate(&c,slopes[i],starts[j],5.0,30,180);
                printf("slope %.2f mV/code start %+6.0f mV: fine moves %3d, coarse %d, outside band %4.1f %%, worst %5.1f mV\n",
                       slopes[i],starts[j],r.fine_changes,r.coarse_changes,100.0*r.outside/r.samples,r.worst);
                if(getenv("NOASSERT"))continue;
                assert(r.outside*100<r.samples*2);
                /* 150 mV of drift in 30 min is more than all of IDAC3: a
                 * few IDAC2 codes are legitimate (below ~0.15 mV/code IDAC3 can no
                 * longer bridge the gap between two IDAC2 codes). */
                assert(r.fine_changes<=80);
                assert(r.coarse_changes<=8);
            }
        }
    }
    puts("control_test PASS: serialization, v3 migration, hysteresis, settled mode, SUMo guard, rescue, mid-ranging, closed loop");
    return 0;
}
