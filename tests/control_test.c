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
    ControlConfig c,d;ControlPI p={0};uint8_t bytes[CONTROL_CONFIG_BYTES];unsigned i;
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

    /* Hysteresis: hold at 5 mV, stay still up to the 20 mV deadband, wake past it. */
    memset(&p,0,sizeof p);p.fine=12;p.coarse=-4;
    control_pi_step(&p,&c,3000,5000,1,0,1);assert(p.holding&&p.fine==12);
    control_pi_step(&p,&c,6000,-18000,1,0,1);assert(p.holding&&p.fine==12);
    control_pi_step(&p,&c,9000,19000,1,0,1);assert(p.holding&&p.fine==12);
    control_pi_step(&p,&c,12000,-40000,1,0,1);assert(!p.holding&&p.fine>12);
    /* Active hold (< SETTLED_MS): one reading past 20 mV wakes the loop. */
    memset(&p,0,sizeof p);
    control_pi_step(&p,&c,3000,4000,1,0,1);assert(p.holding&&!control_pi_settled(&p,&c,30000));
    control_pi_step(&p,&c,30000,25000,1,0,1);assert(!p.holding);
    /* Settled (>= 60 s frozen): +-35 mV band, and 3 readings in a row. */
    memset(&p,0,sizeof p);
    control_pi_step(&p,&c,3000,4000,1,0,1);assert(control_pi_settled(&p,&c,63000));
    control_pi_step(&p,&c,63000,30000,1,0,1);assert(p.holding&&p.fine==0);
    control_pi_step(&p,&c,66000,50000,1,0,1);assert(p.holding);
    control_pi_step(&p,&c,69000,10000,1,0,1);assert(p.holding&&p.wake_count==0);
    control_pi_step(&p,&c,72000,50000,1,0,1);assert(p.holding);
    control_pi_step(&p,&c,75000,50000,1,0,1);assert(p.holding&&p.fine==0);
    control_pi_step(&p,&c,78000,50000,1,0,1);assert(!p.holding&&p.fine<0);
    /* A rail is never filtered by the settled mode. */
    memset(&p,0,sizeof p);
    control_pi_step(&p,&c,3000,4000,1,0,1);
    control_pi_step(&p,&c,90000,112000,0,0,1);assert(!p.holding&&p.fine==-100);
    /* SUMo inside its guard never touches IDAC2, however noisy. */
    memset(&p,0,sizeof p);p.coarse=-4;
    for(i=1;i<=100;++i)control_pi_step(&p,&c,i*3000,(i&1)?-30000:30000,1,(i&1)?-12000:12000,1);
    assert(p.coarse==-4);
    /* SUMo past the guard: one IDAC2 code, then its cooldown. */
    memset(&p,0,sizeof p);p.coarse=-4;
    control_pi_step(&p,&c,1000,0,1,70000,1);assert(p.coarse==-5);
    control_pi_step(&p,&c,20000,0,1,70000,1);assert(p.coarse==-5);
    control_pi_step(&p,&c,47000,0,1,70000,1);assert(p.coarse==-6);
    /* LPo on its positive rail: IDAC3 walks down by RESCUE_STEP, 10 s apart. */
    memset(&p,0,sizeof p);p.fine=80;
    control_pi_step(&p,&c,1000,112000,0,0,1);assert(p.fine==-20);
    control_pi_step(&p,&c,5000,112000,0,0,1);assert(p.fine==-20);
    control_pi_step(&p,&c,11000,112000,0,0,1);assert(p.fine==-120);
    /* Fine at its limit on a rail: IDAC2 takes one code (-150 mV on LPo) and
     * IDAC3 swings to its other end (+64 mV), the smallest jump available. */
    memset(&p,0,sizeof p);p.fine=-255;p.coarse=0;
    control_pi_step(&p,&c,1000,112000,0,0,1);
    assert(p.coarse==1&&p.fine==255);
    /* Mid-ranging only while the error pushes IDAC3 further out... */
    memset(&p,0,sizeof p);p.fine=250;p.coarse=0;
    control_pi_step(&p,&c,1000,-50000,1,0,1);
    assert(p.coarse==-1&&p.fine==-255);
    /* ...never when the error itself brings IDAC3 back. */
    memset(&p,0,sizeof p);p.fine=250;p.coarse=0;
    control_pi_step(&p,&c,1000,50000,1,0,1);
    assert(p.coarse==0&&p.fine<250);

    /* Closed loop against the plant: converge, then stay quiet. */
    {
        static const double slopes[]={0.18,0.25,0.5};
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
