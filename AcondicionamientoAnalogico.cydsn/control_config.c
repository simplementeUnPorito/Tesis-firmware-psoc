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
         * loop).  IDAC3's whole range (+-64 mV) is smaller than one IDAC2 code. */
        2, 5, 54, 1, 3000, 9000, 1, 4,
        /* DEADBAND is the outer band: inside it a held controller stays
         * still.  It must be narrower than the valid window (+-97 mV). */
        20000, 1000,
        40, 100, 10000,
        /* SUMo band is a rail guard, not a target: IDAC3 absorbs a SUMo
         * offset of tens of mV, and a band inside SUMo's noise made IDAC2
         * chase noise forever, kicking LPo across its window each time. */
        60000, 1, 250, -150000,
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
        6000, 240, 45000, 800,
        /* Settled: the scan (every ~2 s) and the reports (every 5 s) are the
         * ticks left on the oscilloscope on 2026-09-16 16:34.  Once LPo has
         * been quiet for a minute they slow to 20 s / 30 s, and the loop only
         * wakes on 3 consecutive readings (9 s) beyond +-35 mV. */
        60000, 35000, 3, 20000, 30000
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
        0,1000,1,1000,1000
    };
    static const int32_t high[CP_COUNT] = {
        8,8,4096,1,60000,600000,100,100,1000000,60000,255,255,600000,
        2400000,8,10000000,10000000,1000000,1000000,120000,2000000,
        1000000,3600000,600000,60000,4,26040,-1000,5000000,2000000,
        255,255,100000,120000,255,255,
        1000000,255,600000,10000,
        3600000,1000000,20,600000,600000
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
static int coarse_ready(const ControlPI *p,const int32_t *v,uint32_t now)
{ return !p->has_coarse || now-p->coarse_ms >= (uint32_t)v[CP_COARSE_MS]; }
static void coarse_moved(ControlPI *p,uint32_t now)
{ p->coarse_ms=now; p->has_coarse=1; }
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
        return;
    }
    /* 2. LPo on a rail: its magnitude is meaningless, walk IDAC3 by sign.
     * IDAC3 is local to the LP stage, so its cooldown is short. */
    if(!valid) {
        pi_forget(p); p->holding=0;
        if(!p->has_rescue || now-p->rescue_ms >= (uint32_t)v[CP_RESCUE_MS]) {
            step=((e>0)==(v[CP_FINE_SLOPE_UV]>0)) ? v[CP_RESCUE_STEP] : -v[CP_RESCUE_STEP];
            if((p->fine>=v[CP_FINE_LIMIT] && step>0)||(p->fine<=-v[CP_FINE_LIMIT] && step<0)) {
                if(coarse_ready(p,v,now)) {
                    move_coarse_bumpless(p,v,((e>0)==(v[CP_COARSE_SLOPE_UV]>0)) ? 1 : -1);
                    coarse_moved(p,now);
                }
            } else p->fine=(int16_t)bounded(p->fine+step,-v[CP_FINE_LIMIT],v[CP_FINE_LIMIT]);
            p->rescue_ms=now; p->has_rescue=1;
        }
        return;
    }
    /* 3. Hysteresis.  Once LPo is inside HOLD the loop freezes and ignores
     * every sample until LPo leaves DEADBAND.  Without it the integer loop
     * chased +-3 mV of ADC noise and never stopped moving. */
    if(p->holding) {
        int settled=control_pi_settled(p,c,now);
        int32_t band=settled ? v[CP_SETTLED_BAND_UV] : v[CP_DEADBAND_UV];
        pi_forget(p);
        if(magnitude(lp)<=band) { p->wake_count=0; return; }
        /* Settled: a lone reading outside is a glitch, not drift. */
        if(settled && ++p->wake_count < (uint8_t)v[CP_WAKE_COUNT]) return;
        p->holding=0; p->wake_count=0;
    } else if(magnitude(lp)<=v[CP_HOLD_UV]) {
        p->holding=1; p->hold_ms=now; p->wake_count=0; pi_forget(p); return;
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
    /* 5. PI toward zero, in fractional codes.  alpha=T/tau of the error per
     * period plus Kp on its change; tolerates ~1/alpha of slope error. */
    if(dt>(uint32_t)v[CP_PERIOD_MS]*2u) dt=(uint32_t)v[CP_PERIOD_MS];
    denom=(int64_t)v[CP_TAU_MS]*v[CP_FINE_SLOPE_UV];
    increment=((int64_t)e+(p->has_previous?p->previous_error:e))*dt*32768/denom;
    increment+=(int64_t)(e-(p->has_previous?p->previous_error:0))*v[CP_PERIOD_MS]*65536/denom*v[CP_KP_NUM]/v[CP_KP_DEN];
    p->fraction+=increment;
    step=(int32_t)(p->fraction/65536);
    step=bounded(step,-v[CP_FINE_STEP],v[CP_FINE_STEP]);
    if(step!=0) {
        int32_t next=bounded(p->fine+step,-v[CP_FINE_LIMIT],v[CP_FINE_LIMIT]);
        p->fraction-=((int64_t)(next-p->fine))*65536;
        if(next==v[CP_FINE_LIMIT] || next==-v[CP_FINE_LIMIT] || magnitude(p->fraction)>65536) p->fraction=0;
        p->fine=(int16_t)next;
    }
    p->previous_error=e; p->previous_ms=now; p->has_previous=1;
}
